// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/analyzecoins.h>
#include <wallet/bls_tx.h>
#include <wallet/wallet.h>
#include <benchmark.h>
#include <chain.h>
#include <coins.h>
#include <checkpoints.h>
#include <config.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <dstencode.h>
#include <init.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <keystore.h>
#include <net.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <scheduler.h>
#include <script/script.h>
#include <script/sighashtype.h>
#include <script/sign.h>
#include <shutdown.h>
#include <timedata.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util/strencodings.h> // for debug
#include <util/splitstring.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <utxo_functions.h> // for GetUTXOSet in SweepCoinsToWallet
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/finaltx.h>
#include <wallet/mnemonic.h>
#include <rpc/server.h> // for IsDeprecatedRPCEnabled

#include <bls/bls_functions.h>

#include <cassert>
#include <future>
#include <random>
#include <variant>
#include <optional>

// Forward declaration
namespace bls {
    class Signature;
}

/** Transaction fee set by the user */
CFeeRate payTxFee(DEFAULT_TRANSACTION_FEE);
bool bSpendZeroConfChange = DEFAULT_SPEND_ZEROCONF_CHANGE;

static CCriticalSection cs_wallets;
static std::vector<std::shared_ptr<CWallet>> vpwallets GUARDED_BY(cs_wallets);

bool AddWallet(const std::shared_ptr<CWallet> &wallet) {
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i =
        std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i != vpwallets.end()) {
        return false;
    }
    vpwallets.push_back(wallet);
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet> &wallet) {
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::iterator i =
        std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i == vpwallets.end()) {
        return false;
    }
    vpwallets.erase(i);
    return true;
}

bool HasWallets() {
    LOCK(cs_wallets);
    return !vpwallets.empty();
}

std::vector<std::shared_ptr<CWallet>> GetWallets() {
    LOCK(cs_wallets);
    return vpwallets;
}

std::shared_ptr<CWallet> GetWallet(const std::string &name) {
    LOCK(cs_wallets);
    for (const std::shared_ptr<CWallet> &wallet : vpwallets) {
        if (wallet->GetName() == name) {
            return wallet;
        }
    }
    return nullptr;
}

static Mutex g_wallet_release_mutex;
static std::condition_variable g_wallet_release_cv;
static std::set<std::string> g_unloading_wallet_set;

// Custom deleter for shared_ptr<CWallet>.
static void ReleaseWallet(CWallet *wallet) {
    // Unregister and delete the wallet right after
    // BlockUntilSyncedToCurrentChain so that it's in sync with the current
    // chainstate.
    const std::string name = wallet->GetName();
    LogPrintf("Releasing wallet\n");
    wallet->BlockUntilSyncedToCurrentChain();
    wallet->Flush();
    UnregisterValidationInterface(wallet);
    delete wallet;
    // Wallet is now released, notify UnloadWallet, if any.
    {
        LOCK(g_wallet_release_mutex);
        if (g_unloading_wallet_set.erase(name) == 0) {
            // UnloadWallet was not called for this wallet, all done.
            return;
        }
    }
    g_wallet_release_cv.notify_all();
}

void UnloadWallet(std::shared_ptr<CWallet> &&wallet) {
    // Mark wallet for unloading.
    const std::string name = wallet->GetName();
    {
        LOCK(g_wallet_release_mutex);
        auto it = g_unloading_wallet_set.insert(name);
        assert(it.second);
    }
    // The wallet can be in use so it's not possible to explicitly unload here.
    // Notify the unload intent so that all remaining shared pointers are
    // released.
    wallet->NotifyUnload();
    // Time to ditch our shared_ptr and wait for ReleaseWallet call.
    wallet.reset();
    {
        WAIT_LOCK(g_wallet_release_mutex, lock);
        while (g_unloading_wallet_set.count(name) == 1) {
            g_wallet_release_cv.wait(lock);
        }
    }
}

OutputType g_address_type = OutputType::LEGACY;

const char *DEFAULT_WALLET_DAT = "wallet.dat";

std::shared_ptr<CWallet> LoadWallet(const CChainParams &chainParams,
                                    interfaces::Chain &chain,
                                    const WalletLocation &location,
                                    std::string &error, std::string &warning) {
    if (!CWallet::Verify(chainParams, chain, location, false, error, warning)) {
        error = "Wallet file verification failed: " + error;
        return nullptr;
    }
    WalletFlag flag;
    std::shared_ptr<CWallet> wallet =
      CWallet::CreateWalletFromFile(chainParams, chain, location, SecureString(""), std::vector<std::string>(), flag);
    if (!wallet) {
        error = "Wallet loading failed.";
        return nullptr;
    }
    AddWallet(wallet);
    wallet->postInitProcess();
    return wallet;
}

std::shared_ptr<CWallet> LoadWallet(const CChainParams &chainParams,
                                    interfaces::Chain &chain,
                                    const std::string &name, std::string &error,
                                    std::string &warning) {
    return LoadWallet(chainParams, chain, WalletLocation(name), error, warning);
}

/**
 * If fee estimation does not have enough data to provide estimates, use this
 * fee instead. Has no effect if not using fee estimation.
 * Override with -fallbackfee
 */
CFeeRate CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);

const BlockHash CMerkleTx::ABANDON_HASH(uint256S(
    "0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

struct CompareValueOnly {
    bool operator()(const CInputCoin &t1, const CInputCoin &t2) const {
        return t1.txout.nValue < t2.txout.nValue;
    }
};

std::string COutput::ToString() const {
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetId().ToString(), i,
                     nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

class CAffectedKeysVisitor {
private:
    const CKeyStore &keystore;
    std::vector<CKeyID> &vKeys;
    std::vector<BKeyID> &v1Keys;

public:
    CAffectedKeysVisitor(const CKeyStore &keystoreIn,
                         std::vector<CKeyID> &vKeysIn,
        std::vector<BKeyID> &vKeys1In)
        : keystore(keystoreIn), vKeys(vKeysIn), v1Keys(vKeys1In) {}

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            for (const CTxDestination &dest : vDest) {
              std::visit(*this, dest);
            }
        }
    }

    void operator()(const CKeyID &keyId) {
        if (keystore.HaveKey(keyId)) {
            vKeys.push_back(keyId);
        }
    }
    void operator()(const BKeyID &keyId) {
        if (keystore.HaveKey(keyId)) {
            v1Keys.push_back(keyId);
        }
    }

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (keystore.GetCScript(scriptId, script)) {
            Process(script);
        }
    }

    void operator()(const CNoDestination &none) {}
};

//------------------------------------------------------------------------------------------
/* Generates a new HD master key (will not be activated) */
//------------------------------------------------------------------------------------------
std::tuple<mnemonic::WordList, std::vector<uint8_t> > GenerateHDMasterKey() {
  std::vector<uint8_t> hashWords;
  mnemonic::WordList words;
  bool success;
  std::string seedphrase = gArgs.GetArg("-seedphrase","").c_str();
  if (seedphrase != "") {
    std::tie(success, words, hashWords) = mnemonic::CheckSeedPhrase(seedphrase);
    if (!success) {
      InitError(_("Invalid Seed Phrase"));
      return std::tuple(words, hashWords);
    }
  } else {

    std::tie(words, hashWords) = mnemonic::GenerateSeedPhrase();
    std::string notice = "This is your seed phrase, please write it down to recover wallet \n\"" + join(words," ")+"\"\n" +
        "NEVER SHARE THIS SEQUENCE WITH ANYONE TO PROTECT YOUR FUNDS";
    ShowSeedPhrase(notice);
  }

  return std::tuple(words, hashWords);
}
//------------------------------------------------------------------------------------------

const CWalletTx *CWallet::GetWalletTx(const TxId &txid) const {
    LOCK(cs_wallet);
    auto it = mapWallet.find(txid);
    if (it == mapWallet.end()) {
        return nullptr;
    }

    return &(it->second);
}

std::tuple<CPubKey, CHDPubKey> CWallet::GenerateNewKey(CHDChain& hdChainDec, bool internal) {
    //assert(!IsWalletPrivate());
    //assert(IsWalletBlank());
    // mapKeyMetadata
    AssertLockHeld(cs_wallet);

    CKey secret;
    CHDPubKey HDKey;
  
    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // use HD key derivation since HD was enabled during wallet creation
    // for now we use a fixed keypath scheme of m/0'/0'/k
    // master key seed (256bit)
    CKey key;
    // hd master key
    CExtKey masterKey;
    // key at m/0'
    //CExtKey accountKey;
    // key at m/0'/0' (external) or m/0'/1' (internal)
    //CExtKey chainChildKey;
    // key at m/0'/0'/<n>'
    CExtKey childKey;

    CHDAccount acc;
    int nAccountIndex = 0;
    if (UseBLSKeys()) nAccountIndex = BLS_ACCOUNT;
        
    hdChainDec.GetAccount(nAccountIndex, acc);
  
    // derive child key at next index, skip keys already known to the wallet
    uint32_t nChildIndex = internal ? acc.nInternalChainCounter : acc.nExternalChainCounter;
    bool cont = true;
    do {
      hdChainDec.DeriveChildExtKey(nAccountIndex, internal, nChildIndex, childKey, UseBLSKeys());
      // increment childkey index
      nChildIndex++;
      if (UseBLSKeys()) cont = HaveKey(childKey.key.GetPubKeyForBLS().GetBLSKeyID());
      else cont = HaveKey(childKey.key.GetPubKey().GetKeyID());
    } while (cont);
        
    secret = childKey.key;
    CPubKey pubkey;
      
    if (UseBLSKeys()) {
      pubkey = secret.GetPubKeyForBLS();
    } else {
      pubkey = secret.GetPubKey();
    }
      
    bool ok = (secret.VerifyPubKey(pubkey));
    assert(ok);
  
    // store metadata
    UpdateTimeFirstKey(metadata.nCreateTime);

    if (internal) {
      acc.nInternalChainCounter = nChildIndex;
    } else {
      acc.nExternalChainCounter = nChildIndex;
    }

    if (!hdChainDec.SetAccount(nAccountIndex, acc))
      throw std::runtime_error(std::string(__func__) + ": SetAccount failed");

    CExtPubKey neutered;

    try {
        if (pubkey.IsBLS()) {
            neutered = childKey.NeuterBLS();
        } else {
            neutered = childKey.Neuter();
        }
    }
    catch (...) {
        throw std::runtime_error(std::string(__func__) + ": childkey.Neuter failed");
    }
    
    HDKey = AddHDPubKeyWithoutDB(neutered, internal);
    metadata.hdKeypath  = HDKey.GetKeyPath();

    if (pubkey.IsBLS()) {
        mapBLSKeyMetadata[pubkey.GetBLSKeyID()] = metadata;
    } else {
        mapKeyMetadata[pubkey.GetKeyID()] = metadata;
    }
    return std::tuple(pubkey, HDKey);

}

void CWallet::LoadKeyMetadata(const CKeyID &keyID, const CKeyMetadata &meta) {
    // mapKeyMetadata
    AssertLockHeld(cs_wallet);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
}
void CWallet::LoadKeyMetadata(const BKeyID &keyID, const CKeyMetadata &meta) {
    // mapKeyMetadata
    AssertLockHeld(cs_wallet);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapBLSKeyMetadata[keyID] = meta;
}

void CWallet::LoadScriptMetadata(const CScriptID &script_id,
                                 const CKeyMetadata &meta) {
    // m_script_metadata
    AssertLockHeld(cs_wallet);
    UpdateTimeFirstKey(meta.nCreateTime);
    m_script_metadata[script_id] = meta;
}


/**
 * Update wallet first key creation time. This should be called whenever keys
 * are added to the wallet, with the oldest key creation time.
 */
void CWallet::UpdateTimeFirstKey(int64_t nCreateTime) {
    AssertLockHeld(cs_wallet);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

bool CWallet::AddCScript(const CScript &redeemScript) {
    if (!CCryptoKeyStore::AddCScript(redeemScript)) {
        return false;
    }
    return WalletBatch(*database).WriteCScript(Hash160(redeemScript), redeemScript);
}

bool CWallet::LoadCScript(const CScript &redeemScript) {
    /**
     * A sanity check was added in pull #3843 to avoid adding redeemScripts that
     * never can be redeemed. However, old wallets may still contain these. Do
     * not add them to the wallet and warn.
     */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        std::string strAddr = EncodeDestination(CScriptID(redeemScript));
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %i "
                  "which exceeds maximum size %i thus can never be redeemed. "
                  "Do not use address %s.\n",
                  __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE,
                  strAddr);
        return true;
    }

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::AddWatchOnly(const CScript &dest) {
    if (!CCryptoKeyStore::AddWatchOnly(dest)) {
        return false;
    }

    const CKeyMetadata &meta = m_script_metadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    if (WalletBatch(*database).WriteWatchOnly(dest, meta)) {
        UnsetWalletBlank();
        return true;
    }
    return false;
}

bool CWallet::AddWatchOnly(const CScript &dest, int64_t nCreateTime) {
    m_script_metadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}

bool CWallet::RemoveWatchOnly(const CScript &dest) {
    AssertLockHeld(cs_wallet);
    if (!CCryptoKeyStore::RemoveWatchOnly(dest)) {
        return false;
    }

    if (!HaveWatchOnly()) {
        NotifyWatchonlyChanged(false);
    }

    return WalletBatch(*database).EraseWatchOnly(dest);
}

bool CWallet::LoadWatchOnly(const CScript &dest) {
    return CCryptoKeyStore::AddWatchOnly(dest);
}

bool CWallet::Unlock(const SecureString &strWalletPassphrase) {
    CCrypter crypter;
    CKeyingMaterial _vMasterKey;

    if (!IsLocked()) return true;
  
    LOCK(cs_wallet);
    for (const MasterKeyMap::value_type &pMasterKey : mapMasterKeys) {
        if (!crypter.SetKeyFromPassphrase(
                strWalletPassphrase, pMasterKey.second.vchSalt,
                pMasterKey.second.nDeriveIterations,
                pMasterKey.second.nDerivationMethod)) {
            return false;
        }

        // Should get back original _vMasterKey....
        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey)) {
            // try another master key
            continue;
        }

        if (CCryptoKeyStore::Unlock(_vMasterKey)) {
            return true;
        }
    }

    return false;
}

bool CWallet::ChangeWalletPassphrase(
    const SecureString &strOldWalletPassphrase,
    const SecureString &strNewWalletPassphrase) {
    bool fWasLocked = IsLocked();

    LOCK(cs_wallet);
    Lock();

    CCrypter crypter;
    CKeyingMaterial _vMasterKey;
    for (MasterKeyMap::value_type &pMasterKey : mapMasterKeys) {
        if (!crypter.SetKeyFromPassphrase(
                strOldWalletPassphrase, pMasterKey.second.vchSalt,
                pMasterKey.second.nDeriveIterations,
                pMasterKey.second.nDerivationMethod)) {
            return false;
        }

        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey)) {
            return false;
        }

        if (CCryptoKeyStore::Unlock(_vMasterKey)) {
            int64_t nStartTime = GetTimeMillis();
            crypter.SetKeyFromPassphrase(strNewWalletPassphrase,
                                         pMasterKey.second.vchSalt,
                                         pMasterKey.second.nDeriveIterations,
                                         pMasterKey.second.nDerivationMethod);
            pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(
                pMasterKey.second.nDeriveIterations *
                (100 / ((double)(GetTimeMillis() - nStartTime))));

            nStartTime = GetTimeMillis();
            crypter.SetKeyFromPassphrase(strNewWalletPassphrase,
                                         pMasterKey.second.vchSalt,
                                         pMasterKey.second.nDeriveIterations,
                                         pMasterKey.second.nDerivationMethod);
            pMasterKey.second.nDeriveIterations =
                (pMasterKey.second.nDeriveIterations +
                 static_cast<unsigned int>(
                     pMasterKey.second.nDeriveIterations * 100 /
                     double(GetTimeMillis() - nStartTime))) /
                2;

            if (pMasterKey.second.nDeriveIterations < 25000) {
                pMasterKey.second.nDeriveIterations = 25000;
            }

            LogPrintf(
                "Wallet passphrase changed to an nDeriveIterations of %i\n",
                pMasterKey.second.nDeriveIterations);

            if (!crypter.SetKeyFromPassphrase(
                    strNewWalletPassphrase, pMasterKey.second.vchSalt,
                    pMasterKey.second.nDeriveIterations,
                    pMasterKey.second.nDerivationMethod)) {
                return false;
            }

            if (!crypter.Encrypt(_vMasterKey,
                                 pMasterKey.second.vchCryptedKey)) {
                return false;
            }

            WalletBatch batch(*database);
            batch.WriteMasterKey(pMasterKey.first, pMasterKey.second);
            batch.TxnCommit();

            if (fWasLocked) {
                Lock();
            }

            return true;
        }
    }

    return false;
}

void CWallet::ChainStateFlushed(const CBlockLocator &loc) {
    WalletBatch batch(*database);
    batch.WriteBestBlock(loc);
}

void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch *pbatchIn,
                            bool fExplicit) {
    // nWalletVersion
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion) {
        return;
    }

    // When doing an explicit upgrade, if we pass the max version permitted,
    // upgrade all the way.
    if (fExplicit && nVersion > nWalletMaxVersion) {
        nVersion = FEATURE_LATEST;
    }

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion) {
        nWalletMaxVersion = nVersion;
    }

    WalletBatch *pbatch = pbatchIn ? pbatchIn : new WalletBatch(*database);
    pbatch->WriteMinVersion(nWalletVersion);

    if (!pbatchIn) {
        delete pbatch;
    }
}

bool CWallet::SetMaxVersion(int nVersion) {
    // nWalletVersion, nWalletMaxVersion
    LOCK(cs_wallet);

    // Cannot downgrade below current version
    if (nWalletVersion > nVersion) {
        return false;
    }

    nWalletMaxVersion = nVersion;

    return true;
}

std::set<TxId> CWallet::GetConflicts(const TxId &txid) const {
    std::set<TxId> result;
    AssertLockHeld(cs_wallet);

    auto it = mapWallet.find(txid);
    if (it == mapWallet.end()) {
        return result;
    }

    const CWalletTx &wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for (const CTxIn &txin : wtx.tx->vin) {
        if (mapTxSpends.count(txin.prevout) <= 1) {
            // No conflict if zero or one spends.
            continue;
        }

        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second;
             ++_it) {
            result.insert(_it->second);
        }
    }

    return result;
}

bool CWallet::HasWalletSpend(const TxId &txid) const {
    AssertLockHeld(cs_wallet);
    auto iter = mapTxSpends.lower_bound(COutPoint(txid, 0));
    return (iter != mapTxSpends.end() && iter->first.GetTxId() == txid);
}

void CWallet::Flush(bool shutdown) {
    database->Flush(shutdown);
}

void CWallet::SyncMetaData(
    std::pair<TxSpends::iterator, TxSpends::iterator> range) {
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx *copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const CWalletTx *wtx = &mapWallet.at(it->second);
        if (wtx->nOrderPos < nMinOrderPos) {
            nMinOrderPos = wtx->nOrderPos;
            copyFrom = wtx;
        }
    }

    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const TxId &txid = it->second;
        CWalletTx *copyTo = &mapWallet.at(txid);
        if (copyFrom == copyTo) {
            continue;
        }

        assert(
            copyFrom &&
            "Oldest wallet transaction in range assumed to have been found.");

        if (!copyFrom->IsEquivalentTo(*copyTo)) {
            continue;
        }

        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose nTimeReceived not copied
        // on purpose.
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        copyTo->strFromAccount = copyFrom->strFromAccount;
        // nOrderPos not copied on purpose cached members not copied on purpose.
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction, spends it:
 */
bool CWallet::IsSpent(interfaces::Chain::Lock &locked_chain,
                      const COutPoint &outpoint) const {
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range =
        mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it) {
        const TxId &wtxid = it->second;
        auto mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain(locked_chain);
            if (depth > 0 || (depth == 0 && !mit->second.isAbandoned())) {
                // Spent
                return true;
            }
        }
    }

    return false;
}

void CWallet::AddToSpends(const COutPoint &outpoint, const TxId &wtxid) {
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}

void CWallet::AddToSpends(const TxId &wtxid) {
    auto it = mapWallet.find(wtxid);
    assert(it != mapWallet.end());
    CWalletTx &thisTx = it->second;
    // Coinbases don't spend anything!
    if (thisTx.IsCoinBase()) {
        return;
    }

    for (const CTxIn &txin : thisTx.tx->vin) {
        AddToSpends(txin.prevout, wtxid);
    }
}

bool CWallet::EncryptHDWallet(const CKeyingMaterial& _vMasterKey,
                              const mnemonic::WordList& words,
                              const std::vector<uint8_t>& hashWords) {

  {
    LOCK(cs_wallet);
    CHDChain hdc;

    assert(words.size() != 0);
    assert(hashWords.size() != 0);
    
    hdc.Setup(words, hashWords);
        
    // BLS Account will be 1, EC is 0
    if (UseBLSKeys()) {
        hdc.AddAccount(BLS_ACCOUNT);
    } else {
        hdc.AddAccount(0);
    }
        
      
    // Should not be Null since just setup 
    if (!hdc.IsNull()) {
      bool ok = EncryptHDChain(_vMasterKey, hdc);
      assert(ok);
        
      CHDChain hdChainCrypted;
      GetCryptedHDChain(hdChainCrypted);
      assert(!hdChainCrypted.IsNull());
        
      // ids should match, seed hashes should not
      assert(hdc.GetID() == hdChainCrypted.GetID());
      assert(hdc.GetSeedHash() != hdChainCrypted.GetSeedHash());
      
      ok = StoreCryptedHDChain(hdChainCrypted);
      assert(ok);
    }
    
  }
  return true;
}

void CWallet::FinishEncryptWallet() {
  {
        LOCK(cs_wallet);
        WalletBatch(*database).TxnCommit();
        Lock();
  }
}

bool CWallet::CreateMasteyKey(const SecureString &strWalletPassphrase,
                              CKeyingMaterial& _vMasterKey) {
  
    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&_vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey; // Based on Password and random iterations

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(
        2500000 / double(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                 kMasterKey.nDeriveIterations,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations =
        (kMasterKey.nDeriveIterations +
         static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 /
                                   double(GetTimeMillis() - nStartTime))) /
        2;

    if (kMasterKey.nDeriveIterations < 25000) {
        kMasterKey.nDeriveIterations = 25000;
    }

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n",
              kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                      kMasterKey.nDeriveIterations,
                                      kMasterKey.nDerivationMethod)) {
        return false;
    }

    // Go from Random Bytes to Create Crypted value for kMasterKey
    if (!crypter.Encrypt(_vMasterKey, kMasterKey.vchCryptedKey)) {
        return false;
    }
    // Now kMasterKey is complete

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        WalletBatch batch(*database);
        if (!batch.TxnBegin()) {
            return false;
        }
        batch.WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        batch.TxnCommit();
    }
    return true;
}

DBErrors CWallet::ReorderTransactions() {
    LOCK(cs_wallet);
    WalletBatch batch(*database);

    // Old wallets didn't have any defined order for transactions. Probably a
    // bad idea to change the output of this.

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-time
    // multimap.
    TxItems txByTime;

    for (auto& it : mapWallet) {
        CWalletTx *wtx = &(it.second);
        txByTime.insert(std::make_pair(wtx->nTimeReceived, TxPair(wtx, nullptr)));
    }

    std::list<CAccountingEntry> acentries;
    batch.ListAccountCreditDebit("", acentries);
    for (CAccountingEntry &entry : acentries) {
        txByTime.insert(std::make_pair(entry.nTime, TxPair(nullptr, &entry)));
    }

    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (auto& it : txByTime) {
        CWalletTx *const pwtx = it.second.first;
        CAccountingEntry *const pacentry = it.second.second;
        int64_t &nOrderPos =
            (pwtx != nullptr) ? pwtx->nOrderPos : pacentry->nOrderPos;

        if (nOrderPos == -1) {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (pwtx) {
                if (!batch.WriteTx(*pwtx)) {
                    return DBErrors::LOAD_FAIL;
                }
            } else if (!batch.WriteAccountingEntry(pacentry->nEntryNo,
                                                      *pacentry)) {
                return DBErrors::LOAD_FAIL;
            }
        } else {
            int64_t nOrderPosOff = 0;
            for (const int64_t &nOffsetStart : nOrderPosOffsets) {
                if (nOrderPos >= nOffsetStart) {
                    ++nOrderPosOff;
                }
            }

            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff) {
                continue;
            }

            // Since we're changing the order, write it back.
            if (pwtx) {
                if (!batch.WriteTx(*pwtx)) {
                    return DBErrors::LOAD_FAIL;
                }
            } else if (!batch.WriteAccountingEntry(pacentry->nEntryNo,
                                                      *pacentry)) {
                return DBErrors::LOAD_FAIL;
            }
        }
    }

    batch.WriteOrderPosNext(nOrderPosNext);

    return DBErrors::LOAD_OK;
}

int64_t CWallet::IncOrderPosNext(WalletBatch *pbatch) {
    // nOrderPosNext
    AssertLockHeld(cs_wallet);
    int64_t nRet = nOrderPosNext++;
    if (pbatch) {
        pbatch->WriteOrderPosNext(nOrderPosNext);
    } else {
        WalletBatch(*database).WriteOrderPosNext(nOrderPosNext);
    }

    return nRet;
}

bool CWallet::AccountMove(std::string strFrom, std::string strTo,
                          const Amount nAmount, std::string strComment) {
    WalletBatch batch(*database);
    if (!batch.TxnBegin()) {
        return false;
    }

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = IncOrderPosNext(&batch);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    AddAccountingEntry(debit, &batch);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = IncOrderPosNext(&batch);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    AddAccountingEntry(credit, &batch);

    return batch.TxnCommit();
}

bool CWallet::GetLabelDestination(std::string &dest,
                                  const std::string &label) {
    WalletBatch batch(*database);
    return batch.ReadLabel(dest, label);
}

DBErrors CWallet::FindLabelledAddresses(std::map<std::string, std::string>& mapLabels) {
    WalletBatch batch(*database);
    return batch.FindLabelledAddresses(mapLabels);
}
    
void CWallet::MarkDirty() {
    LOCK(cs_wallet);
    for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
        item.second.MarkDirty();
    }
}

bool CWallet::AddToWallet(const CWalletTx &wtxIn, bool fFlushOnClose) {
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+", fFlushOnClose);

    const TxId &txid = wtxIn.GetId();

    // Inserts only if not already there, returns tx inserted or tx found.
    std::pair<std::map<TxId, CWalletTx>::iterator, bool> ret =
        mapWallet.insert(std::make_pair(txid, wtxIn));
    CWalletTx &wtx = (*ret.first).second;
    wtx.BindWallet(this);
    bool fInsertedNew = ret.second;
    if (fInsertedNew) {
        wtx.nTimeReceived = GetAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(
            std::make_pair(wtx.nOrderPos, TxPair(&wtx, nullptr)));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(txid);
    }

    bool fUpdated = false;
    if (!fInsertedNew) {
        // Merge
        if (!wtxIn.hashUnset() && wtxIn.hashBlock != wtx.hashBlock) {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }

        // If no longer abandoned, update
        if (wtxIn.hashBlock.IsNull() && wtx.isAbandoned()) {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }

        if (wtxIn.nIndex != -1 && (wtxIn.nIndex != wtx.nIndex)) {
            wtx.nIndex = wtxIn.nIndex;
            fUpdated = true;
        }

        if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe) {
            wtx.fFromMe = wtxIn.fFromMe;
            fUpdated = true;
        }
    }

    //// debug print
    LogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetId().ToString(),
              (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if ((fInsertedNew || fUpdated) && !batch.WriteTx(wtx)) {
        return false;
    }

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction.
    NotifyTransactionChanged(this, txid, fInsertedNew ? CT_NEW : CT_UPDATED);

    // Notify an external script when a wallet transaction comes in or is
    // updated.
    std::string strCmd = gArgs.GetArg("-walletnotify", "");

    if (!strCmd.empty()) {
        strCmd.replace(strCmd.find("%s"), 2, wtxIn.GetId().GetHex());
        std::thread t(runCommand, strCmd);
        // Thread runs free.
        t.detach();
    }

    return true;
}

void CWallet::LoadToWallet(const CWalletTx &wtxIn) {
    const TxId &txid = wtxIn.GetId();
    const auto &ins = mapWallet.emplace(txid, wtxIn);
    CWalletTx &wtx = ins.first->second;
    wtx.BindWallet(this);
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(
            std::make_pair(wtx.nOrderPos, TxPair(&wtx, nullptr)));
    }
    AddToSpends(txid);
    for (const CTxIn &txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.GetTxId());
        if (it != mapWallet.end()) {
            CWalletTx &prevtx = it->second;
            if (prevtx.nIndex == -1 && !prevtx.hashUnset()) {
                MarkConflicted(prevtx.hashBlock, wtx.GetId());
            }
        }
    }
}

/**
 * Add a transaction to the wallet, or update it.  pIndex and posInBlock should
 * be set when the transaction was known to be included in a block. When pIndex
 * == nullptr, then wallet state is not updated in AddToWallet, but
 * notifications happen and cached balances are marked dirty.
 *
 * If fUpdate is true, existing transactions will be updated.
 * TODO: One exception to this is that the abandoned state is cleared under the
 * assumption that any further notification of a transaction that was considered
 * abandoned is an indication that it is not safe to be considered abandoned.
 * Abandoned state should probably be more carefuly tracked via different
 * posInBlock signals or by checking mempool presence when necessary.
 */
bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef &ptx,
                                       const CBlockIndex *pIndex,
                                       int posInBlock, bool fUpdate) {
    const CTransaction &tx = *ptx;
    AssertLockHeld(cs_wallet);

    if (pIndex != nullptr) {
        for (const CTxIn &txin : tx.vin) {
            std::pair<TxSpends::const_iterator, TxSpends::const_iterator>
                range = mapTxSpends.equal_range(txin.prevout);
            while (range.first != range.second) {
                if (range.first->second != tx.GetId()) {
                    LogPrintf("Transaction %s (in block %s) conflicts with "
                              "wallet transaction %s (both spend %s:%i)\n",
                              tx.GetId().ToString(),
                              pIndex->GetBlockHash().ToString(),
                              range.first->second.ToString(),
                              range.first->first.GetTxId().ToString(),
                              range.first->first.GetN());
                    MarkConflicted(pIndex->GetBlockHash(), range.first->second);
                }
                range.first++;
            }
        }
    }

    bool fExisted = mapWallet.count(tx.GetId()) != 0;
    if (fExisted && !fUpdate) {
        return false;
    }
    if (fExisted || IsMine(tx) || IsFromMe(tx)) {
        /**
         * Check if any keys in the wallet keypool that were supposed to be
         * unused have appeared in a new transaction. If so, remove those keys
         * from the keypool. This can happen when restoring an old wallet backup
         * that does not contain the mostly recently created transactions from
         * newer versions of the wallet.
         */

        // loop though all outputs
        for (const CTxOut &txout : tx.vout) {
            // extract addresses and check if they match with an unused keypool
            // key
            std::vector<CKeyID> vAffected;
            std::vector<BKeyID> v1Affected;
            CAffectedKeysVisitor(*this, vAffected, v1Affected).Process(txout.scriptPubKey);
            for (const CKeyID &keyid : vAffected) {
                  auto mi = m_pool_key_to_index.find(keyid);
                  if (mi != m_pool_key_to_index.end()) {
                      LogPrintf("%s: Detected a used keypool key, mark all "
                                "keypool key up to this key as used\n",
                                __func__);
                      MarkReserveKeysAsUsed(mi->second);

                      if (!TopUpKeyPool()) {
                          LogPrintf(
                              "%s: Topping up keypool failed (locked wallet)\n",
                              __func__);
                      }
                  }
            }
            for (const BKeyID &keyid : v1Affected) {
                  auto mi = m_pool_blskey_to_index.find(keyid);
                  if (mi != m_pool_blskey_to_index.end()) {
                      LogPrintf("%s: Detected a used keypool key, mark all "
                             "keypool key up to this key as used\n",
                             __func__);
                      MarkReserveKeysAsUsed(mi->second);
          
                      if (!TopUpKeyPool()) {
                          LogPrintf(
                                    "%s: Topping up keypool failed (locked wallet)\n",
                                    __func__);
                      }
                  }
            }
        }

        
        CWalletTx wtx(this, ptx);

        // Get merkle branch if transaction was found in a block
        if (pIndex != nullptr) {
            wtx.SetMerkleBranch(pIndex, posInBlock);
        }

        return AddToWallet(wtx, false);
    }

    return false;
}

bool CWallet::TransactionCanBeAbandoned(const TxId &txid) const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);
    const CWalletTx *wtx = GetWalletTx(txid);
    return wtx && !wtx->isAbandoned() &&
           wtx->GetDepthInMainChain(*locked_chain) == 0 && !wtx->InMempool();
}

void CWallet::MarkInputsDirty(const CTransactionRef &tx) {
    for (const CTxIn &txin : tx->vin) {
        auto it = mapWallet.find(txin.prevout.GetTxId());
        if (it != mapWallet.end()) {
            it->second.MarkDirty();
        }
    }
}

bool CWallet::AbandonTransaction(interfaces::Chain::Lock &locked_chain,
                                 const TxId &txid) {
    // Temporary. Removed in upcoming lock cleanup
    auto locked_chain_recursive = chain().lock();
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+");

    std::set<TxId> todo;
    std::set<TxId> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(txid);
    assert(it != mapWallet.end());
    CWalletTx &origtx = it->second;
    if (origtx.GetDepthInMainChain(locked_chain) != 0 || origtx.InMempool()) {
        return false;
    }

    todo.insert(txid);

    while (!todo.empty()) {
        const TxId now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx &wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain(locked_chain);
        // If the orig tx was not in block, none of its spends can be.
        assert(currentconfirm <= 0);
        // If (currentconfirm < 0) {Tx and spends are already conflicted, no
        // need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can
            // be in mempool.
            assert(!wtx.InMempool());
            wtx.nIndex = -1;
            wtx.setAbandoned();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetId(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet
            // that spend them abandoned too.
            TxSpends::const_iterator iter =
                mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.GetTxId() == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }

            // If a transaction changes 'conflicted' state, that changes the
            // balance available of the outputs it spends. So force those to be
            // recomputed.
            MarkInputsDirty(wtx.tx);
        }
    }

    return true;
}

void CWallet::MarkConflicted(const BlockHash &hashBlock, const TxId &txid) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    int conflictconfirms = 0;
    if (mapBlockIndex.count(hashBlock)) {
        CBlockIndex *pindex = mapBlockIndex[hashBlock];
        if (chainActive.Contains(pindex)) {
            conflictconfirms = -(chainActive.Height() - pindex->nHeight + 1);
        }
    }

    // If number of conflict confirms cannot be determined, this means that the
    // block is still unknown or not yet part of the main chain, for example
    // when loading the wallet during a reindex. Do nothing in that case.
    if (conflictconfirms >= 0) {
        return;
    }

    // Do not flush the wallet here for performance reasons.
    WalletBatch batch(*database, "r+", false);

    std::set<TxId> todo;
    std::set<TxId> done;

    todo.insert(txid);

    while (!todo.empty()) {
        const TxId now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx &wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain(*locked_chain);
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.nIndex = -1;
            wtx.hashBlock = hashBlock;
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet
            // that spend them conflicted too.
            TxSpends::const_iterator iter =
                mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.GetTxId() == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }

            // If a transaction changes 'conflicted' state, that changes the
            // balance available of the outputs it spends. So force those to be
            // recomputed.
            MarkInputsDirty(wtx.tx);
        } else {
          LogPrintf("Conflicted Transaction %s in wallet removed since it's replaced with Tx in block\n", now.ToString());
          batch.EraseTx(now);
          MarkDirty();
          NotifyTransactionChanged(this, wtx.GetId(), CT_DELETED);
        }
    }
}

void CWallet::SyncTransaction(const CTransactionRef &ptx,
                              const CBlockIndex *pindex, int posInBlock,
                              bool update_tx) {
    if (!AddToWalletIfInvolvingMe(ptx, pindex, posInBlock, update_tx)) {
        // Not one of ours
        return;
    }

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(ptx);
}

void CWallet::TransactionAddedToMempool(const CTransactionRef &ptx) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);
    SyncTransaction(ptx);

    auto it = mapWallet.find(ptx->GetId());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(ptx->GetId());
    if (it != mapWallet.end()) {
        it->second.fInMempool = false;
    }
}

void CWallet::BlockConnected(
    const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex,
    const std::vector<CTransactionRef> &vtxConflicted) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    // TODO: Temporarily ensure that mempool removals are notified before
    // connected transactions. This shouldn't matter, but the abandoned state of
    // transactions in our wallet is currently cleared when we receive another
    // notification and there is a race condition where notification of a
    // connected conflict might cause an outside process to abandon a
    // transaction and then have it inadvertently cleared by the notification
    // that the conflicted transaction was evicted.
    for (const CTransactionRef &ptx : vtxConflicted) {
        SyncTransaction(ptx);
        TransactionRemovedFromMempool(ptx);
    }

    for (size_t i = 0; i < pblock->vtx.size(); i++) {
        SyncTransaction(pblock->vtx[i], pindex, i);
        TransactionRemovedFromMempool(pblock->vtx[i]);
    }

    m_last_block_processed = pindex;
}

void CWallet::BlockDisconnected(const std::shared_ptr<const CBlock> &pblock) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    for (const CTransactionRef &ptx : pblock->vtx) {
        SyncTransaction(ptx);
    }
}

void CWallet::BlockUntilSyncedToCurrentChain() {
    AssertLockNotHeld(cs_main);
    AssertLockNotHeld(cs_wallet);

    {
        // Skip the queue-draining stuff if we know we're caught up with
        // chainActive.Tip()...
        // We could also take cs_wallet here, and call m_last_block_processed
        // protected by cs_wallet instead of cs_main, but as long as we need
        // cs_main here anyway, it's easier to just call it cs_main-protected.
        auto locked_chain = chain().lock();
        const CBlockIndex *initialChainTip = chainActive.Tip();
        if (m_last_block_processed &&
            m_last_block_processed->GetAncestor(initialChainTip->nHeight) ==
                initialChainTip) {
            return;
        }
    }

    // ...otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    SyncWithValidationInterfaceQueue();
}

isminetype CWallet::IsMine(const CTxIn &txin) const {
    LOCK(cs_wallet);
    auto mi =  mapWallet.find(txin.prevout.GetTxId());
    if (mi != mapWallet.end()) {
        const CWalletTx &prev = (*mi).second;
        if (txin.prevout.GetN() < prev.tx->vout.size()) {
            return IsMine(prev.tx->vout[txin.prevout.GetN()]);
        }
    }

    return ISMINE_NO;
}

// Note that this function doesn't distinguish between a 0-valued input, and a
// not-"is mine" (according to the filter) input.
Amount CWallet::GetDebit(const CTxIn &txin, const isminefilter &filter) const {
    LOCK(cs_wallet);
    auto mi = mapWallet.find(txin.prevout.GetTxId());
    if (mi != mapWallet.end()) {
        const CWalletTx &prev = (*mi).second;
        if (txin.prevout.GetN() < prev.tx->vout.size()) {
            if (IsMine(prev.tx->vout[txin.prevout.GetN()]) & filter) {
                return prev.tx->vout[txin.prevout.GetN()].nValue;
            }
        }
    }

    return Amount::zero();
}

isminetype CWallet::IsMine(const CTxOut &txout) const {
    return ::IsMine(*this, txout.scriptPubKey);
}

Amount CWallet::GetCredit(const CTxOut &txout,
                          const isminefilter &filter) const {
    if (!MoneyRange(txout.nValue)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": value out of range");
    }

    return (IsMine(txout) & filter) ? txout.nValue : Amount::zero();
}

bool CWallet::IsChange(const CTxOut &txout) const {
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book is
    // change. That assumption is likely to break when we implement
    // multisignature wallets that return change back into a
    // multi-signature-protected address; a better way of identifying which
    // outputs are 'the send' and which are 'the change' will need to be
    // implemented (maybe extend CWalletTx to remember which output, if any, was
    // change).
    if (::IsMine(*this, txout.scriptPubKey)) {
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address)) {
            return true;
        }

        LOCK(cs_wallet);
        if (!mapAddressBook.count(address)) {
            return true;
        }
    }

    return false;
}

Amount CWallet::GetChange(const CTxOut &txout) const {
    if (!MoneyRange(txout.nValue)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": value out of range");
    }

    return (IsChange(txout) ? txout.nValue : Amount::zero());
}

bool CWallet::IsMine(const CTransaction &tx) const {
    for (const CTxOut &txout : tx.vout) {
        if (IsMine(txout)) {
            return true;
        }
    }

    return false;
}

bool CWallet::IsFromMe(const CTransaction &tx) const {
    return GetDebit(tx, ISMINE_ALL) > Amount::zero();
}

Amount CWallet::GetDebit(const CTransaction &tx,
                         const isminefilter &filter) const {
    Amount nDebit = Amount::zero();
    for (const CTxIn &txin : tx.vin) {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nDebit;
}

bool CWallet::IsAllFromMe(const CTransaction &tx,
                          const isminefilter &filter) const {
    LOCK(cs_wallet);

    for (const CTxIn &txin : tx.vin) {
        auto mi = mapWallet.find(txin.prevout.GetTxId());
        if (mi == mapWallet.end()) {
            // Any unknown inputs can't be from us.
            return false;
        }

        const CWalletTx &prev = (*mi).second;

        if (txin.prevout.GetN() >= prev.tx->vout.size()) {
            // Invalid input!
            return false;
        }

        if (!(IsMine(prev.tx->vout[txin.prevout.GetN()]) & filter)) {
            return false;
        }
    }

    return true;
}

Amount CWallet::GetCredit(const CTransaction &tx,
                          const isminefilter &filter) const {
    Amount nCredit = Amount::zero();
    for (const CTxOut &txout : tx.vout) {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nCredit;
}

Amount CWallet::GetChange(const CTransaction &tx) const {
    Amount nChange = Amount::zero();
    for (const CTxOut &txout : tx.vout) {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nChange;
}

bool CWallet::CanGenerateKeys() {
    LOCK(cs_wallet);
    return true;
}

bool CWallet::CanGetAddresses(bool internal) {
    LOCK(cs_wallet);
    // Check if the keypool has keys
    bool keypool_has_keys;
    if (internal && CanSupportFeature(FEATURE_FLAGS)) {
        keypool_has_keys = setInternalKeyPool.size() > 0;
    } else {
        keypool_has_keys = KeypoolCountExternalKeys() > 0;
    }
    // If the keypool doesn't have keys, check if we can generate them
    if (!keypool_has_keys) {
        return CanGenerateKeys();
    }
    return keypool_has_keys;
}

void CWallet::SetWalletBlank() {
    LOCK(cs_wallet);
    m_wallet_flags.SetBlank();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::SetWalletBLS() {
    LOCK(cs_wallet);
    m_wallet_flags.SetBLS();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::SetWalletLegacy() {
    LOCK(cs_wallet);
    m_wallet_flags.SetLegacyWallet();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::UnsetWalletLEGACY() {
    LOCK(cs_wallet);
    m_wallet_flags.UnsetLEGACY();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::SetWalletPrivate() {
    LOCK(cs_wallet);
    m_wallet_flags.SetPrivate();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::UnsetWalletBlank() {
    LOCK(cs_wallet);
    m_wallet_flags.UnsetBlank();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}
void CWallet::UnsetWalletPrivate() {
    LOCK(cs_wallet);
    m_wallet_flags.UnsetPrivate();
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +  ": writing wallet flags failed");
    }
}

bool CWallet::IsWalletBlank() const { return (m_wallet_flags.GetBlank()); }
bool CWallet::IsWalletBLS() const { return (m_wallet_flags.HasBLS()); }
bool CWallet::IsWalletLegacy() const { return (m_wallet_flags.HasLEGACY()); }
bool CWallet::IsWalletPrivate() const { return (m_wallet_flags.GetPrivate()); }

/*
bool CWallet::SetWalletFlags(uint64_t overwriteFlags, bool memonly) {
    LOCK(cs_wallet);
    m_wallet_flags = overwriteFlags;
    if (((overwriteFlags & g_known_wallet_flags) >> 32) ^
        (overwriteFlags >> 32)) {
        // contains unknown non-tolerable wallet flags
        return false;
    }
    if (!memonly && !WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": writing wallet flags failed");
    }

    return true;
}
*/

int64_t CWalletTx::GetTxTime() const {
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

// Helper for producing a max-sized low-S signature (eg 72 bytes)
bool CWallet::DummySignInput(CTxIn &tx_in, const CTxOut &txout) const {
    // Fill in dummy signatures for fee calculation.
    const CScript &scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;

    if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata)) {
        return false;
    }

    UpdateInput(tx_in, sigdata);
    return true;
}

// Helper for producing a bunch of max-sized low-S signatures (eg 72 bytes)
bool CWallet::DummySignTx(CMutableTransaction &txNew,
                          const std::vector<CTxOut> &txouts) const {
    // Fill in dummy signatures for fee calculation.
    int nIn = 0;
    for (const auto &txout : txouts) {
        if (!DummySignInput(txNew.vin[nIn], txout)) {
            return false;
        }

        nIn++;
    }
    return true;
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx,
                                     const CWallet *wallet) {
    std::vector<CTxOut> txouts;
    // Look up the inputs.  We should have already checked that this transaction
    // IsAllFromMe(ISMINE_SPENDABLE), so every input should already be in our
    // wallet, with a valid index into the vout array, and the ability to sign.
    for (auto &input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.GetTxId());
        if (mi == wallet->mapWallet.end()) {
            return -1;
        }
        assert(input.prevout.GetN() < mi->second.tx->vout.size());
        txouts.emplace_back(mi->second.tx->vout[input.prevout.GetN()]);
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx,
                                     const CWallet *wallet,
                                     const std::vector<CTxOut> &txouts) {
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts)) {
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
        return -1;
    }
    // This is formula for BLS Txes
    // will overestimate size for EC Tx when more outputs
    int64_t bls_size = 91 * tx.vin.size() + 83 * tx.vout.size() + 110;
    // This is formula for EC Txes
    // will overestimate size for BLS Tx when more inputs
    int64_t ec_size = 148 * tx.vin.size() + 34 * tx.vout.size() + 10;
    // For now return the larger of the two, this will mean potentially
    // more fees than needed but sizes should be comparable and this
    // will prevent issues with fees being too low
    int64_t old_size = GetVirtualTransactionSize(CTransaction(txNew));
    int64_t size = std::max(bls_size,ec_size);
    LogPrintf("CalculatedMaximumSignedTxSize old %d, ec %d, bls %, final %d\n",
              old_size, ec_size, bls_size, size);
    return size;
}

int CalculateMaximumSignedInputSize(const CTxOut &txout,
                                    const CWallet *wallet) {
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout)) {
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
        return -1;
    }
    return GetVirtualTransactionInputSize(txn.vin[0]);
}

void CWalletTx::GetAmounts(std::list<COutputEntry> &listReceived,
                           std::list<COutputEntry> &listSent, Amount &nFee,
                           std::string &strSentAccount,
                           const isminefilter &filter) const {
    nFee = Amount::zero();
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    Amount nDebit = GetDebit(filter);
    // debit>0 means we signed/sent this transaction.
    if (nDebit > Amount::zero()) {
        Amount nValueOut = tx->GetValueOut();
        nFee = (nDebit - nValueOut);
    }

    // Sent/received.
    for (unsigned int i = 0; i < tx->vout.size(); ++i) {
        const CTxOut &txout = tx->vout[i];
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > Amount::zero()) {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout)) {
                continue;
            }
        } else if (!(fIsMine & filter)) {
            continue;
        }

        // In either case, we need to get the destination address.
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address) &&
            !txout.scriptPubKey.IsUnspendable()) {
            LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, "
                      "txid %s\n",
                      this->GetId().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, txout.nValue, (int)i};

        // If we are debited by the transaction, add the output as a "sent"
        // entry.
        if (nDebit > Amount::zero()) {
            listSent.push_back(output);
        }

        // If we are receiving the output, add it as a "received" entry.
        if (fIsMine & filter) {
            listReceived.push_back(output);
        }
    }
}

/**
 * Scan active chain for relevant transactions after importing keys. This should
 * be called whenever new keys are added to the wallet, with the oldest key
 * creation time.
 *
 * @return Earliest timestamp that could be successfully scanned from. Timestamp
 * returned will be higher than startTime if relevant blocks could not be read.
 */
int64_t CWallet::RescanFromTime(int64_t startTime,
                                const WalletRescanReserver &reserver,
                                bool update) {
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    CBlockIndex *startBlock = nullptr;
    {
        auto locked_chain = chain().lock();
        startBlock =
            chainActive.FindEarliestAtLeast(startTime - TIMESTAMP_WINDOW);
        LogPrintf("%s: Rescanning last %i blocks\n", __func__,
                  startBlock ? chainActive.Height() - startBlock->nHeight + 1
                             : 0);
    }

    if (startBlock) {
        const CBlockIndex *const failedBlock =
            ScanForWalletTransactions(startBlock, nullptr, reserver, update);
        if (failedBlock) {
            return failedBlock->GetBlockTimeMax() + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

/**
 * Scan the block chain (starting in pindexStart) for transactions from or to
 * us. If fUpdate is true, found transactions that already exist in the wallet
 * will be updated.
 *
 * Returns null if scan was successful. Otherwise, if a complete rescan was not
 * possible (due to pruning or corruption), returns pointer to the most recent
 * block that could not be scanned.
 *
 * If pindexStop is not a nullptr, the scan will stop at the block-index
 * defined by pindexStop
 *
 * Caller needs to make sure pindexStop (and the optional pindexStart) are on
 * the main chain after to the addition of any new keys you want to detect
 * transactions for.
 */
CBlockIndex *CWallet::ScanForWalletTransactions(
    CBlockIndex *pindexStart, CBlockIndex *pindexStop,
    const WalletRescanReserver &reserver, bool fUpdate) {
    int64_t nNow = GetTime();

    assert(reserver.isReserved());
    if (pindexStop) {
        assert(pindexStop->nHeight >= pindexStart->nHeight);
    }

    CBlockIndex *pindex = pindexStart;
    CBlockIndex *ret = nullptr;

    if (pindex) {
        LogPrintf("Rescan started from block %d...\n", pindex->nHeight);
    }

    {
        fAbortRescan = false;

        // Show rescan progress in GUI as dialog or on splashscreen, if -rescan
        // on startup.
        ShowProgress(_("Rescanning..."), 0);
        CBlockIndex *tip = nullptr;
        double progress_begin;
        double progress_end;
        {
            auto locked_chain = chain().lock();
            progress_begin =
                GuessVerificationProgress(chainParams.TxData(), pindex);
            if (pindexStop == nullptr) {
                tip = chainActive.Tip();
                progress_end =
                    GuessVerificationProgress(chainParams.TxData(), tip);
            } else {
                progress_end =
                    GuessVerificationProgress(chainParams.TxData(), pindexStop);
            }
        }
        double progress_current = progress_begin;
        while (pindex && !fAbortRescan && !ShutdownRequested()) {
            if (pindex->nHeight % 100 == 0 &&
                progress_end - progress_begin > 0.0) {
                ShowProgress(
                    _("Rescanning..."),
                    std::max(
                        1,
                        std::min(99, (int)((progress_current - progress_begin) /
                                           (progress_end - progress_begin) *
                                           100))));
            }
            if (GetTime() >= nNow + 60) {
                nNow = GetTime();
                LogPrintf("Still rescanning. At block %d. Progress=%f\n",
                          pindex->nHeight, progress_current);
            }
            if (StopDialogRequested()) AbortRescan();
          
            CBlock block;
            if (ReadBlockFromDisk(block, pindex, chainParams.GetConsensus())) {
                auto locked_chain = chain().lock();
                LOCK(cs_wallet);
                if (pindex && !chainActive.Contains(pindex)) {
                    // Abort scan if current block is no longer active, to
                    // prevent marking transactions as coming from the wrong
                    // block.
                    ret = pindex;
                    break;
                }
                for (size_t posInBlock = 0; posInBlock < block.vtx.size();
                     ++posInBlock) {
                    SyncTransaction(block.vtx[posInBlock], pindex, posInBlock,
                                    fUpdate);
                }
            } else {
                ret = pindex;
            }
            if (pindex == pindexStop) {
                break;
            }
            {
                auto locked_chain = chain().lock();
                pindex = chainActive.Next(pindex);
                progress_current =
                    GuessVerificationProgress(chainParams.TxData(), pindex);
                if (pindexStop == nullptr && tip != chainActive.Tip()) {
                    tip = chainActive.Tip();
                    // in case the tip has changed, update progress max
                    progress_end =
                        GuessVerificationProgress(chainParams.TxData(), tip);
                }
            }
        }

        if (pindex && fAbortRescan) {
            LogPrintf("Rescan aborted at block %d. Progress=%f\n",
                      pindex->nHeight, progress_current);
        } else if (pindex && ShutdownRequested()) {
            LogPrintf("Rescan interrupted by shutdown request at block %d. "
                      "Progress=%f\n",
                      pindex->nHeight, progress_current);
        }

        // Hide progress dialog in GUI.
        ShowProgress(_("Rescanning..."), 100);
    }
    return ret;
}

void CWallet::ReacceptWalletTransactions() {
    // If transactions aren't being broadcasted, don't let them into local
    // mempool either.
    if (!fBroadcastTransactions) {
        return;
    }

    auto locked_chain = chain().lock();
    LOCK(cs_wallet);
    std::map<int64_t, CWalletTx *> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion
    // order.
    for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
        const TxId &wtxid = item.first;
        CWalletTx &wtx = item.second;
        assert(wtx.GetId() == wtxid);

        int nDepth = wtx.GetDepthInMainChain(*locked_chain);

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool.
    for (std::pair<const int64_t, CWalletTx *> &item : mapSorted) {
        CWalletTx &wtx = *(item.second);
        CValidationState state;
        wtx.AcceptToMemoryPool(*locked_chain, maxTxFee, state);
    }
}

bool CWalletTx::RelayWalletTransaction(interfaces::Chain::Lock &locked_chain,
                                       CConnman *connman) {
    assert(pwallet->GetBroadcastTransactions());
    if (IsCoinBase() || isAbandoned() ||
        GetDepthInMainChain(locked_chain) != 0) {
        return false;
    }

    CValidationState state;
    // GetDepthInMainChain already catches known conflicts.
    if (InMempool() || AcceptToMemoryPool(locked_chain, maxTxFee, state)) {
        LogPrintf("Relaying wtx %s\n", GetId().ToString());
        if (connman) {
            CInv inv(MSG_TX, GetId());
            connman->ForEachNode(
                [&inv](CNode *pnode) { pnode->PushInventory(inv); });
            return true;
        }
    }

    return false;
}

std::set<TxId> CWalletTx::GetConflicts() const {
    std::set<TxId> result;
    if (pwallet != nullptr) {
        const TxId &txid = GetId();
        result = pwallet->GetConflicts(txid);
        result.erase(txid);
    }

    return result;
}

Amount CWalletTx::GetDebit(const isminefilter &filter) const {
    if (tx->vin.empty()) {
        return Amount::zero();
    }

    Amount debit = Amount::zero();
    if (filter & ISMINE_SPENDABLE) {
        if (fDebitCached) {
            debit += nDebitCached;
        } else {
            nDebitCached = pwallet->GetDebit(*tx, ISMINE_SPENDABLE);
            fDebitCached = true;
            debit += nDebitCached;
        }
    }

    if (filter & ISMINE_WATCH_ONLY) {
        if (fWatchDebitCached) {
            debit += nWatchDebitCached;
        } else {
            nWatchDebitCached = pwallet->GetDebit(*tx, ISMINE_WATCH_ONLY);
            fWatchDebitCached = true;
            debit += Amount(nWatchDebitCached);
        }
    }

    return debit;
}

Amount CWalletTx::GetCredit(interfaces::Chain::Lock &locked_chain,
                            const isminefilter &filter) const {
    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsImmatureCoinBase(locked_chain)) {
        return Amount::zero();
    }

    Amount credit = Amount::zero();
    if (filter & ISMINE_SPENDABLE) {
        // GetBalance can assume transactions in mapWallet won't change.
        if (fCreditCached) {
            credit += nCreditCached;
        } else {
            nCreditCached = pwallet->GetCredit(*tx, ISMINE_SPENDABLE);
            fCreditCached = true;
            credit += nCreditCached;
        }
    }

    if (filter & ISMINE_WATCH_ONLY) {
        if (fWatchCreditCached) {
            credit += nWatchCreditCached;
        } else {
            nWatchCreditCached = pwallet->GetCredit(*tx, ISMINE_WATCH_ONLY);
            fWatchCreditCached = true;
            credit += nWatchCreditCached;
        }
    }

    return credit;
}

Amount CWalletTx::GetImmatureCredit(interfaces::Chain::Lock &locked_chain,
                                    bool fUseCache) const {
    if (IsImmatureCoinBase(locked_chain) && IsInMainChain(locked_chain)) {
        if (fUseCache && fImmatureCreditCached) {
            return nImmatureCreditCached;
        }

        nImmatureCreditCached = pwallet->GetCredit(*tx, ISMINE_SPENDABLE);
        fImmatureCreditCached = true;
        return nImmatureCreditCached;
    }

    return Amount::zero();
}

Amount CWalletTx::GetAvailableCredit(interfaces::Chain::Lock &locked_chain,
                                     bool fUseCache) const {
    if (pwallet == nullptr) {
        return Amount::zero();
    }

    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsImmatureCoinBase(locked_chain)) {
        return Amount::zero();
    }

    if (fUseCache && fAvailableCreditCached) {
        return nAvailableCreditCached;
    }

    Amount nCredit = Amount::zero();
    const TxId &txid = GetId();
    for (uint32_t i = 0; i < tx->vout.size(); i++) {
        if (!pwallet->IsSpent(locked_chain, COutPoint(txid, i))) {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (!MoneyRange(nCredit)) {
                throw std::runtime_error(std::string(__func__) +
                                         " : value out of range");
            }
        }
    }

    nAvailableCreditCached = nCredit;
    fAvailableCreditCached = true;
    return nCredit;
}

Amount CWalletTx::GetUnvestingCredit(interfaces::Chain::Lock &locked_chain ,bool fUseCache) const {
    LOCK(cs_wallets);
    if (pwallet == nullptr) {
        return Amount::zero();
    }

    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsImmatureCoinBase(locked_chain)) {
        return Amount::zero();
    }

    // Anything below the Min Reward Balance will be counted as Unvesting
    int Height = chainActive.Height();
    const Amount minAmount = Params().GetConsensus().getMinRewardBalance(Height);

    //    if (fUseCache && fAvailableCreditCached) {        return nAvailableCreditCached;    }

    Amount nCredit = Amount::zero();
    const TxId &txid = GetId();
    for (uint32_t i = 0; i < tx->vout.size(); i++) {
        if (!pwallet->IsSpent(locked_chain, COutPoint(txid, i))) {
            const CTxOut &txout = tx->vout[i];
            Amount val = pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (val < minAmount || IsCoinBase()) {
                nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
                if (!MoneyRange(nCredit)) {
                    throw std::runtime_error(std::string(__func__) +
                                             " : value out of range");
                }
            }
        }
    }

    //    nAvailableCreditCached = nCredit;
    //    fAvailableCreditCached = true;
    return nCredit;
}

Amount
CWalletTx::GetImmatureWatchOnlyCredit(interfaces::Chain::Lock &locked_chain,
                                      const bool fUseCache) const {
    if (IsImmatureCoinBase(locked_chain) && IsInMainChain(locked_chain)) {
        if (fUseCache && fImmatureWatchCreditCached) {
            return nImmatureWatchCreditCached;
        }

        nImmatureWatchCreditCached = pwallet->GetCredit(*tx, ISMINE_WATCH_ONLY);
        fImmatureWatchCreditCached = true;
        return nImmatureWatchCreditCached;
    }

    return Amount::zero();
}

Amount CWalletTx::GetAvailableWatchOnlyCredit(interfaces::Chain::Lock &locked_chain, const bool fUseCache) const {
    if (pwallet == nullptr) {
        return Amount::zero();
    }

    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsCoinBase() && GetBlocksToMaturity(locked_chain) > 0) {
        return Amount::zero();
    }

    if (fUseCache && fAvailableWatchCreditCached) {
        return nAvailableWatchCreditCached;
    }

    Amount nCredit = Amount::zero();
    const TxId &txid = GetId();
    for (uint32_t i = 0; i < tx->vout.size(); i++) {
        if (!pwallet->IsSpent(locked_chain, COutPoint(txid, i))) {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_WATCH_ONLY);
            if (!MoneyRange(nCredit)) {
                throw std::runtime_error(std::string(__func__) +
                                         ": value out of range");
            }
        }
    }

    nAvailableWatchCreditCached = nCredit;
    fAvailableWatchCreditCached = true;
    return nCredit;
}

Amount CWalletTx::GetChange() const {
    if (fChangeCached) {
        return nChangeCached;
    }

    nChangeCached = pwallet->GetChange(*tx);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const {
    return fInMempool;
}

bool CWalletTx::IsTrusted(interfaces::Chain::Lock &locked_chain) const {
    // Temporary, for CheckFinalTx below. Removed in upcoming commit.
    LockAnnotation lock(::cs_main);

    // Quick answer in most cases
    if (!CheckFinalTx(*tx)) {
        return false;
    }

    int nDepth = GetDepthInMainChain(locked_chain);
    if (nDepth >= 1) {
        return true;
    }

    if (nDepth < 0) {
        return false;
    }

    // using wtx's cached debit
    if (!bSpendZeroConfChange || !IsFromMe(ISMINE_ALL)) {
        return false;
    }

    // Don't trust unconfirmed transactions from us unless they are in the
    // mempool.
    if (!InMempool()) {
        return false;
    }

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxIn &txin : tx->vin) {
        // Transactions not sent by us: not trusted
        const CWalletTx *parent = pwallet->GetWalletTx(txin.prevout.GetTxId());
        if (parent == nullptr) {
            return false;
        }

        const CTxOut &parentOut = parent->tx->vout[txin.prevout.GetN()];
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE) {
            return false;
        }
    }

    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx &_tx) const {
    CMutableTransaction tx1{*this->tx};
    CMutableTransaction tx2{*_tx.tx};
    for (CTxIn &in : tx1.vin) {
        in.scriptSig = CScript();
    }

    for (CTxIn &in : tx2.vin) {
        in.scriptSig = CScript();
    }

    return CTransaction(tx1) == CTransaction(tx2);
}

std::vector<uint256>
CWallet::ResendWalletTransactionsBefore(interfaces::Chain::Lock &locked_chain,
                                        int64_t nTime, CConnman *connman) {
    std::vector<uint256> result;

    LOCK(cs_wallet);

    // Sort them in chronological order
    std::multimap<unsigned int, CWalletTx *> mapSorted;
    for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
        CWalletTx &wtx = item.second;
        // Don't rebroadcast if newer than nTime:
        if (wtx.nTimeReceived > nTime) {
            continue;
        }

        mapSorted.insert(std::make_pair(wtx.nTimeReceived, &wtx));
    }

    for (std::pair<const unsigned int, CWalletTx *> &item : mapSorted) {
        CWalletTx &wtx = *item.second;
        if (wtx.RelayWalletTransaction(locked_chain, connman)) {
            result.push_back(wtx.GetId());
        }
    }

    return result;
}

void CWallet::ResendWalletTransactions(int64_t nBestBlockTime,
                                       CConnman *connman) {
    // Do this infrequently and randomly to avoid giving away that these are our
    // transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions) {
        return;
    }

    bool fFirst = (nNextResend == 0);
    nNextResend = GetTime() + GetRand(30 * 60);
    if (fFirst) {
        return;
    }

    // Only do it if there's been a new block since last time
    if (nBestBlockTime < nLastResend) {
        return;
    }

    nLastResend = GetTime();

    // Temporary. Removed in upcoming lock cleanup
    auto locked_chain = chain().assumeLocked();
    // Rebroadcast unconfirmed txes older than 5 minutes before the last block
    // was found:
    std::vector<uint256> relayed = ResendWalletTransactionsBefore(
        *locked_chain, nBestBlockTime - 5 * 60, connman);
    if (!relayed.empty()) {
        LogPrintf("%s: rebroadcast %u unconfirmed transactions\n", __func__,
                  relayed.size());
    }
}

/** @} */ // end of mapWallet

/**
 * @defgroup Actions
 *
 * @{
 */
Amount CWallet::GetBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx *pcoin = &entry.second;
        if (pcoin->IsTrusted(*locked_chain)) {
            nTotal += pcoin->GetAvailableCredit(*locked_chain, true);
        }
    }

    return nTotal;
}

Amount CWallet::GetUnconfirmedBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx *pcoin = &entry.second;
        if (!pcoin->IsTrusted(*locked_chain) &&
            pcoin->GetDepthInMainChain(*locked_chain) == 0 &&
            pcoin->InMempool()) {
            nTotal += pcoin->GetAvailableCredit(*locked_chain);
        }
    }

    return nTotal;
}

Amount CWallet::GetUnvestingBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &p : mapWallet) {
        const CWalletTx *pcoin = &p.second;
        if (pcoin->IsTrusted(*locked_chain)) {
            nTotal += pcoin->GetUnvestingCredit(*locked_chain);
        }
    }

    return nTotal;
}

Amount CWallet::GetImmatureBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx *pcoin = &entry.second;
        nTotal += pcoin->GetImmatureCredit(*locked_chain);
    }

    return nTotal;
}

Amount CWallet::GetWatchOnlyBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &p : mapWallet) {
        const CWalletTx *pcoin = &p.second;
        if (pcoin->IsTrusted(*locked_chain)) {
            nTotal += pcoin->GetAvailableWatchOnlyCredit(*locked_chain);
        }
    }

    return nTotal;
}

Amount CWallet::GetUnconfirmedWatchOnlyBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx *pcoin = &entry.second;
        if (!pcoin->IsTrusted(*locked_chain) &&
            pcoin->GetDepthInMainChain(*locked_chain) == 0 &&
            pcoin->InMempool()) {
            nTotal += pcoin->GetAvailableCredit(*locked_chain);
        }
    }

    return nTotal;
}

Amount CWallet::GetImmatureWatchOnlyBalance() const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount nTotal = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx *pcoin = &entry.second;
        nTotal += pcoin->GetImmatureWatchOnlyCredit(*locked_chain);
    }

    return nTotal;
}

// Calculate total balance in a different way from GetBalance. The biggest
// difference is that GetBalance sums up all unspent TxOuts paying to the
// wallet, while this sums up both spent and unspent TxOuts paying to the
// wallet, and then subtracts the values of TxIns spending from the wallet. This
// also has fewer restrictions on which unconfirmed transactions are considered
// trusted.
Amount CWallet::GetLegacyBalance(const isminefilter &filter, int minDepth,
                                 const std::string *account) const {
    // Temporary, for CheckFinalTx below. Removed in upcoming commit.
    LockAnnotation lock(::cs_main);
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount balance = Amount::zero();
    for (const auto &entry : mapWallet) {
        const CWalletTx &wtx = entry.second;
        const int depth = wtx.GetDepthInMainChain(*locked_chain);
        if (depth < 0 || !CheckFinalTx(*wtx.tx) ||
            wtx.IsImmatureCoinBase(*locked_chain)) {
            continue;
        }

        // Loop through tx outputs and add incoming payments. For outgoing txs,
        // treat change outputs specially, as part of the amount debited.
        Amount debit = wtx.GetDebit(filter);
        const bool outgoing = debit > Amount::zero();
        for (const CTxOut &out : wtx.tx->vout) {
            if (outgoing && IsChange(out)) {
                debit -= out.nValue;
            } else if (IsMine(out) & filter && depth >= minDepth &&
                       (!account ||
                        *account == GetLabelName(out.scriptPubKey))) {
                balance += out.nValue;
            }
        }

        // For outgoing txs, subtract amount debited.
        if (outgoing && (!account || *account == wtx.strFromAccount)) {
            balance -= debit;
        }
    }

    if (account) {
        balance += WalletBatch(*database).GetAccountCreditDebit(*account);
    }

    return balance;
}

Amount CWallet::GetAvailableBalance(const CCoinControl *coinControl) const {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    Amount balance = Amount::zero();
    std::vector<COutput> vCoins;
    AvailableCoins(*locked_chain, vCoins, true, coinControl);
    for (const COutput &out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

void CWallet::AvailableCoins(interfaces::Chain::Lock &locked_chain,
                             std::vector<COutput> &vCoins, bool fOnlySafe,
                             const CCoinControl *coinControl,
                             const Amount nMinimumAmount,
                             const Amount nMaximumAmount,
                             const Amount nMinimumSumAmount,
                             const uint64_t nMaximumCount, const int nMinDepth,
                             const int nMaxDepth) const {
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    Amount nTotal = Amount::zero();
    KeyType ktype;
    
    for (auto& it : mapWallet) {
        const TxId &wtxid = it.first;
        const CWalletTx *pcoin = &(it.second);

        if (!CheckFinalTx(*pcoin->tx)) {
            continue;
        }

        if (pcoin->IsImmatureCoinBase(locked_chain)) {
            continue;
        }

        int nDepth = pcoin->GetDepthInMainChain(locked_chain);
        if (nDepth < 0) {
            continue;
        }
      
        // Don't allow user to chain multiple unconfirmed BLS transactions
        if (nDepth == 0 && pcoin->IsBLS()) {
          continue;
        }
      
        // We should not consider coins which aren't at least in our mempool.
        // It's possible for these to be conflicted via ancestors which we may
        // never be able to detect.
        if (nDepth == 0 && !pcoin->InMempool()) {
            continue;
        }

        bool safeTx = pcoin->IsTrusted(locked_chain);

        // Bitcoin-ABC: Removed check that prevents consideration of coins from
        // transactions that are replacing other transactions. This check based
        // on pcoin->mapValue.count("replaces_txid") which was not being set
        // anywhere.

        // Similarly, we should not consider coins from transactions that have
        // been replaced. In the example above, we would want to prevent
        // creation of a transaction A' spending an output of A, because if
        // transaction B were initially confirmed, conflicting with A and A', we
        // wouldn't want to the user to create a transaction D intending to
        // replace A', but potentially resulting in a scenario where A, A', and
        // D could all be accepted (instead of just B and D, or just A and A'
        // like the user would want).

        // Bitcoin-ABC: retained this check as 'replaced_by_txid' is still set
        // in the wallet code.
        if (nDepth == 0 && pcoin->mapValue.count("replaced_by_txid")) {
            safeTx = false;
        }

        if (fOnlySafe && !safeTx) {
            continue;
        }

        if (nDepth < nMinDepth || nDepth > nMaxDepth) {
            continue;
        }

        for (uint32_t i = 0; i < pcoin->tx->vout.size(); i++) {
            if (pcoin->tx->vout[i].nValue < nMinimumAmount ||
                pcoin->tx->vout[i].nValue > nMaximumAmount) {
                continue;
            }

            const COutPoint outpoint(wtxid, i);

            if (coinControl && coinControl->HasSelected() &&
                !coinControl->fAllowOtherInputs &&
                !coinControl->IsSelected(outpoint)) {
                continue;
            }

            if (IsLockedCoin(outpoint)) {
                continue;
            }

            if (IsSpent(locked_chain, outpoint)) {
                continue;
            }

            isminetype mine = IsMine(pcoin->tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }
            
            bool fSpendableIn = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
                                (coinControl && coinControl->fAllowWatchOnly &&
                                 (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO);
            bool fSolvableIn =
                (mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) !=
                ISMINE_NO;

            ktype = (pcoin->tx->vout[i].IsBLS()) ? KeyType::BLS : KeyType::EC;            

            vCoins.emplace_back(pcoin, i, nDepth, fSpendableIn, fSolvableIn, safeTx, ktype);

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += pcoin->tx->vout[i].nValue;

                if (nTotal >= nMinimumSumAmount) {
                    return;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && vCoins.size() >= nMaximumCount) {
                return;
            }
        }
    }
}

std::map<CTxDestination, std::vector<COutput>>
CWallet::ListCoins(interfaces::Chain::Lock &locked_chain) const {
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    AvailableCoins(locked_chain, availableCoins);

    for (auto &coin : availableCoins) {
        CTxDestination address;
        if (coin.fSpendable &&
            ExtractDestination(
                FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey,
                address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<COutPoint> lockedCoins;
    ListLockedCoins(lockedCoins);
    for (const auto &output : lockedCoins) {
        auto it = mapWallet.find(output.GetTxId());
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain(locked_chain);
            if (depth >= 0 && output.GetN() < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.GetN()]) ==
                    ISMINE_SPENDABLE) {
                CTxDestination address;
                if (ExtractDestination(
                        FindNonChangeParentOutput(*it->second.tx, output.GetN())
                            .scriptPubKey,
                        address)) {
                    result[address].emplace_back(
                        &it->second, output.GetN(), depth, true /* spendable */,
                        true /* solvable */, false /* safe */);
                }
            }
        }
    }

    return result;
}

const CTxOut &CWallet::FindNonChangeParentOutput(const CTransaction &tx,
                                                 int output) const {
    const CTransaction *ptx = &tx;
    int n = output;
    while (IsChange(ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint &prevout = ptx->vin[0].prevout;
        auto it = mapWallet.find(prevout.GetTxId());
        if (it == mapWallet.end() ||
            it->second.tx->vout.size() <= prevout.GetN() ||
            !IsMine(it->second.tx->vout[prevout.GetN()])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.GetN();
    }
    return ptx->vout[n];
}

static void ApproximateBestSubset(const std::vector<CInputCoin> &vValue,
                                  const Amount &nTotalLower,
                                  const Amount &nTargetValue,
                                  std::vector<char> &vfBest, Amount &nBest,
                                  int iterations = 1000) {
    std::vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    FastRandomContext insecure_rand;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++) {
        vfIncluded.assign(vValue.size(), false);
        Amount nTotal = Amount::zero();
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++) {
            for (size_t i = 0; i < vValue.size(); i++) {
                // The solver here uses a randomized algorithm, the randomness
                // serves no real security purpose but is just needed to prevent
                // degenerate behavior and it is important that the rng is fast.
                // We do not use a constant random sequence, because there may
                // be some privacy improvement by making the selection random.
                if (nPass == 0 ? insecure_rand.randbool() : !vfIncluded[i]) {
                    nTotal += vValue[i].txout.nValue;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue) {
                        fReachedTarget = true;
                        if (nTotal < nBest) {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }

                        nTotal -= vValue[i].txout.nValue;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool CWallet::OutputEligibleForSpending(const COutput &output,
                                        const int nConfMine,
                                        const int nConfTheirs,
                                        const uint64_t nMaxAncestors) const {
    if (!output.fSpendable) {
        return false;
    }

    if (output.nDepth <
        (output.tx->IsFromMe(ISMINE_ALL) ? nConfMine : nConfTheirs)) {
        return false;
    }

    if (!g_mempool.TransactionWithinChainLimit(output.tx->GetId(),
                                               nMaxAncestors)) {
        return false;
    }

    return true;
}

bool CWallet::SelectCoinsMinConf(const Amount nTargetValue, const int nConfMine,
                                 const int nConfTheirs,
                                 const uint64_t nMaxAncestors,
                                 std::vector<COutput> vCoins,
                                 std::set<CInputCoin> &setCoinsRet,
                                 Amount &nValueRet) const {
    setCoinsRet.clear();
    nValueRet = Amount::zero();

    // List of values less than target
    std::optional<CInputCoin> coinLowestLarger;
    std::vector<CInputCoin> vValue;
    Amount nTotalLower = Amount::zero();

    Shuffle(vCoins.begin(), vCoins.end(), FastRandomContext());

    for (const COutput &output : vCoins) {
        if (!OutputEligibleForSpending(output, nConfMine, nConfTheirs,
                                       nMaxAncestors)) {
            continue;
        }

        CInputCoin coin = CInputCoin(output.tx, output.i);

        if (coin.txout.nValue == nTargetValue) {
            setCoinsRet.insert(coin);
            nValueRet += coin.txout.nValue;
            return true;
        } else if (coin.txout.nValue < nTargetValue + MIN_CHANGE) {
            vValue.push_back(coin);
            nTotalLower += coin.txout.nValue;
        } else if (!coinLowestLarger ||
                   coin.txout.nValue < coinLowestLarger->txout.nValue) {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue) {
        for (auto& i : vValue) {
            setCoinsRet.insert(i);
            nValueRet += i.txout.nValue;
        }

        return true;
    }

    if (nTotalLower < nTargetValue) {
        if (!coinLowestLarger) {
            return false;
        }
        setCoinsRet.insert(coinLowestLarger.value());
        nValueRet += coinLowestLarger->txout.nValue;
        return true;
    }

    // Solve subset sum by stochastic approximation
    std::sort(vValue.begin(), vValue.end(), CompareValueOnly());
    std::reverse(vValue.begin(), vValue.end());
    std::vector<char> vfBest;
    Amount nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + MIN_CHANGE) {
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + MIN_CHANGE,
                              vfBest, nBest);
    }

    // If we have a bigger coin and (either the stochastic approximation didn't
    // find a good solution, or the next bigger coin is closer), return the
    // bigger coin.
    if (coinLowestLarger &&
        ((nBest != nTargetValue && nBest < nTargetValue + MIN_CHANGE) ||
         coinLowestLarger->txout.nValue <= nBest)) {
        setCoinsRet.insert(coinLowestLarger.value());
        nValueRet += coinLowestLarger->txout.nValue;
    } else {
        for (unsigned int i = 0; i < vValue.size(); i++) {
            if (vfBest[i]) {
                setCoinsRet.insert(vValue[i]);
                nValueRet += vValue[i].txout.nValue;
            }
        }

        if (LogAcceptCategory(BCLog::SELECTCOINS)) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoins() best subset: ");
            for (size_t i = 0; i < vValue.size(); i++) {
                if (vfBest[i]) {
                    LogPrint(BCLog::SELECTCOINS, "%s ",
                             FormatMoney(vValue[i].txout.nValue));
                }
            }
            LogPrint(BCLog::SELECTCOINS, "total %s\n", FormatMoney(nBest));
        }
    }

    return true;
}

bool CWallet::SelectCoins(const std::vector<COutput> &vAvailableCoins,
                          const Amount nTargetValue,
                          std::set<CInputCoin> &setCoinsRet, Amount &nValueRet,
                          KeyTypes& ktype,
                          const CCoinControl *coinControl) const {
    std::vector<COutput> vCoins(vAvailableCoins);
    
    bool LegacyOnly = true;
    bool BLSOnly = true;

    // coin control -> return all selected outputs (we want all selected to go
    // into the transaction for sure).
    if (coinControl && coinControl->HasSelected() &&
        !coinControl->fAllowOtherInputs) {
        for (const COutput &out : vCoins) {
            if (!out.fSpendable) {
                continue;
            }

            nValueRet += out.tx->tx->vout[out.i].nValue;
            if (out.GetType() == KeyType::BLS) LegacyOnly = false;
            else BLSOnly = false;
            setCoinsRet.insert(CInputCoin(out.tx, out.i));
        }

        if (LegacyOnly) ktype = KeyTypes::LEGACY_ONLY;
        else if (BLSOnly) ktype = KeyTypes::BLS_ONLY;
        else ktype = KeyTypes::MIXED_COINS;
        return (nValueRet >= nTargetValue);
    }

    
    // Calculate value from preset inputs and store them.
    std::set<CInputCoin> setPresetCoins;
    Amount nValueFromPresetInputs = Amount::zero();

    std::vector<COutPoint> vPresetInputs;
    if (coinControl) {
        coinControl->ListSelected(vPresetInputs);
    }

    for (const COutPoint &outpoint : vPresetInputs) {
        auto it = mapWallet.find(outpoint.GetTxId());
        if (it == mapWallet.end()) {
            // TODO: Allow non-wallet inputs
            return false;
        }

        const CWalletTx *pcoin = &it->second;
        // Clearly invalid input, fail.
        if (pcoin->tx->vout.size() <= outpoint.GetN()) {
            return false;
        }

        nValueFromPresetInputs += pcoin->tx->vout[outpoint.GetN()].nValue;
        setPresetCoins.insert(CInputCoin(pcoin, outpoint.GetN()));
    }

    // Remove preset inputs from vCoins.
    for (std::vector<COutput>::iterator it = vCoins.begin();
         it != vCoins.end() && coinControl && coinControl->HasSelected();) {
        if (setPresetCoins.count(CInputCoin(it->tx, it->i))) {
            it = vCoins.erase(it);
        } else {
            ++it;
        }
    }

    size_t nMaxChainLength = std::min(
        gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT),
        gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));

    bool fRejectLongChains = gArgs.GetBoolArg(
        "-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res =
        nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, 0,
                           vCoins, setCoinsRet, nValueRet) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, 0,
                           vCoins, setCoinsRet, nValueRet) ||
        (bSpendZeroConfChange &&
         SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, 2,
                            vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange &&
         SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1,
                            std::min((size_t)4, nMaxChainLength / 3), vCoins,
                            setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange &&
         SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1,
                            nMaxChainLength / 2, vCoins, setCoinsRet,
                            nValueRet)) ||
        (bSpendZeroConfChange &&
         SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1,
                            nMaxChainLength, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && !fRejectLongChains &&
         SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1,
                            std::numeric_limits<uint64_t>::max(), vCoins,
                            setCoinsRet, nValueRet));

    // Because SelectCoinsMinConf clears the setCoinsRet, we now add the
    // possible inputs to the coinset.
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // Add preset inputs to the total value selected.
    nValueRet += nValueFromPresetInputs;

  
    LegacyOnly = true;
    BLSOnly = true;

    for (const auto& out : setCoinsRet) {
        if (out.txout.IsBLS()) LegacyOnly = false;
        else BLSOnly = false;
    }

    if (LegacyOnly) ktype = KeyTypes::LEGACY_ONLY;
    else if (BLSOnly) ktype = KeyTypes::BLS_ONLY;
    else ktype = KeyTypes::MIXED_COINS;
    
    return res;
}

bool CWallet::SignTransaction(CMutableTransaction &tx) {
    // sign the new tx
    CTransaction txNewConst(tx);
    int nIn = 0;
    for (auto &input : tx.vin) {
        auto mi = mapWallet.find(input.prevout.GetTxId());
        if (mi == mapWallet.end() ||
            input.prevout.GetN() >= mi->second.tx->vout.size()) {
            return false;
        }
        const CScript &scriptPubKey =
            mi->second.tx->vout[input.prevout.GetN()].scriptPubKey;
        const Amount amount = mi->second.tx->vout[input.prevout.GetN()].nValue;
        SignatureData sigdata;
        SigHashType sigHashType = SigHashType().withForkId();
        if (!ProduceSignature(TransactionSignatureCreator(
                                  this, &txNewConst, nIn, amount, sigHashType),
                              scriptPubKey, sigdata)) {
            return false;
        }
        UpdateTransaction(tx, nIn, sigdata);
        nIn++;
    }
    return true;
}

bool CWallet::FundTransaction(CMutableTransaction &tx, Amount &nFeeRet,
                              int &nChangePosInOut, std::string &strFailReason,
                              bool lockUnspents,
                              const std::set<int> &setSubtractFeeFromOutputs,
                              CCoinControl& coinControl) {
    std::vector<CRecipient> vecSend;

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut &txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue,
                                setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    coinControl.fAllowOtherInputs = true;

    for (const CTxIn &txin : tx.vin) {
        coinControl.Select(txin.prevout);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    CReserveKey reservekey(this);
    CTransactionRef tx_new;
    if (!CreateTransaction(*locked_chain, vecSend, tx_new, reservekey, nFeeRet,
                           nChangePosInOut, strFailReason, coinControl,
                           false)) {
        return false;
    }

    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut,
                       tx_new->vout[nChangePosInOut]);
        // We dont have the normal Create/Commit cycle, and dont want to
        // risk reusing change, so just remove the key from the keypool
        // here.
        reservekey.KeepKey();
    }

    // Copy output sizes from new transaction; they may have had the fee
    // subtracted from them.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }

    // Add new txins (keeping original txin scriptSig/order)
    for (const CTxIn &txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);

            if (lockUnspents) {
                LockCoin(txin.prevout);
            }
        }
    }

    return true;
}

/* later
OutputType
CWallet::TransactionChangeType(OutputType change_type,
                               const std::vector<CRecipient> &vecSend) {
    // If -changetype is specified, always use that change type.
    if (change_type != OutputType::CHANGE_AUTO) {
        return change_type;
    }

    // if m_default_address_type is legacy, use legacy address as change.
    if (m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }

    // else use m_default_address_type for change
    return m_default_address_type;
}
*/

bool CWallet::CreateTransaction(interfaces::Chain::Lock &locked_chainIn,
                                const std::vector<CRecipient> &vecSend,
                                CTransactionRef &tx, CReserveKey &reservekey,
                                Amount &nFeeRet, int &nChangePosInOut,
                                std::string &strFailReason,
                                const CCoinControl &coinControl, bool sign) {
    Amount nValue = Amount::zero();
    KeyTypes kTypes = KeyTypes::POSSIBLY_MIXED;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto &recipient : vecSend) {
        if (nValue < Amount::zero() || recipient.nAmount < Amount::zero()) {
            strFailReason = _("Transaction amounts must not be negative");
            return false;
        }

        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount) {
            nSubtractFeeFromAmount++;
        }
    }

    if (vecSend.empty()) {
        strFailReason = _("Transaction must have at least one recipient");
        return false;
    }

    CMutableTransaction txNew;

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and the
    // mempool can exceed the cost of deliberately attempting to mine two blocks
    // to orphan the current best block. By setting nLockTime such that only the
    // next block can include the transaction, we discourage this practice as
    // the height restricted and limited blocksize gives miners considering fee
    // sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this way
    // we're basically making the statement that we only want this transaction
    // to appear in the next block; we don't want to potentially encourage
    // reorgs by allowing transactions to appear at lower heights than the next
    // block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low enough,
    // that fee sniping isn't a problem yet, but by implementing a fix now we
    // ensure code won't be written that makes assumptions about nLockTime that
    // preclude a fix later.
    txNew.nLockTime = chainActive.Height();

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0) {
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));
    }

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    {
        std::set<CInputCoin> setCoins;
        // Full of possibly mixed coins
        auto locked_chain = chain().lock();
        LOCK(cs_wallet);
        std::vector<COutput> vAvailableCoins;
        AvailableCoins(*locked_chain, vAvailableCoins, true, &coinControl);
        // Parameters for coin selection, init with dummy
        //CoinSelectionParams coin_selection_params;

        // Create change script that will be used if we need change
        // TODO: pass in scriptChange instead of reservekey so
        // change transaction isn't always pay-to-bitcoin-address
        CScript scriptChange;

        // coin control: send change to custom address
        {
          try {
            std::get<CNoDestination>(coinControl.destChange);
            scriptChange = GetScriptForDestination(coinControl.destChange);
          }
          catch (std::bad_variant_access&) { LogPrintf("bad variant access"); }
            // no coin control: send change to newly generated address
            // Note: We use a new key here to keep it from being obvious
            // which side is the change.
            //  The drawback is that by not reusing a previous key, the
            //  change may be lost if a backup is restored, if the backup
            //  doesn't have the new private key for the change. If we
            //  reused the old key, it would be possible to add code to look
            //  for and rediscover unknown transactions that were written
            //  with keys of ours to recover post-backup change.

            // Reserve a new key pair from key pool
            if (!IsWalletPrivate()) {
                strFailReason =
                    _("Can't generate a change-address key. Private keys "
                      "are disabled for this wallet.");
                return false;
            }
            CPubKey vchPubKey;
            bool ret;
            ret = reservekey.GetReservedKey(vchPubKey, true);
            if (!ret) {
                strFailReason =
                    _("Keypool ran out, please call keypoolrefill first");
                return false;
            }

            if (vchPubKey.IsEC()) {
                scriptChange = GetScriptForDestination(vchPubKey.GetKeyID());
            } else {
                scriptChange = GetScriptForDestination(vchPubKey.GetBLSKeyID());
            }
        }
        CTxOut change_prototype_txout(Amount::zero(), scriptChange);
        size_t change_prototype_size =
            GetSerializeSize(change_prototype_txout);

        nFeeRet = MIN_FEE;
        bool pick_new_inputs = true;
        Amount nValueIn = Amount::zero();
        // Start with no fee and loop until there is enough fee
        while (true) {
            nChangePosInOut = nChangePosRequest;
            txNew.vin.clear();
            txNew.vout.clear();
            bool fFirst = true;

            Amount nValueToSelect = nValue;
            if (nSubtractFeeFromAmount == 0) {
                nValueToSelect += nFeeRet;
            }

            // vouts to the payees
            for (const auto &recipient : vecSend) {
                CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                if (recipient.fSubtractFeeFromAmount) {
                    assert(nSubtractFeeFromAmount != 0);
                    // Subtract fee equally from each selected recipient.
                    txout.nValue -= nFeeRet / int(nSubtractFeeFromAmount);

                    // First receiver pays the remainder not divisible by output
                    // count.
                    if (fFirst) {
                        fFirst = false;
                        txout.nValue -= nFeeRet % int(nSubtractFeeFromAmount);
                    }
                }

                if (IsDust(txout, dustRelayFee)) {
                    if (recipient.fSubtractFeeFromAmount &&
                        nFeeRet > Amount::zero()) {
                        if (txout.nValue < Amount::zero()) {
                            strFailReason = _("The transaction amount is "
                                              "too small to pay the fee");
                        } else {
                            strFailReason =
                                _("The transaction amount is too small to "
                                  "send after the fee has been deducted");
                        }
                    } else {
                        strFailReason = _("Transaction amount too small");
                    }

                    return false;
                }

                txNew.vout.push_back(txout);
            }

            // Choose coins to use
            if (pick_new_inputs) {
                nValueIn = Amount::zero();
                setCoins.clear();
                if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins,
                                 nValueIn, kTypes, &coinControl)) {
                        strFailReason = _("Insufficient funds");
                        return false;
                }
            }

            const Amount nChange = nValueIn - nValueToSelect;
            if (nChange > Amount::zero()) {
                // Fill a vout to ourself.
                CTxOut newTxOut(nChange, scriptChange);

                // Never create dust outputs; if we would, just add the dust to
                // the fee.
                if (IsDust(newTxOut, dustRelayFee)) {
                    nChangePosInOut = -1;
                    nFeeRet += nChange;
                } else {
                    if (nChangePosInOut == -1) {
                        // Insert change txn at random position:
                        nChangePosInOut = GetRandInt(txNew.vout.size() + 1);
                    } else if ((unsigned int)nChangePosInOut >
                               txNew.vout.size()) {
                        strFailReason = _("Change index out of range");
                        return false;
                    }

                    std::vector<CTxOut>::iterator position =
                        txNew.vout.begin() + nChangePosInOut;
                    txNew.vout.insert(position, newTxOut);
                }
            } else {
                nChangePosInOut = -1;
            }

            // Fill vin
            //
            // Note how the sequence number is set to non-maxint so that the
            // nLockTime set above actually works.
            for (const auto &coin : setCoins) {
                txNew.vin.emplace_back(coin.outpoint, CScript(),
                          std::numeric_limits<uint32_t>::max() - 1);
            }

            CTransaction txNewConst(txNew);
            int nBytes = CalculateMaximumSignedTxSize(txNewConst, this);
            if (nBytes < 0) {
                strFailReason = _("Signing transaction failed");
                return false;
            }

            Amount nFeeNeeded = GetMinimumFee(nBytes, coinControl, g_mempool);

            // If we made it here and we aren't even able to meet the relay fee
            // on the next pass, give up because we must be at the maximum
            // allowed fee.
            Amount minFee = GetConfig().GetMinFeePerKB().GetFee(nBytes);
            if (nFeeNeeded < minFee) {
                strFailReason = _("Transaction too large for fee policy");
                return false;
            }

            if (nFeeRet >= nFeeNeeded) {
                // Reduce fee to only the needed amount if possible. This
                // prevents potential overpayment in fees if the coins selected
                // to meet nFeeNeeded result in a transaction that requires less
                // fee than the prior iteration.

                // If we have no change and a big enough excess fee, then try to
                // construct transaction again only without picking new inputs.
                // We now know we only need the smaller fee (because of reduced
                // tx size) and so we should add a change output. Only try this
                // once.
                Amount fee_needed_for_change = GetMinimumFee(
                    change_prototype_size, coinControl, g_mempool);
                Amount minimum_value_for_change =
                    GetDustThreshold(change_prototype_txout, dustRelayFee);
                Amount max_excess_fee =
                    fee_needed_for_change + minimum_value_for_change;
                if (nFeeRet > nFeeNeeded + max_excess_fee &&
                    nChangePosInOut == -1 && nSubtractFeeFromAmount == 0 &&
                    pick_new_inputs) {
                    pick_new_inputs = false;
                    nFeeRet = nFeeNeeded + fee_needed_for_change;
                    continue;
                }

                // If we have change output already, just increase it
                if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 &&
                    nSubtractFeeFromAmount == 0) {
                    Amount extraFeePaid = nFeeRet - nFeeNeeded;
                    std::vector<CTxOut>::iterator change_position =
                        txNew.vout.begin() + nChangePosInOut;
                    change_position->nValue += extraFeePaid;
                    nFeeRet -= extraFeePaid;
                }

                // Done, enough fee included.
                break;
            } else if (!pick_new_inputs) {
                // This shouldn't happen, we should have had enough excess fee
                // to pay for the new output and still meet nFeeNeeded.
                // Or we should have just subtracted fee from recipients and
                // nFeeNeeded should not have changed.
                strFailReason =
                    _("Transaction fee and change calculation failed");
                return false;
            }

            // Try to reduce change to include necessary fee.
            if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                Amount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                std::vector<CTxOut>::iterator change_position =
                    txNew.vout.begin() + nChangePosInOut;
                // Only reduce change if remaining amount is still a large
                // enough output.
                if (change_position->nValue >=
                    MIN_FINAL_CHANGE + additionalFeeNeeded) {
                    change_position->nValue -= additionalFeeNeeded;
                    nFeeRet += additionalFeeNeeded;
                    // Done, able to increase fee from change.
                    break;
                }
            }

            // If subtracting fee from recipients, we now know what fee we
            // need to subtract, we have no reason to reselect inputs.
            if (nSubtractFeeFromAmount > 0) {
                pick_new_inputs = false;
            }

            // Include more fee and try again.
            nFeeRet = nFeeNeeded;
            continue;
        }

        if (nChangePosInOut == -1) {
            // Return any reserved key if we don't have change
            reservekey.ReturnKey();
        }

        if (sign) {

            // BLS only inputs
            if (kTypes == KeyTypes::BLS_ONLY) {
              auto strFail = CreatePrivateTxWithSig(this, setCoins, txNew);
              if (strFail) {
                strFailReason = strFail.value();
                return false;
              }
              
            } else {

              SigHashType sigHashType = SigHashType().withForkId();
              CTransaction txNewConst(txNew);
              int nIn = 0;
              
              for (const auto &coin : setCoins) {
                const CScript &scriptPubKey = coin.txout.scriptPubKey;
                SignatureData sigdata;
                
                if (!ProduceSignature(TransactionSignatureCreator(
                                                                  this, &txNewConst, nIn,
                                                                  coin.txout.nValue, sigHashType),
                                      scriptPubKey, sigdata)) {
                  strFailReason = _("Signing transaction failed");
                  return false;
                }

                UpdateTransaction(txNew, nIn, sigdata);
                nIn++;
              }
            }
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size.
        //LogPrintf("For %d inputs, Tx size for Temporary debug = %d\n",tx->vin.size(), tx->GetTotalSize());
        if (tx->GetTotalSize() >= MAX_STANDARD_TX_SIZE) {
            strFailReason = _("Transaction too large");
            return false;
        }
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains",
                         DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits.
        LockPoints lp;
        CTxMemPoolEntry entry(tx, Amount::zero(), 0, 0, 0, Amount::zero(),
                              false, 0, lp);
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors =
            gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize =
            gArgs.GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) *
            1000;
        size_t nLimitDescendants =
            gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize =
            gArgs.GetArg("-limitdescendantsize",
                         DEFAULT_DESCENDANT_SIZE_LIMIT) *
            1000;
        std::string errString;
        if (!g_mempool.CalculateMemPoolAncestors(
                entry, setAncestors, nLimitAncestors, nLimitAncestorSize,
                nLimitDescendants, nLimitDescendantSize, errString)) {
            strFailReason = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool CWallet::CommitTransaction(
    CTransactionRef tx, mapValue_t mapValue,
    std::vector<std::pair<std::string, std::string>> orderForm,
    std::string fromAccount, CReserveKey &reservekey, CConnman *connman,
    CValidationState &state) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    CWalletTx wtxNew(this, std::move(tx));
    wtxNew.mapValue = std::move(mapValue);
    wtxNew.vOrderForm = std::move(orderForm);
    wtxNew.strFromAccount = std::move(fromAccount);
    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.fFromMe = true;

    LogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString());

    // Take key pair from key pool so it won't be used again.
    reservekey.KeepKey();

    // Add tx to wallet, because if it has change it's also ours, otherwise just
    // for transaction history.
    AddToWallet(wtxNew);

    // Notify that old coins are spent.
    for (const CTxIn &txin : wtxNew.tx->vin) {
        CWalletTx &coin = mapWallet.at(txin.prevout.GetTxId());
        coin.BindWallet(this);
        NotifyTransactionChanged(this, coin.GetId(), CT_UPDATED);
    }

    // Get the inserted-CWalletTx from mapWallet so that the
    // fInMempool flag is cached properly
    CWalletTx &wtx = mapWallet.at(wtxNew.GetId());

    if (fBroadcastTransactions) {
        // Broadcast
        if (!wtx.AcceptToMemoryPool(*locked_chain, maxTxFee, state)) {
            LogPrintf("CommitTransaction(): Transaction cannot be broadcast "
                      "immediately, %s\n",
                      state.GetRejectReason());
            // TODO: if we expect the failure to be long term or permanent,
            // instead delete wtx from the wallet and return failure.
        } else {
            wtx.RelayWalletTransaction(*locked_chain, connman);
        }
    }

    return true;
}


CValidationState CWallet::CommitConsolidate(CTransactionRef tx, CConnman *connman) {
    CValidationState state;
    auto locked_chain = chain().lock();

    CWalletTx wtxNew(this, std::move(tx));
    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.fFromMe = true;

    LogPrintf("CommitConsolidate:\n%s", wtxNew.tx->ToString());

    // Add tx to wallet, because if it has change it's also ours, otherwise just
    // for transaction history.
    AddToWallet(wtxNew);

    // Notify that old coins are spent.
    for (const CTxIn &txin : wtxNew.tx->vin) {
        CWalletTx &coin = mapWallet.at(txin.prevout.GetTxId());
        coin.BindWallet(this);
        NotifyTransactionChanged(this, coin.GetId(), CT_UPDATED);
    }

    // Get the inserted-CWalletTx from mapWallet so that the fInMempool flag is cached properly
    CWalletTx &wtx = mapWallet.at(wtxNew.GetId());

    if (fBroadcastTransactions) {
        // Broadcast
        if (!wtx.AcceptToMemoryPool(*locked_chain, maxTxFee, state)) {
            LogPrintf("CommitConsolidate(): Transaction cannot be broadcast "
                      "immediately, %s\n",
                      state.GetRejectReason());
            // TODO: if we expect the failure to be long term or permanent,
            // instead delete wtx from the wallet and return failure.
        } else {
            wtx.RelayWalletTransaction(*locked_chain, connman);
        }
    }
    return state;
}


CValidationState CWallet::CommitSweep(CTransactionRef tx, CConnman *connman) {
    CValidationState state;
    auto locked_chain = chain().lock();

    CWalletTx wtxNew(this, std::move(tx));
    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.fFromMe = true;

    LogPrintf("CommitSweep:\n%s", wtxNew.tx->ToString());

    // Add tx to wallet, because if it has change it's also ours, otherwise just
    // for transaction history.
    AddToWallet(wtxNew);

    // Get the inserted-CWalletTx from mapWallet so that the fInMempool flag is cached properly
    CWalletTx &wtx = mapWallet.at(wtxNew.GetId());

    if (fBroadcastTransactions) {
        // Broadcast
        if (!wtx.AcceptToMemoryPool(*locked_chain, maxTxFee, state)) {
            LogPrintf("CommitSweep(): Transaction cannot be broadcast "
                      "immediately, %s\n",
                      state.GetRejectReason());
            // TODO: if we expect the failure to be long term or permanent,
            // instead delete wtx from the wallet and return failure.
        } else {
            wtx.RelayWalletTransaction(*locked_chain,connman);
        }
    }
    return state;
}
 
bool CWallet::ConsolidateRewards(const CTxDestination &recipient, double minPercent, Amount minAmount, std::string &message) {

    auto locked_chain = chain().lock();
    CTransactionRef tx;
    std::vector<CInputCoin> coins_to_use;
    {
      LOCK(cs_wallet);
      std::map<CTxDestination, std::vector<COutput>> listCoins = ListCoins(*locked_chain);
      coins_to_use = analyzecoins(listCoins, minPercent);
    }
    Amount nAmount(0);
    for (const auto &coin : coins_to_use) { nAmount += coin.txout.nValue; }
    // Bail out if amount is too small
    if (nAmount < minAmount) {
        message = nAmount.ToString() + " coins to move are less than minimum of " + minAmount.ToString();
        return false;
    }

    if (!ConsolidateCoins(recipient, coins_to_use, tx, message)) {
        message = strprintf("Error: ConsolidateCoins failed! Reason given: %s", message);
        return false;
    }
    
    message = tx->GetId().GetHex();
    return true;
}
 
bool CWallet::ConsolidateCoins(const CTxDestination &recipient, double minPercent,
                               CTransactionRef &tx, std::string &strFailReason) {

  auto locked_chain = chain().lock();
  std::vector<CInputCoin> coins_to_use;
  {
    LOCK(cs_wallet);
    std::map<CTxDestination, std::vector<COutput>> listCoins = ListCoins(*locked_chain);
    coins_to_use = analyzecoins(listCoins, minPercent);
  }
  return ConsolidateCoins(recipient, coins_to_use, tx, strFailReason);
}
 

bool CWallet::ConsolidateCoins(const CTxDestination &recipient,
                               const std::vector<CInputCoin> &coins_to_use,
                               CTransactionRef &tx, std::string &strFailReason) {

    Amount nAmount(0);
    for (const auto &coin : coins_to_use) { nAmount += coin.txout.nValue; }
  
    CMutableTransaction txNew;

    txNew.nLockTime = chainActive.Height();
    if (GetRandInt(10) == 0) {
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));
    }

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    LOCK2(cs_main, cs_wallet);

    // no change ....
    txNew.vin.clear();
    txNew.vout.clear();

    // vouts to the payees
    CScript scriptPub = GetScriptForDestination(recipient);
    CTxOut txout(nAmount, scriptPub);
    // add for sizing, replace later after fee subtracted
    txNew.vout.push_back(txout);

    // Note how the sequence number is set to non-maxint so that the nLockTime set above actually works.
    for (const auto &coin : coins_to_use) {
      txNew.vin.emplace_back(coin.outpoint, CScript(),
                             std::numeric_limits<uint32_t>::max() - 1);
    }

    CTransaction txNewConst(txNew);
    int nBytes = CalculateMaximumSignedTxSize(txNewConst, this);
    if (nBytes < 0) {
        strFailReason = _("Signing transaction failed");
        return false;
    }

    // for now
    Amount nFee = GetConfig().GetMinFeePerKB().GetFee(nBytes);

    txNew.vout.clear();
    txout.nValue -= nFee;
    txNew.vout.push_back(txout);
    CTransaction txRedoNewConst(txNew);

    SigHashType sigHashType = SigHashType().withForkId();
    int nIn = 0;
    for (const auto &coin : coins_to_use) {
      const CScript &scriptPubKey = coin.txout.scriptPubKey;
      SignatureData sigdata;

      if (!ProduceSignature(
                            TransactionSignatureCreator(this, &txRedoNewConst, nIn,
                                                        coin.txout.nValue, sigHashType),
                            scriptPubKey, sigdata)) {
        strFailReason = _("Signing transaction failed");
        return false;
      }
            
      UpdateTransaction(txNew, nIn, sigdata);
      nIn++;
    }
    
    // Return the constructed transaction data.
    tx = MakeTransactionRef(std::move(txNew));

    // Limit size.
    if (tx->GetTotalSize() >= MAX_STANDARD_TX_SIZE) {
        strFailReason = _("Transaction too large");
        return false;
    }

    
    CValidationState state = CommitConsolidate(tx, g_connman.get());
    return true;
}
 
bool CWallet::SweepCoinsToWallet(const CKey& key,
                                 CTransactionRef &tx,
                                 bool from_bls,
                                 std::string &strFailReason) {

    // 1. Get all the UTXOs for the Key/Address
    // 2. Get a (new) key from wallet to send to
    // 3. Create transaction with the above info
    // 4. Calculate fee & sign transaction using temporary keystore
    // 5. Call CommitSweep to add to Memory pool/Relay
    // 6. Notify

    CTxDestination source;
    if (from_bls) {
        source = key.GetPubKey().GetBLSKeyID();
    } else {
        source = key.GetPubKey().GetKeyID();
    }
    CScript coinscript =  GetScriptForDestination(source);

    if (::IsMine(*this, coinscript)) {
        strFailReason = _("Source address already contained in this wallet, can not sweep");
        return false;
    }

    // Get the list of UTXOs for the coin
    std::map<COutPoint, Coin> coins_to_use = GetUTXOSet(pcoinsdbview.get(), source);

    if (coins_to_use.size() == 0) {
        strFailReason = _("Error: No UTXOs at this address");
        return false;
    }
    
    Amount nAmount(0);
    for (const auto &coin : coins_to_use) {
        nAmount += coin.second.GetTxOut().nValue;
    }
    
    CMutableTransaction txNew;

    txNew.nLockTime = chainActive.Height();
    if (GetRandInt(10) == 0) {
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));
    }

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    LOCK2(cs_main, cs_wallet);

    // no change ....
    txNew.vin.clear();
    txNew.vout.clear();

    // Generate a new key that is added to wallet
    if (!IsLocked()) {
        TopUpKeyPool();
    }

    CPubKey newKey;
    if (!GetKeyFromPool(newKey, false)) {
        strFailReason = _("Error: Keypool ran out, please call keypoolrefill first");
        return false;
    }

    // vouts to the payees
    CTxDestination recipient;
    if (UseBLSKeys()) {
        recipient = newKey.GetBLSKeyID();
    } else {
        recipient = newKey.GetKeyID();
    }
    CScript scriptPub = GetScriptForDestination(recipient);
    CTxOut txout(nAmount, scriptPub);
    // add for sizing, replace later after fee subtracted
    txNew.vout.push_back(txout);

    // Note how the sequence number is set to non-maxint so that the nLockTime set above actually works.
    for (const auto &coin : coins_to_use) {
      txNew.vin.emplace_back(coin.first, CScript(),
                             std::numeric_limits<uint32_t>::max() - 1);
    }

    CTransaction txNewConst(txNew);

    int nBytes;
    {
      std::vector<CTxOut> vtx;
      // resize to same size at vins since seems to be required for
      // Calc function below to work
      for (size_t i=0;i<coins_to_use.size();i++) vtx.push_back(txout);
      nBytes = CalculateMaximumSignedTxSize(txNewConst, this, vtx);
      if (nBytes < 0) {
        strFailReason = _("Signing transaction failed");
        return false;
      }
    }

    // for now
    Amount nFee = GetConfig().GetMinFeePerKB().GetFee(nBytes);

    txNew.vout.clear();
    txout.nValue -= nFee;
    txNew.vout.push_back(txout);
    CTransaction txRedoNewConst(txNew);
    Amount amount;

    SigHashType sigHashType = SigHashType().withForkId();
    int nIn = 0;

    if (UseBLSKeys()) {

      // BLS only inputs
      auto strFail = CreatePrivateTxWithSig(this, txNew);
      if (strFail) {
        strFailReason = strFail.value();
        return false;
      }
  
    } else {
      // We create a temporary keystore and then add this key to it for signing purposes
      CBasicKeyStore keystore;
      keystore.AddKey(key);
      

      for (const auto &coin : coins_to_use) {
        const CScript &scriptPubKey = coin.second.GetTxOut().scriptPubKey;
        SignatureData sigdata;
        amount = coin.second.GetTxOut().nValue;
        ProduceSignature(MutableTransactionSignatureCreator(
                                                            &keystore, &txNew, nIn, amount, sigHashType),
                         scriptPubKey, sigdata);
        
        sigdata = CombineSignatures(
                                    scriptPubKey, TransactionSignatureChecker(&txRedoNewConst, nIn, amount),
                                    sigdata, DataFromTransaction(txNew, nIn));

        UpdateTransaction(txNew, nIn, sigdata);
        nIn++;
      }

    }
    
    // Return the constructed transaction data.
    tx = MakeTransactionRef(std::move(txNew));

    // Limit size.
    if (tx->GetTotalSize() >= MAX_STANDARD_TX_SIZE) {
        strFailReason = _("Transaction too large");
        return false;
    }

    CValidationState state = CommitSweep(tx, g_connman.get());

    // Notify that coin is spent (just 1 address)
    NotifyTransactionChanged(this, tx->GetId(), CT_UPDATED);

    return true;
}
 
void CWallet::ListAccountCreditDebit(const std::string &strAccount,
                                     std::list<CAccountingEntry> &entries) {
    WalletBatch batch(*database);
    return batch.ListAccountCreditDebit(strAccount, entries);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry &acentry) {
    WalletBatch batch(*database);
    return AddAccountingEntry(acentry, &batch);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry &acentry,
                                 WalletBatch *pbatch) {
    if (!pbatch->WriteAccountingEntry(++nAccountingEntryNumber, acentry)) {
        return false;
    }

    laccentries.push_back(acentry);
    CAccountingEntry &entry = laccentries.back();
    wtxOrdered.insert(std::make_pair(entry.nOrderPos, TxPair(nullptr, &entry)));

    return true;
}

DBErrors CWallet::LoadWallet(bool &fFirstRunRet) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database, "cr+").LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            m_pool_blskey_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    {
        LOCK(cs_KeyStore);
        // This wallet is in its first run if all of these are empty
        fFirstRunRet = mapKeys.empty() && mapScripts.empty() &&
            mapWatchKeys.empty() && setWatchOnly.empty() &&
            mapHdPubKeys.empty() && mapBLSPubKeys.empty() &&
            mapKeyMetadata.empty() && IsWalletPrivate() && !IsWalletBlank();
    }
        
    if (nLoadWalletRet != DBErrors::LOAD_OK) {
        return nLoadWalletRet;
    }

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapSelectTx(std::vector<TxId> &txIdsIn,
                              std::vector<TxId> &txIdsOut) {
    AssertLockHeld(cs_wallet); // mapWallet
    DBErrors nZapSelectTxRet =
        WalletBatch(*database, "cr+").ZapSelectTx(txIdsIn, txIdsOut);
    for (const TxId &txid : txIdsOut) {
        const auto &it = mapWallet.find(txid);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        mapWallet.erase(it);
    }

    if (nZapSelectTxRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            m_pool_blskey_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapSelectTxRet != DBErrors::LOAD_OK) {
        return nZapSelectTxRet;
    }

    MarkDirty();

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx> &vWtx) {
    DBErrors nZapWalletTxRet = WalletBatch(*database, "cr+").ZapWalletTx(vWtx);
    if (nZapWalletTxRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            LOCK(cs_wallet);
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            m_pool_blskey_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DBErrors::LOAD_OK) {
        return nZapWalletTxRet;
    }

    return DBErrors::LOAD_OK;
}

bool CWallet::SetAddressBook(const CTxDestination &address,
                             const std::string &strName,
                             const std::string &strPurpose) {
    bool fUpdated = false;
    {
        // mapAddressBook
        LOCK(cs_wallet);
        auto mi =  mapAddressBook.find(address);
        fUpdated = mi != mapAddressBook.end();
        mapAddressBook[address].name = strName;
        // Update purpose only if requested.
        if (!strPurpose.empty()) {
            mapAddressBook[address].purpose = strPurpose;
        }
    }

    NotifyAddressBookChanged(this, address, strName,
                             ::IsMine(*this, address) != ISMINE_NO, strPurpose,
                             (fUpdated ? CT_UPDATED : CT_NEW));

    if (!strPurpose.empty() &&
        !WalletBatch(*database).WritePurpose(address, strPurpose)) {
        return false;
    }

    return WalletBatch(*database).WriteNameAndLabel(address, strName);
}

bool CWallet::SetLabel(const CTxDestination &address,
                       const std::string &strName,
                       const std::string &strPurpose) {

    if (!strPurpose.empty() &&
        !WalletBatch(*database).WritePurpose(address, strPurpose)) {
        return false;
    }

    return WalletBatch(*database).WriteNameAndLabel(address, strName);
}

bool CWallet::DelAddressBook(const CTxDestination &address) {
    {
        // mapAddressBook
        LOCK(cs_wallet);

        // Delete destdata tuples associated with address.
        for (const auto &item : mapAddressBook[address].destdata) {
            WalletBatch(*database).EraseDestData(address, item.first);
        }

        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "",
                             ::IsMine(*this, address) != ISMINE_NO, "",
                             CT_DELETED);

    WalletBatch(*database).ErasePurpose(address);
    return WalletBatch(*database).EraseName(address);
}

const std::string &CWallet::GetLabelName(const CScript &scriptPubKey) const {
    CTxDestination address;
    if (ExtractDestination(scriptPubKey, address) &&
        !scriptPubKey.IsUnspendable()) {
        auto mi = mapAddressBook.find(address);
        if (mi != mapAddressBook.end()) {
            return mi->second.name;
        }
    }
    // A scriptPubKey that doesn't have an entry in the address book is
    // associated with the default label ("").
    const static std::string DEFAULT_LABEL_NAME;
    return DEFAULT_LABEL_NAME;
}

/**
 * Mark old keypool keys as used, and generate all new keys.
 */
bool CWallet::NewKeyPool() {
    if (!IsWalletPrivate()) {
        return false;
    }
    LOCK(cs_wallet);
    WalletBatch batch(*database);

    for (int64_t nIndex : setInternalKeyPool) {
        batch.ErasePool(nIndex);
    }
    setInternalKeyPool.clear();

    for (int64_t nIndex : setExternalKeyPool) {
        batch.ErasePool(nIndex);
    }
    setExternalKeyPool.clear();

    m_pool_key_to_index.clear();
    m_pool_blskey_to_index.clear();

    if (!TopUpKeyPool()) {
        return false;
    }

    LogPrintf("CWallet::NewKeyPool rewrote keypool\n");
    return true;
}

size_t CWallet::KeypoolCountExternalKeys() {
    // setExternalKeyPool
    AssertLockHeld(cs_wallet);
    return setExternalKeyPool.size();
}

static void LoadReserveKeysToSet(std::set<CKeyID>& setAddress, const std::set<int64_t>& setKeyPool, WalletBatch& batch)
{
  for (const int64_t& id : setKeyPool)
    {
        CKeyPool keypool;
        if (!batch.ReadPool(id, keypool))
            throw std::runtime_error(std::string(__func__) + ": read failed");
        assert(keypool.vchPubKey.IsValid());
        if (keypool.vchPubKey.IsEC()) {
            CKeyID keyID = keypool.vchPubKey.GetKeyID();
            setAddress.insert(keyID);
        }
    }
}
static void LoadReserveKeysToSet(std::set<BKeyID>& setAddress, const std::set<int64_t>& setKeyPool, WalletBatch& batch)
    {
      for (const int64_t& id : setKeyPool)
        {
            CKeyPool keypool;
            if (!batch.ReadPool(id, keypool))
                throw std::runtime_error(std::string(__func__) + ": read failed");
            assert(keypool.vchPubKey.IsValid());
            if (keypool.vchPubKey.IsBLS()) {
                BKeyID keyID = keypool.vchPubKey.GetBLSKeyID();
                setAddress.insert(keyID);
            }
        }
    }

void CWallet::GetAllReserveKeys(std::set<CKeyID>& setAddress) const
{
    setAddress.clear();

    WalletBatch batch(*database);

    LOCK2(cs_main, cs_wallet);
    LoadReserveKeysToSet(setAddress, setInternalKeyPool, batch);
    LoadReserveKeysToSet(setAddress, setExternalKeyPool, batch);

    for (const CKeyID& keyID : setAddress) {
      if (!HaveKey(keyID)) {
        throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
      }
    }
}
    
void CWallet::GetAllBLSReserveKeys(std::set<BKeyID>& setAddress) const
    {
        setAddress.clear();

        WalletBatch batch(*database);

        LOCK2(cs_main, cs_wallet);
        LoadReserveKeysToSet(setAddress, setInternalKeyPool, batch);
        LoadReserveKeysToSet(setAddress, setExternalKeyPool, batch);

        for (const BKeyID& keyID : setAddress) {
          if (!HaveKey(keyID)) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
          }
        }
    }


void CWallet::LoadKeyPool(int64_t nIndex, const CKeyPool &keypool) {
    AssertLockHeld(cs_wallet);
    if (keypool.fInternal) {
        setInternalKeyPool.insert(nIndex);
    } else {
        setExternalKeyPool.insert(nIndex);
    }
    m_max_keypool_index = std::max(m_max_keypool_index, nIndex);

    // If no metadata exists yet, create a default with the pool key's
    // creation time. Note that this may be overwritten by actually
    // stored metadata for that key later, which is fine.
    if (keypool.vchPubKey.IsEC()) {
        m_pool_key_to_index[keypool.vchPubKey.GetKeyID()] = nIndex;
        CKeyID keyid = keypool.vchPubKey.GetKeyID();
        if (mapKeyMetadata.count(keyid) == 0) {
            mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
        }
    } else {
        m_pool_blskey_to_index[keypool.vchPubKey.GetBLSKeyID()] = nIndex;
        BKeyID blskeyid = keypool.vchPubKey.GetBLSKeyID();
        if (mapBLSKeyMetadata.count(blskeyid) == 0) {
            mapBLSKeyMetadata[blskeyid] = CKeyMetadata(keypool.nTime);
        }
    }
}

bool CWallet::TopUpKeyPool(unsigned int kpSize) {
    if (!IsWalletPrivate()) {
        return false;
    }
    LOCK(cs_wallet);

    if (IsLocked() || !CanGenerateKeys()) {
        return false;
    }

    // Top up key pool
    unsigned int nTargetSize;
    if (kpSize > 0) {
        nTargetSize = kpSize;
    } else {
        nTargetSize = std::max<int64_t>(
            gArgs.GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), 0);
    }

    // count amount of available keys (internal, external)
    // make sure the keypool of external and internal keys fits the user
    // selected target (-keypool)
    int64_t missingExternal = std::max<int64_t>(std::max<int64_t>(nTargetSize, 1) - setExternalKeyPool.size(), 0);
    int64_t missingInternal = std::max<int64_t>(std::max<int64_t>(nTargetSize, 1) - setInternalKeyPool.size(), 0);

    // Will generate keys and add to these vectors, when done, flush them to the dB in burst mode
    std::vector<CHDPubKey> hdpubkeys;
    std::vector<CKeyPool> pubkeys;
  
    bool internal = false;
    int64_t saved_keypool_index = m_max_keypool_index;

    auto [hdChainDec, hdChainEnc] = GetHDChains();

    Benchmark timer;
    
    int count=1;
    for (int64_t i = missingInternal + missingExternal; i--;) {
        interruption_point(ShutdownRequested());
        if (i < missingInternal) {
            internal = true;
        }

        // How in the hell did you use so many keys?
        assert(m_max_keypool_index < std::numeric_limits<int64_t>::max());
        int64_t index = ++m_max_keypool_index;

        timer.start();

        auto key_tuple = GenerateNewKey(hdChainDec, internal);
      
        CPubKey pubkey(std::get<0>(key_tuple));
        hdpubkeys.push_back(std::get<1>(key_tuple));
        pubkeys.push_back(CKeyPool(pubkey, internal));
      
        if (internal) {
            setInternalKeyPool.insert(index);
        } else {
            setExternalKeyPool.insert(index);
        }
        if (pubkey.IsBLS()) {
            m_pool_blskey_to_index[pubkey.GetBLSKeyID()] = index;
        } else {
            m_pool_key_to_index[pubkey.GetKeyID()] = index;
        }
        
        timer.stop();
        double dProgress = (100.f * count)/ (missingExternal + missingInternal);
        std::string strMsg = strprintf(_("Adding keys... (%3.2f %%, %d us)"), dProgress, timer.uduration());
        if (count%10 == 0) uiInterface.InitMessage(strMsg);
        count++;
    }

    if (missingInternal + missingExternal > 0) {
        LogPrintf(
            "keypool added %d keys (%d internal), size=%u (%u internal)\n",
            missingInternal + missingExternal, missingInternal,
            setInternalKeyPool.size() + setExternalKeyPool.size(),
            setInternalKeyPool.size());
    }
  
  
    if (hdpubkeys.size() > 0) {

      CHDAccount acc;
      int nAccountIndex = 0; // Only using acc 0
      if (UseBLSKeys()) nAccountIndex = BLS_ACCOUNT;

      // Get updated Account info and set for Encrypted Chain
      hdChainDec.GetAccount(nAccountIndex, acc);
      hdChainEnc.SetAccount(nAccountIndex, acc);
      
      // Save to crypter
      if (!SetCryptedHDChain(hdChainEnc)) {
        throw std::runtime_error(std::string(__func__) + ": SetCryptedHDChain failed");
      }
 
      // Grab updated hdChain from CCryptoStore and store to dB
      if (!StoreCryptedHDChain()) {
        throw std::runtime_error(std::string(__func__) + ": StoreCryptedHDChain failed");
      }
      
      LogPrintf("%s : flushing %d keys to dB\n",__func__,hdpubkeys.size());
      WalletBatch batch(*database);
      batch.WriteHDPubKeys(hdpubkeys, mapKeyMetadata, mapBLSKeyMetadata);
      batch.WritePool(pubkeys, saved_keypool_index);
    }
    return true;
}

bool CWallet::ReserveKeyFromKeyPool(int64_t &nIndex, CKeyPool &keypool,
                                    bool fRequestedInternal) {
    nIndex = -1;
    keypool.vchPubKey = CPubKey();

    LOCK(cs_wallet);

    if (!IsLocked()) {
        TopUpKeyPool();
    }
    bool fReturningInternal = fRequestedInternal;
    std::set<int64_t> &setKeyPool =  (fReturningInternal) ? setInternalKeyPool : setExternalKeyPool;

    // Get the oldest key
    if (setKeyPool.empty()) {
        return false;
    }

    WalletBatch batch(*database);

    auto it = setKeyPool.begin();
    nIndex = *it;
    setKeyPool.erase(it);
    if (!batch.ReadPool(nIndex, keypool)) {
        throw std::runtime_error(std::string(__func__) + ": read failed");
    }
    if (UseBLSKeys()) {
      if (!HaveKey(keypool.vchPubKey.GetBLSKeyID())) {
        throw std::runtime_error(std::string(__func__) + ": unknown BLS key in key pool");
      }
    } else {
      if (!HaveKey(keypool.vchPubKey.GetKeyID())) {
        throw std::runtime_error(std::string(__func__) + ": unknown EC key in key pool");
      }
    }
    
    assert(keypool.vchPubKey.IsValid());
    if (keypool.vchPubKey.IsBLS()) {
        m_pool_blskey_to_index.erase(keypool.vchPubKey.GetBLSKeyID());
    } else {
        m_pool_key_to_index.erase(keypool.vchPubKey.GetKeyID());
    }
    // If the key was pre-split keypool, we don't care about what type it is
    /*
    if (use_split_keypool && keypool.fInternal != fReturningInternal) {
        throw std::runtime_error(std::string(__func__) +
                                 ": keypool entry misclassified");
    }
    */
    if (!keypool.vchPubKey.IsValid()) {
        throw std::runtime_error(std::string(__func__) +
                                 ": keypool entry invalid");
    }

    LogPrintf("keypool reserve %d\n", nIndex);

    return true;
}

void CWallet::KeepKey(int64_t nIndex) {
    // Remove from key pool.
    WalletBatch batch(*database);
    batch.ErasePool(nIndex);
    LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex, bool fInternal, const CPubKey &pubkey) {
    // Return to key pool
    {
        LOCK(cs_wallet);
        if (fInternal) {
            setInternalKeyPool.insert(nIndex);
        } else {
            setExternalKeyPool.insert(nIndex);
        }
        if (pubkey.IsEC())
            m_pool_key_to_index[pubkey.GetKeyID()] = nIndex;
        else
            m_pool_blskey_to_index[pubkey.GetBLSKeyID()] = nIndex;
    }

    LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey &result, bool internal) {
    if (!CanGetAddresses(internal)) {
        return false;
    }

    CKeyPool keypool;
    LOCK(cs_wallet);
    int64_t nIndex;
    if (!ReserveKeyFromKeyPool(nIndex, keypool, internal)) {
        if (IsLocked()) {
            return false;
        }
        WalletBatch batch(*database);
        int saved_keypool_index = m_max_keypool_index++;

        auto [hdChainDec, hdChainEnc] = GetHDChains();
        
        auto key_tuple = GenerateNewKey(hdChainDec, internal);
        result = std::get<0>(key_tuple);
        // Will generate keys and add to these vectors, when done, flush them to the dB in burst mode
        std::vector<CHDPubKey> hdpubkeys;
        std::vector<CKeyPool> pubkeys;
      
        hdpubkeys.push_back(std::get<1>(key_tuple));
        pubkeys.push_back(CKeyPool(result, internal));

        CHDAccount acc;
        int nAccountIndex = 0; // Only using acc 0
        if (UseBLSKeys()) nAccountIndex = BLS_ACCOUNT;
        
        // Get updated Account info and set for Encrypted Chain
        hdChainDec.GetAccount(nAccountIndex, acc);
        hdChainEnc.SetAccount(nAccountIndex, acc);
        
        // Save to CCryptoStore
        if (!SetCryptedHDChain(hdChainEnc)) {
          throw std::runtime_error(std::string(__func__) + ": SetCryptedHDChain failed");
        }
 
        // Grab updated hdChain from CCryptoStore and store to dB
        if (!StoreCryptedHDChain()) {
          throw std::runtime_error(std::string(__func__) + ": StoreCryptedHDChain failed");
        }
      
        batch.WriteHDPubKeys(hdpubkeys, mapKeyMetadata, mapBLSKeyMetadata);
        batch.WritePool(pubkeys, saved_keypool_index);
        return true;
    }

    KeepKey(nIndex);
    result = keypool.vchPubKey;

    return true;
}

static int64_t GetOldestKeyTimeInPool(const std::set<int64_t> &setKeyPool,
                                      WalletBatch &batch) {
    if (setKeyPool.empty()) {
        return GetTime();
    }

    CKeyPool keypool;
    int64_t nIndex = *(setKeyPool.begin());
    if (!batch.ReadPool(nIndex, keypool)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": read oldest key in keypool failed");
    }

    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

int64_t CWallet::GetOldestKeyPoolTime() {
    LOCK(cs_wallet);

    WalletBatch batch(*database);

    // load oldest key from keypool, get time and return
    int64_t oldestKey = GetOldestKeyTimeInPool(setExternalKeyPool, batch);
    oldestKey = std::max(GetOldestKeyTimeInPool(setInternalKeyPool, batch), oldestKey);

    return oldestKey;
}

std::map<CTxDestination, Amount>
CWallet::GetAddressBalances(interfaces::Chain::Lock &locked_chain) {
    std::map<CTxDestination, Amount> balances;

    LOCK(cs_wallet);
    for (const auto &walletEntry : mapWallet) {
        const CWalletTx *pcoin = &walletEntry.second;

        if (!pcoin->IsTrusted(locked_chain)) {
            continue;
        }

        if (pcoin->IsImmatureCoinBase(locked_chain)) {
            continue;
        }

        int nDepth = pcoin->GetDepthInMainChain(locked_chain);
        if (nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? 0 : 1)) {
            continue;
        }

        for (uint32_t i = 0; i < pcoin->tx->vout.size(); i++) {
            CTxDestination addr;
            if (!IsMine(pcoin->tx->vout[i])) {
                continue;
            }

            if (!ExtractDestination(pcoin->tx->vout[i].scriptPubKey, addr)) {
                continue;
            }

            Amount n = IsSpent(locked_chain, COutPoint(walletEntry.first, i))
                           ? Amount::zero()
                           : pcoin->tx->vout[i].nValue;

            if (!balances.count(addr)) {
                balances[addr] = Amount::zero();
            }
            balances[addr] += n;
        }
    }

    return balances;
}

std::set<std::set<CTxDestination>> CWallet::GetAddressGroupings() {
    // mapWallet
    AssertLockHeld(cs_wallet);
    std::set<std::set<CTxDestination>> groupings;
    std::set<CTxDestination> grouping;

    for (const auto &walletEntry : mapWallet) {
        const CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->tx->vin.size() > 0) {
            bool any_mine = false;
            // Group all input addresses with each other.
            for (const auto& txin : pcoin->tx->vin) {
                CTxDestination address;
                // If this input isn't mine, ignore it.
                if (!IsMine(txin)) {
                    continue;
                }

                if (!ExtractDestination(mapWallet.at(txin.prevout.GetTxId())
                                            .tx->vout[txin.prevout.GetN()]
                                            .scriptPubKey,
                                        address)) {
                    continue;
                }

                grouping.insert(address);
                any_mine = true;
            }

            // Group change with input addresses.
            if (any_mine) {
                for (const auto& txout : pcoin->tx->vout) {
                    if (IsChange(txout)) {
                        CTxDestination txoutAddr;
                        if (!ExtractDestination(txout.scriptPubKey,
                                                txoutAddr)) {
                            continue;
                        }

                        grouping.insert(txoutAddr);
                    }
                }
            }

            if (grouping.size() > 0) {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // Group lone addrs by themselves.
        for (const auto& i : pcoin->tx->vout) {
            if (IsMine(i)) {
                CTxDestination address;
                if (!ExtractDestination(i.scriptPubKey,address)) {
                    continue;
                }
                
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
        }
    }

    // A set of pointers to groups of addresses.
    std::set<std::set<CTxDestination> *> uniqueGroupings;
    // Map addresses to the unique group containing it.
    std::map<CTxDestination, std::set<CTxDestination> *> setmap;
    for (std::set<CTxDestination> _grouping : groupings) {
        // Make a set of all the groups hit by this new group.
        std::set<std::set<CTxDestination> *> hits;
        std::map<CTxDestination, std::set<CTxDestination> *>::iterator it;
        for (CTxDestination address : _grouping) {
            if ((it = setmap.find(address)) != setmap.end()) {
                hits.insert((*it).second);
            }
        }

        // Merge all hit groups into a new single group and delete old groups.
        std::set<CTxDestination> *merged =
            new std::set<CTxDestination>(_grouping);
        for (std::set<CTxDestination> *hit : hits) {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // Update setmap.
        for (CTxDestination element : *merged) {
            setmap[element] = merged;
        }
    }

    std::set<std::set<CTxDestination>> ret;
    for (std::set<CTxDestination> *uniqueGrouping : uniqueGroupings) {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

std::set<CTxDestination>
CWallet::GetLabelAddresses(const std::string &label) const {
    LOCK(cs_wallet);
    std::set<CTxDestination> result;
    for (const auto &item : mapAddressBook) {
        const CTxDestination &address = item.first;
        const std::string &strName = item.second.name;
        if (strName == label) {
            result.insert(address);
        }
    }

    return result;
}

void CWallet::DeleteLabel(const std::string &label) {
    WalletBatch batch(*database);
    batch.EraseAccount(label);
}

bool CReserveKey::GetReservedKey(CPubKey &pubkey, bool internal) {
    if (!pwallet->CanGetAddresses(internal)) {
        return false;
    }

    if (nIndex == -1) {
        CKeyPool keypool;
        if (!pwallet->ReserveKeyFromKeyPool(nIndex, keypool, internal)) {
            return false;
        }

        vchPubKey = keypool.vchPubKey;
        fInternal = keypool.fInternal;
    }

    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey() {
    if (nIndex != -1) {
        pwallet->KeepKey(nIndex);
    }

    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey() {
    if (nIndex != -1) {
        pwallet->ReturnKey(nIndex, fInternal, vchPubKey);
    }
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::MarkReserveKeysAsUsed(int64_t keypool_id) {
    AssertLockHeld(cs_wallet);
    bool internal = setInternalKeyPool.count(keypool_id);
    if (!internal) {
        assert(setExternalKeyPool.count(keypool_id));
    }

    std::set<int64_t> *setKeyPool =
        internal ? &setInternalKeyPool : &setExternalKeyPool;
    auto it = setKeyPool->begin();

    WalletBatch batch(*database);
    while (it != std::end(*setKeyPool)) {
        const int64_t &index = *(it);
        if (index > keypool_id) {
            // set*KeyPool is ordered
            break;
        }

        CKeyPool keypool;
        if (batch.ReadPool(index, keypool)) {
            // TODO: This should be unnecessary
            if (keypool.vchPubKey.IsEC())
                m_pool_key_to_index.erase(keypool.vchPubKey.GetKeyID());
            else
                m_pool_blskey_to_index.erase(keypool.vchPubKey.GetBLSKeyID());
        }
        batch.ErasePool(index);
        it = setKeyPool->erase(it);
    }
}

void CWallet::GetScriptForMining(std::shared_ptr<CReserveScript> &script) {
    std::shared_ptr<CReserveKey> rKey = std::make_shared<CReserveKey>(this);
    CPubKey pubkey;
    if (!rKey->GetReservedKey(pubkey)) {
        return;
    }

    script = rKey;
    script->reserveScript = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
}

CScript CWallet::GetScriptForMining(CPubKey& pubkey) {
      std::vector<uint8_t> vchHash(20);
      CHash160()
        .Write(pubkey.begin(), pubkey.size())
        .Finalize(vchHash.data());

  CScript script;
  script = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
  return script;
}



void CWallet::LockCoin(const COutPoint &output) {
    // setLockedCoins
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const COutPoint &output) {
    // setLockedCoins
    AssertLockHeld(cs_wallet);
    setLockedCoins.erase(output);
}

void CWallet::UnlockAllCoins() {
    // setLockedCoins
    AssertLockHeld(cs_wallet);
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(const COutPoint &outpoint) const {
    // setLockedCoins
    AssertLockHeld(cs_wallet);

    return setLockedCoins.count(outpoint) > 0;
}

void CWallet::ListLockedCoins(std::vector<COutPoint> &vOutpts) const {
    // setLockedCoins
    AssertLockHeld(cs_wallet);
    for (COutPoint outpoint : setLockedCoins) {
        vOutpts.push_back(outpoint);
    }
}

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(
    interfaces::Chain::Lock &locked_chain,
    std::map<CTxDestination, int64_t> &mapKeyBirth) const {
    // mapKeyMetadata
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    // Get birth times for keys with metadata.
    for (const auto &entry : mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }
    return;
}
void CWallet::GetBLSKeyBirthTimes(
    interfaces::Chain::Lock &locked_chain,
    std::map<CTxDestination, int64_t> &mapKeyBirth) const {
    // mapKeyMetadata
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    // Get birth times for keys with metadata.
    for (const auto &entry : mapBLSKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }
    return;
}

/**
 * Compute smart timestamp for a transaction being added to the wallet.
 *
 * Logic:
 * - If sending a transaction, assign its timestamp to the current time.
 * - If receiving a transaction outside a block, assign its timestamp to the
 *   current time.
 * - If receiving a block with a future timestamp, assign all its (not already
 *   known) transactions' timestamps to the current time.
 * - If receiving a block with a past timestamp, before the most recent known
 *   transaction (that we care about), assign all its (not already known)
 *   transactions' timestamps to the same timestamp as that most-recent-known
 *   transaction.
 * - If receiving a block with a past timestamp, but after the most recent known
 *   transaction, assign all its (not already known) transactions' timestamps to
 *   the block time.
 *
 * For more information see CWalletTx::nTimeSmart,
 * https://bitcointalk.org/?topic=54527, or
 * https://github.com/bitcoin/bitcoin/pull/1393.
 */
unsigned int CWallet::ComputeTimeSmart(const CWalletTx &wtx) const {
    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (!wtx.hashUnset()) {
        if (mapBlockIndex.count(wtx.hashBlock)) {
            int64_t latestNow = wtx.nTimeReceived;
            int64_t latestEntry = 0;

            // Tolerate times up to the last timestamp in the wallet not more
            // than 5 minutes into the future
            int64_t latestTolerated = latestNow + 300;
            const TxItems &txOrdered = wtxOrdered;
            for (auto it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
                CWalletTx *const pwtx = it->second.first;
                if (pwtx == &wtx) {
                    continue;
                }
                CAccountingEntry *const pacentry = it->second.second;
                int64_t nSmartTime;
                if (pwtx) {
                    nSmartTime = pwtx->nTimeSmart;
                    if (!nSmartTime) {
                        nSmartTime = pwtx->nTimeReceived;
                    }
                } else {
                    nSmartTime = pacentry->nTime;
                }
                if (nSmartTime <= latestTolerated) {
                    latestEntry = nSmartTime;
                    if (nSmartTime > latestNow) {
                        latestNow = nSmartTime;
                    }
                    break;
                }
            }

            int64_t blocktime = mapBlockIndex[wtx.hashBlock]->GetBlockTime();
            nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
        } else {
            LogPrintf("%s: found %s in block %s not in index\n", __func__,
                      wtx.GetId().ToString(), wtx.hashBlock.ToString());
        }
    }
    return nTimeSmart;
}

bool CWallet::AddDestData(const CTxDestination &dest, const std::string &key,
                          const std::string &value) {
  try {
    std::get<CNoDestination>(dest);
    return false;
  } catch (std::bad_variant_access&) {  }
  // Must be a CKeyID, BKeyID or CScriptID if we got here
  mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
  return WalletBatch(*database).WriteDestData(dest, key, value);
}

bool CWallet::EraseDestData(const CTxDestination &dest,
                            const std::string &key) {
    if (!mapAddressBook[dest].destdata.erase(key)) {
        return false;
    }

    return WalletBatch(*database).EraseDestData(dest, key);
}

void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key,
                           const std::string &value) {
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key,
                          std::string *value) const {
    auto i = mapAddressBook.find(dest);
    if (i != mapAddressBook.end()) {
        auto j = i->second.destdata.find(key);
        if (j != i->second.destdata.end()) {
            if (value) {
                *value = j->second;
            }

            return true;
        }
    }
    return false;
}

std::vector<std::string>
CWallet::GetDestValues(const std::string &prefix) const {
    LOCK(cs_wallet);
    std::vector<std::string> values;
    for (const auto &address : mapAddressBook) {
        for (const auto &data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}
bool CWallet::Verify(const CChainParams &chainParams, interfaces::Chain &chain,
                     const WalletLocation &location, bool salvage_wallet,
                     std::string &error_string, std::string &warning_string) {
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    // LOCK(cs_wallet);  -- not yet since static
    const fs::path &wallet_path = location.GetPath();
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
#ifdef NO_BOOST_FILESYSTEM
    if (!(path_type == fs::file_type::not_found || path_type == fs::file_type::directory ||
          (path_type == fs::file_type::symlink && fs::is_directory(wallet_path)) ||
          (path_type == fs::file_type::regular &&
           fs::path(location.GetName()).filename() == location.GetName()))) {
#else
    if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
          (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
          (path_type == fs::regular_file &&
           fs::path(location.GetName()).filename() == location.GetName()))) {
#endif
        error_string =
            strprintf("Invalid -wallet path '%s'. -wallet path should point to "
                      "a directory where wallet.dat and "
                      "database/log.?????????? files can be stored, a location "
                      "where such a directory could be created, "
                      "or (for backwards compatibility) the name of an "
                      "existing data file in -walletdir (%s)",
                      location.GetName(), GetWalletDir());
        return false;
    }

    // Make sure that the wallet path doesn't clash with an existing wallet path
    if (IsWalletLoaded(wallet_path)) {
        error_string = strprintf(
            "Error loading wallet %s. Duplicate -wallet filename specified.",
            location.GetName());
        return false;
    }

    // Keep same database environment instance across Verify/Recover calls
    // below.
    std::unique_ptr<WalletDatabase> database =
        WalletDatabase::Create(wallet_path);

    try {
        if (!WalletBatch::VerifyEnvironment(wallet_path, error_string)) {
            return false;
        }
    } catch (const fs::filesystem_error &e) {
        error_string = strprintf("Error loading wallet %s. %s",
                                 location.GetName(), e.what());
        return false;
    }

    if (salvage_wallet) {
        // Recover readable keypairs:
        CWallet dummyWallet(chainParams, chain, WalletLocation(),
                            WalletDatabase::CreateDummy());
        std::string backup_filename;
        if (!WalletBatch::Recover(
                wallet_path, static_cast<void *>(&dummyWallet),
                WalletBatch::RecoverKeysOnlyFilter, backup_filename)) {
            return false;
        }
    }

    return WalletBatch::VerifyDatabaseFile(wallet_path, warning_string,
                                           error_string);
}

#ifdef USE_PRESPLIT
void CWallet::MarkPreSplitKeys() {
    WalletBatch batch(*database);
    for (auto it = setExternalKeyPool.begin();
         it != setExternalKeyPool.end();) {
        int64_t index = *it;
        CKeyPool keypool;
        if (!batch.ReadPool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": read keypool entry failed");
        }
        keypool.m_pre_split = true;
        if (!batch.WritePool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": writing modified keypool entry failed");
        }
        set_pre_split_keypool.insert(index);
        it = setExternalKeyPool.erase(it);
    }
}
#endif

std::shared_ptr<CWallet>
CWallet::LoadWalletFromFile(const CChainParams &chainParams,
                            interfaces::Chain &chain,
                            const WalletLocation &location) {

    WalletFlag flag;
    auto ret = CreateWalletFromFile(chainParams, chain, location, SecureString(""),
                                    std::vector<std::string>(), flag);
    return ret;
 }

std::shared_ptr<CWallet>
CWallet::CreateWalletFromFile(const CChainParams &chainParams,
                              interfaces::Chain &chain,
                              const WalletLocation &location,
                              const SecureString& walletPassphrase,
                              const mnemonic::WordList& words,
                              const WalletFlag& wallet_creation_flags // just used if fFirstRun
                              ) {
    // Needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    const std::string &walletFile = location.GetName();
    
    if (gArgs.GetBoolArg("-zapwallettxes", false)) {
        uiInterface.InitMessage(_("Zapping all transactions from wallet..."));

        std::unique_ptr<CWallet> tempWallet = std::make_unique<CWallet>(
            chainParams, chain, location,
            WalletDatabase::Create(location.GetPath()));
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DBErrors::LOAD_OK) {
            InitError(
                strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
            return nullptr;
        }
    }

    uiInterface.InitMessage(_("Loading wallet..."));

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    std::shared_ptr<CWallet> walletInstance(
        new CWallet(chainParams, chain, location,
                    WalletDatabase::Create(location.GetPath())),
        ReleaseWallet);

   
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DBErrors::LOAD_OK) {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            InitError(
                strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
            return nullptr;
        }

        if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR) {
            InitWarning(strprintf(
                _("Error reading %s! All keys read correctly, but transaction "
                  "data"
                  " or address book entries might be missing or incorrect."),
                walletFile));
        } else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            InitError(strprintf(
                _("Error loading %s: Wallet requires newer version of DeVault-Core"),
                walletFile));
            return nullptr;
        } else if (nLoadWalletRet == DBErrors::NEED_REWRITE) {
            InitError(strprintf(
                _("Wallet needed to be rewritten: restart DeVault-Core to complete")));
            return nullptr;
        } else {
            InitError(strprintf(_("Error loading %s"), walletFile));
            return nullptr;
        }
    }
  
  
    // If old wallet and version doesn't have wallet flags,
    // set walletInstance wallet flags to a default with EC keys but no BLS,blank,etc
    {
        if (walletInstance->nWalletMaxVersion < FEATURE_FLAGS) {
            walletInstance->SetLegacyWalletFlags();
        }
      
        // Can happen on any run - do after loading WalletFlag from DB file & after potentially setting as Legacy
      /*
        if (gArgs.GetBoolArg("-upgradebls",false)) {
            walletInstance->SetWalletBLS();
        }
       */

    }

    if (gArgs.GetBoolArg("-upgradewallet", fFirstRun)) {
        int nMaxVersion = gArgs.GetArg("-upgradewallet", 0);
        // The -upgradewallet without argument case
        if (nMaxVersion == 0) {
            LogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = FEATURE_LATEST;
            // permanently upgrade the wallet immediately
            walletInstance->SetMinVersion(FEATURE_LATEST);
        } else {
            LogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        }

        if (nMaxVersion < walletInstance->GetVersion()) {
            InitError(_("Cannot downgrade wallet"));
            return nullptr;
        }

        walletInstance->SetMaxVersion(nMaxVersion);
    }



    if (fFirstRun) {
      CKeyingMaterial _vMasterKey; // Just new Random Bytes
      mnemonic::WordList cwords;
      std::vector<uint8_t> hashWords;

      // selective allow to set flags
      // On First run if using BLS, disable EC keys
      // or if -legacy, don't used BLS stuff

      bool legacy = gArgs.GetBoolArg("-legacy",false);
      if (legacy) walletInstance->SetWalletLegacy();
      else        walletInstance->SetWalletFlags(wallet_creation_flags);


      if (walletInstance->IsWalletBLS()) walletInstance->UnsetWalletLEGACY();

     // Create MasterKey and store to mapMasterKeys
      LogPrintf("%s : Creating MasterKey for wallet\n", __func__);
      walletInstance->CreateMasteyKey(walletPassphrase, _vMasterKey);
      
      //
      if (words.size() !=0) {
          hashWords = mnemonic::decodeMnemonic(words);
          cwords = words;
      } else {
          std::tie(cwords, hashWords) = GenerateHDMasterKey();
      }

      walletInstance->EncryptHDWallet(_vMasterKey, cwords, hashWords);
      walletInstance->Unlock(walletPassphrase);


      

      LogPrintf("%s : Generating HD keys, please don't interrupt til' done\n", __func__);
      // Top up the keypool
      if (walletInstance->IsWalletPrivate() &&  !walletInstance->TopUpKeyPool()) {
            InitError(_("Unable to generate initial keys") += "\n");
            return nullptr;
      }
      walletInstance->FinishEncryptWallet();
      LogPrintf("%s : Encrypted HDChain & keys written to wallet\n", __func__);
      
      walletInstance->ChainStateFlushed(chainActive.GetLocator());
    }

    
    if (gArgs.GetBoolArg("-bypasspassword",false)) {
      if (walletInstance->Unlock(BypassPassword)) {
          LogPrintf("Wallet successfully unlocked with bypass password\n");
      } else {
          InitError(strprintf(
                              _("Error loading %s: Wallet password is not compatible with -bypasspassword option."),
                              walletFile));
          return nullptr;
      }
    }


    

    if (!fFirstRun) {

        if (!walletInstance->IsWalletPrivate()) {
            LOCK(walletInstance->cs_KeyStore);
            if (!walletInstance->mapKeys.empty()) {
                InitWarning(strprintf(_("Warning: Private keys detected in wallet "
                                        "{%s} with disabled private keys"),
                                      walletFile));
                
            }
        }

        // If Upgrading from Non-BLS wallet, Remove Old Key pool and Replace
        // Maybe should be part of Wallet upgrade - check LATER XXX
        // Currently will skip if wallet is locked....
        if (walletInstance->IsWalletBLS() && !walletInstance->HasBLSKeys()) {
            
            // Should Add BLS Account to hdChain here.... and then re save
            CHDChain hdChainCrypted;
            walletInstance->GetCryptedHDChain(hdChainCrypted);
            assert(!hdChainCrypted.IsNull());
                
            hdChainCrypted.AddAccount(BLS_ACCOUNT);
            
            bool ok = walletInstance->StoreCryptedHDChain(hdChainCrypted);
            assert(ok);
            
            walletInstance->NewKeyPool();
        }
    }

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    // Temporary, for FindForkInGlobalIndex below. Removed in upcoming commit.
    LockAnnotation lock(::cs_main);
    auto locked_chain = chain.lock();
    LOCK(walletInstance->cs_wallet);

    CBlockIndex *pindexRescan = chainActive.Genesis();
    if (!gArgs.GetBoolArg("-rescan", false)) {
        WalletBatch batch(*walletInstance->database);
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator)) {
            pindexRescan = FindForkInGlobalIndex(chainActive, locator);
        }
    }

    walletInstance->m_last_block_processed = chainActive.Tip();
    // NOT YET - CHECK LATER RegisterValidationInterface(walletInstance);

    if (chainActive.Tip() && chainActive.Tip() != pindexRescan) {
        // We can't rescan beyond non-pruned blocks, stop and throw an error.
        // This might happen if a user uses a old wallet within a pruned node or
        // if he ran -disablewallet for a longer time, then decided to
        // re-enable.
        if (fPruneMode) {
            CBlockIndex *block = chainActive.Tip();
            while (block && block->pprev && block->pprev->nStatus.hasData() &&
                   block->pprev->nTx > 0 && pindexRescan != block) {
                block = block->pprev;
            }

            if (pindexRescan != block) {
                InitError(_("Prune: last wallet synchronisation goes beyond "
                            "pruned data. You need to -reindex (download the "
                            "whole blockchain again in case of pruned node)"));
                return nullptr;
            }
        }

        uiInterface.InitMessage(_("Rescanning..."));
        LogPrintf("Rescanning last %i blocks (from block %i)...\n",
                  chainActive.Height() - pindexRescan->nHeight,
                  pindexRescan->nHeight);

        // No need to read and scan block if block was created before our wallet
        // birthday (as adjusted for block time variability)
        while (pindexRescan && walletInstance->nTimeFirstKey &&
               (pindexRescan->GetBlockTime() <
                (walletInstance->nTimeFirstKey - TIMESTAMP_WINDOW))) {
            pindexRescan = chainActive.Next(pindexRescan);
        }

        nStart = GetTimeMillis();
        {
            WalletRescanReserver reserver(walletInstance.get());
            if (!reserver.reserve()) {
                InitError(
                    _("Failed to rescan the wallet during initialization"));
                return nullptr;
            }
            walletInstance->ScanForWalletTransactions(pindexRescan, nullptr,
                                                      reserver, true);
        }
        LogPrintf(" rescan      %15dms\n", GetTimeMillis() - nStart);
        walletInstance->ChainStateFlushed(chainActive.GetLocator());
        walletInstance->database->IncrementUpdateCounter();

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (gArgs.GetBoolArg("-zapwallettxes", false) &&
            gArgs.GetArg("-zapwallettxes", "1") != "2") {
            WalletBatch batch(*walletInstance->database);

            for (const CWalletTx &wtxOld : vWtx) {
                const TxId txid = wtxOld.GetId();
                auto mi = walletInstance->mapWallet.find(txid);
                if (mi != walletInstance->mapWallet.end()) {
                    const CWalletTx *copyFrom = &wtxOld;
                    CWalletTx *copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->strFromAccount = copyFrom->strFromAccount;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    batch.WriteTx(*copyTo);
                }
            }
        }
    }

          // was uiInterface.LoadWallet(walletInstance);
    chain.loadWallet(interfaces::MakeWallet(walletInstance));

    // Register with the validation interface. It's ok to do this after rescan
    // since we're still holding cs_main.
    RegisterValidationInterface(walletInstance.get());

    walletInstance->SetBroadcastTransactions(
        gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    //LOCK(walletInstance->cs_wallet);
    LogPrintf("setKeyPool.size() = %u\n", walletInstance->GetKeyPoolSize());
    LogPrintf("mapWallet.size() = %u\n", walletInstance->mapWallet.size());
    LogPrintf("mapAddressBook.size() = %u\n",
              walletInstance->mapAddressBook.size());

    return walletInstance;
}

std::atomic<bool> CWallet::fFlushScheduled(false);

void CWallet::postInitProcess() {
    // Add wallet transactions that aren't already in a block to mempool.
    // Do this here as mempool requires genesis block to be loaded.
    ReacceptWalletTransactions();
}

bool CWallet::BackupWallet(const std::string &strDest) {
  return database->Backup(strDest);
}

CKeyPool::CKeyPool() {
    nTime = GetTime();
    fInternal = false;
}

CKeyPool::CKeyPool(const CPubKey &vchPubKeyIn, bool internalIn) {
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = internalIn;
}

CWalletKey::CWalletKey(int64_t nExpires) {
    nTimeCreated = (nExpires ? GetTime() : 0);
    nTimeExpires = nExpires;
}

void CMerkleTx::SetMerkleBranch(const CBlockIndex *pindex, int posInBlock) {
    // Update the tx's hashBlock
    hashBlock = pindex->GetBlockHash();

    // Set the position of the transaction in the block.
    nIndex = posInBlock;
}

int CMerkleTx::GetDepthInMainChain(
    interfaces::Chain::Lock &locked_chain) const {
    if (hashUnset()) {
        return 0;
    }

    AssertLockHeld(cs_main);

    // Find the block it claims to be in.
    auto mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end()) {
        return 0;
    }

    CBlockIndex *pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex)) {
        return 0;
    }

    return ((nIndex == -1) ? (-1) : 1) *
           (chainActive.Height() - pindex->nHeight + 1);
}

int CMerkleTx::GetBlocksToMaturity(
    interfaces::Chain::Lock &locked_chain) const {
    if (!IsCoinBase()) {
        return 0;
    }

    return std::max(0, (COINBASE_MATURITY + 1) -
                           GetDepthInMainChain(locked_chain));
}

bool CMerkleTx::IsImmatureCoinBase(
    interfaces::Chain::Lock &locked_chain) const {
    // note GetBlocksToMaturity is 0 for non-coinbase tx
    return GetBlocksToMaturity(locked_chain) > 0;
}

bool CWalletTx::AcceptToMemoryPool(interfaces::Chain::Lock &locked_chain,
                                   const Amount nAbsurdFee,
                                   CValidationState &state) {

    // Quick check to avoid re-setting fInMempool to false
    if (g_mempool.exists(tx->GetId())) {
        return false;
    }
    // Temporary, for AcceptToMemoryPool below. Removed in upcoming commit.
    LockAnnotation lock(::cs_main);

    // We must set fInMempool here - while it will be re-set to true by the
    // entered-mempool callback, if we did not there would be a race where a
    // user could call sendmoney in a loop and hit spurious out of funds errors
    // because we think that the transaction they just generated's change is
    // unavailable as we're not yet aware its in mempool.
    bool ret = ::AcceptToMemoryPool(
        GetConfig(), g_mempool, state, tx, true /* fLimitFree */,
        nullptr /* pfMissingInputs */, false /* fOverrideMempoolLimit */,
        nAbsurdFee);
    fInMempool = ret;
    return ret;
}

bool CWallet::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_wallet);
    auto mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        const CHDPubKey &hdPubKey = (*mi).second;
        vchPubKeyOut = hdPubKey.extPubKey.pubkey;
        return true;
    }
    else
        return CCryptoKeyStore::GetPubKey(address, vchPubKeyOut);
}
bool CWallet::GetPubKey(const BKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_wallet);
    auto mi = mapBLSPubKeys.find(address);
    if (mi != mapBLSPubKeys.end())
    {
        const CHDPubKey &hdPubKey = (*mi).second;
        vchPubKeyOut = hdPubKey.extPubKey.pubkey;
        return true;
    }
    else
        return CCryptoKeyStore::GetPubKey(address, vchPubKeyOut); // Watch only
}

bool CWallet::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_wallet);
    auto mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        // if the key has been found in mapHdPubKeys, derive it on the fly
        const CHDPubKey &hdPubKey = (*mi).second;
        CHDChain hdChainCurrent;
        // Get & Decrypt HDchain
        if (!CCryptoKeyStore::GetDecryptedHDChain(hdChainCurrent))
          throw std::runtime_error(std::string(__func__) + ": GetDecryptedHDChain failed");

        CExtKey extkey;
        hdChainCurrent.DeriveChildExtKey(hdPubKey.nAccountIndex, hdPubKey.nChangeIndex != 0, hdPubKey.extPubKey.nChild, extkey, false);
        keyOut = extkey.key;

        return true;
    }
    else {
        return CCryptoKeyStore::GetKey(address, keyOut);
    }
}

bool CWallet::GetKey(const BKeyID &address, CKey& keyOut) const
{
    LOCK(cs_wallet);
    auto mi = mapBLSPubKeys.find(address);
    if (mi != mapBLSPubKeys.end())
    {
        // if the key has been found in mapBLSPubKeys, derive it on the fly
        const CHDPubKey &hdPubKey = (*mi).second;
        CHDChain hdChainCurrent;
        // Get & Decrypt HDchain
        if (!CCryptoKeyStore::GetDecryptedHDChain(hdChainCurrent))
          throw std::runtime_error(std::string(__func__) + ": GetDecryptedHDChain failed");

        CExtKey extkey;
        hdChainCurrent.DeriveChildExtKey(hdPubKey.nAccountIndex, hdPubKey.nChangeIndex != 0, hdPubKey.extPubKey.nChild, extkey, true);
        keyOut = extkey.key;

        return true;
    }
    else {
        return CCryptoKeyStore::GetKey(address, keyOut);
    }
}

bool CWallet::GetMnemonic(CHDChain &chain, SecureString& securewords) const
{
    LOCK(cs_wallet);
    if (chain.IsNull()) return false;
    if (!DecryptHDChain(chain))
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChainSeed failed");
    chain.GetMnemonic(securewords);
    return true;
}
SecureVector CWallet::getWords() const
{
  SecureVector words;
  LOCK(cs_wallet);
  auto [hdChainDec, hdChainEnc] = GetHDChains();
  (void)hdChainEnc;
  hdChainDec.GetMnemonic(words);
  return words;
}

bool CWallet::HaveKey(const CKeyID &address) const
{
    LOCK(cs_wallet);
    if (mapHdPubKeys.count(address) > 0)
        return true;
    return CCryptoKeyStore::HaveKey(address);
}

bool CWallet::HaveKey(const BKeyID &address) const
{
    LOCK(cs_wallet);
    if (mapBLSPubKeys.count(address) > 0)
        return true;
    return CCryptoKeyStore::HaveKey(address);
}

bool CWallet::LoadHDPubKey(const CHDPubKey &hdPubKey)
{
    AssertLockHeld(cs_wallet);
    //LogPrintf("Loaded EC %s\n",EncodeDestination(hdPubKey.extPubKey.pubkey.GetKeyID()));
    if (hdPubKey.extPubKey.IsBLS()) {
        mapBLSPubKeys[hdPubKey.extPubKey.pubkey.GetBLSKeyID()] = hdPubKey;
        //std::cout << "Loaded " << EncodeDestination(hdPubKey.extPubKey.blspubkey.GetKeyID()) << "\n";
        //LogPrintf("Loaded %s\n",EncodeDestination(hdPubKey.extPubKey.blspubkey.GetKeyID()));
    } else {
        mapHdPubKeys[hdPubKey.extPubKey.pubkey.GetKeyID()] = hdPubKey;
    }
    return true;
}

bool CWallet::AddHDPubKey(const CExtPubKey &extPubKey, bool fInternal)
{
    AssertLockHeld(cs_wallet);

    CHDChain hdChainCurrent;
    GetCryptedHDChain(hdChainCurrent);

    CHDPubKey hdPubKey;
    hdPubKey.extPubKey = extPubKey;
    hdPubKey.hdchainID = hdChainCurrent.GetID();
    hdPubKey.nChangeIndex = fInternal ? 1 : 0;
    if (hdPubKey.extPubKey.IsBLS()) {
        mapBLSPubKeys[extPubKey.pubkey.GetBLSKeyID()] = hdPubKey;
    } else {
        mapHdPubKeys[extPubKey.pubkey.GetKeyID()] = hdPubKey;
    }

    // check if we need to remove from watch-only
    CScript script;
    if (extPubKey.pubkey.IsEC()) {
        script = GetScriptForDestination(extPubKey.pubkey.GetKeyID());
    } else {
        script = GetScriptForDestination(extPubKey.pubkey.GetBLSKeyID());
    }
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(extPubKey.pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    if (hdPubKey.extPubKey.IsBLS()) {
        return WalletBatch(*database).WriteHDPubKey(hdPubKey, mapBLSKeyMetadata[extPubKey.pubkey.GetBLSKeyID()]);
    } else {
        return WalletBatch(*database).WriteHDPubKey(hdPubKey, mapKeyMetadata[extPubKey.pubkey.GetKeyID()]);
    }
}

CHDPubKey CWallet::AddHDPubKeyWithoutDB(const CExtPubKey &extPubKey, bool fInternal)
  {
    AssertLockHeld(cs_wallet);
    
    CHDChain hdChainCurrent;
    GetCryptedHDChain(hdChainCurrent);
    
    CHDPubKey hdPubKey;
    hdPubKey.extPubKey = extPubKey;
    hdPubKey.hdchainID = hdChainCurrent.GetID();
    hdPubKey.nChangeIndex = fInternal ? 1 : 0;
    if (hdPubKey.extPubKey.IsBLS()) {
        hdPubKey.nAccountIndex = BLS_ACCOUNT;
        mapBLSPubKeys[extPubKey.pubkey.GetBLSKeyID()] = hdPubKey;
    } else {
        hdPubKey.nAccountIndex = 0;
        mapHdPubKeys[extPubKey.pubkey.GetKeyID()] = hdPubKey;
    }
    
    // check if we need to remove from watch-only
    CScript script;
    if (extPubKey.pubkey.IsEC()) {
        script = GetScriptForDestination(extPubKey.pubkey.GetKeyID());
    } else {
        script = GetScriptForDestination(extPubKey.pubkey.GetBLSKeyID());
    }
    if (HaveWatchOnly(script))
      RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(extPubKey.pubkey);
    if (HaveWatchOnly(script))
      RemoveWatchOnly(script);
    
    return hdPubKey;
  }

bool CWallet::WriteBLSRandomKey(const CKey &key) const {
  auto nID = key.GetPubKeyForBLS();
  if (!WalletBatch(*database).WriteRandomKey(nID, key.GetBLSPrivateKey())) {
      throw std::runtime_error(std::string(__func__) + ": WriteRandomKey failed");
    }
    return true;
}

bool CWallet::StoreCryptedHDChain(const CHDChain& chain) {
    LOCK(cs_wallet);
    if (!WalletBatch(*database).WriteCryptedHDChain(chain)) {
      throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
    }
    return true;
}

bool CWallet::StoreCryptedHDChain() {

  LOCK(cs_wallet);
  CHDChain chain;
  if (!CCryptoKeyStore::GetCryptedHDChain(chain)) {
    throw std::runtime_error(std::string(__func__) + ": GetCryptedHDChain failed");
  }
    
  if (!WalletBatch(*database).WriteCryptedHDChain(chain)) {
    throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
  }
  return true;
}

bool CWallet::SetCryptedHDChain(const CHDChain& chain) {
  LOCK(cs_wallet);
  if (!CCryptoKeyStore::SetCryptedHDChain(chain)) {
        return false;
  }
  return true;
}

// Return Decrypted then Encrypted Chains
std::tuple<CHDChain,CHDChain> CWallet::GetHDChains() const {

  CHDChain hdChainEnc;
  CHDChain hdChainDec;

  // Get Encrypted hdChain
  if (!GetCryptedHDChain(hdChainEnc)) {
    throw std::runtime_error(std::string(__func__) + ": GetCryptedHDChain failed");
  }
  
  // Decrypt
  if (hdChainEnc.IsCrypted()) {
    hdChainDec = hdChainEnc;
    if (!DecryptHDChain(hdChainDec))
      throw std::runtime_error(std::string(__func__) + ": DecryptHDChainSeed failed");
  }

  // make sure seed matches this chain
  if (hdChainDec.GetID() != hdChainDec.GetSeedHash())
    throw std::runtime_error(std::string(__func__) + ": Wrong HD chain!");

  return std::tuple(hdChainDec, hdChainEnc);
}

std::tuple<bool, CKey, CPubKey, bool> CWallet::ExtractFromBLSScript(const CScript &scriptPubKey) const {
    CKey key;
    CPubKey pubkey;
    txnouttype whichTypeRet;
    std::vector<std::vector<uint8_t>> vSolutions;
    bool ok = Solver(scriptPubKey, whichTypeRet, vSolutions);
    if (!ok) {
        return std::tuple(ok, key, pubkey, whichTypeRet == TX_BLSPUBKEY);
    }
    
    BKeyID keyID1;
    if (whichTypeRet == TX_BLSPUBKEY) {
        keyID1 = CPubKey(vSolutions[0]).GetBLSKeyID();
    } else {
        keyID1 = BKeyID(uint160(vSolutions[0]));
    }
    
    bool got_key = GetKey(keyID1, key);
    GetPubKey(keyID1, pubkey);

    return std::tuple(got_key, key, pubkey, whichTypeRet == TX_BLSPUBKEY);
}
