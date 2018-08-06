// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Bitcoin developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletdb.h>


#include <chainparams.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <dstencode.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <util/fs_util.h>
#include <util/system.h>
#include <util/time.h>
#include <wallet/wallet.h>

#include <thread>
#include <atomic>

//
// WalletBatch
//

bool WalletBatch::ReadName(const std::string& address, std::string &strName) {
    return m_batch.Read(std::make_pair(std::string("name"), address), strName);
}
bool WalletBatch::ReadLabel(std::string& address, const std::string &strName) {
    return m_batch.Read(std::make_pair(std::string("label"), strName), address);
}

bool WalletBatch::WriteNameAndLabel(const CTxDestination &address,
                          const std::string &strName) {
    if (!IsValidDestination(address)) {
        return false;
    }
    // Provide 2-way look ups by label/address
    if (!WriteIC(std::make_pair(std::string("label"), strName), EncodeDestination(address))) return false;
    return WriteIC(std::make_pair(std::string("name"),EncodeDestination(address)),strName);
}

bool WalletBatch::EraseName(const CTxDestination &address) {
    // This should only be used for sending addresses, never for receiving
    // addresses, receiving addresses must always have an address book entry if
    // they're not change return.
    if (!IsValidDestination(address)) {
        return false;
    }
    return EraseIC(std::make_pair(std::string("name"), EncodeDestination(address)));
}

bool WalletBatch::WritePurpose(const CTxDestination &address,
                             const std::string &strPurpose) {
    if (!IsValidDestination(address)) {
        return false;
    }
    return WriteIC(std::make_pair(std::string("purpose"),EncodeDestination(address)),
                   strPurpose);
}

bool WalletBatch::ErasePurpose(const CTxDestination &address) {
    if (!IsValidDestination(address)) {
        return false;
    }
    return EraseIC(std::make_pair(std::string("purpose"),EncodeDestination(address)));
}

bool WalletBatch::WriteTx(const CWalletTx &wtx) {
    return WriteIC(std::make_pair(std::string("tx"), wtx.GetId()), wtx);
}

bool WalletBatch::EraseTx(uint256 hash) {
    return EraseIC(std::make_pair(std::string("tx"), hash));
}

bool WalletBatch::WriteMasterKey(unsigned int nID, const CMasterKey &kMasterKey) {
    return WriteIC(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
}

bool WalletBatch::WriteRandomKey(const CPubKey &pubk, const CPrivKey &kRandomKey) {
    return WriteIC(std::make_pair(std::string("rkey"), pubk), kRandomKey, true);
}

bool WalletBatch::WriteCScript(const uint160 &hash, const CScript &redeemScript) {
    return WriteIC(std::make_pair(std::string("cscript"), hash), redeemScript,
                   false);
}

bool WalletBatch::WriteWatchOnly(const CScript &dest,
                               const CKeyMetadata &keyMeta) {
    if (!WriteIC(std::make_pair(std::string("watchmeta"), dest), keyMeta)) {
        return false;
    }
    return WriteIC(std::make_pair(std::string("watchs"), dest), '1');
}

bool WalletBatch::EraseWatchOnly(const CScript &dest) {
    if (!EraseIC(std::make_pair(std::string("watchmeta"), dest))) {
        return false;
    }
    return EraseIC(std::make_pair(std::string("watchs"), dest));
}

bool WalletBatch::WriteBestBlock(const CBlockLocator &locator) {
    // Write empty block locator so versions that require a merkle branch
    // automatically rescan
    WriteIC(std::string("bestblock"), CBlockLocator());
    return WriteIC(std::string("bestblock_nomerkle"), locator);
}

bool WalletBatch::ReadBestBlock(CBlockLocator &locator) {
    if (m_batch.Read(std::string("bestblock"), locator) &&
        !locator.vHave.empty()) {
        return true;
    }
    return m_batch.Read(std::string("bestblock_nomerkle"), locator);
}

bool WalletBatch::WriteOrderPosNext(int64_t nOrderPosNext) {
    return WriteIC(std::string("orderposnext"), nOrderPosNext);
}

bool WalletBatch::ReadPool(int64_t nPool, CKeyPool &keypool) {
    return m_batch.Read(std::make_pair(std::string("pool"), nPool), keypool);
}

bool WalletBatch::WritePool(int64_t nPool, const CKeyPool &keypool) {
    return WriteIC(std::make_pair(std::string("pool"), nPool), keypool);
}

bool WalletBatch::ErasePool(int64_t nPool) {
    return EraseIC(std::make_pair(std::string("pool"), nPool));
}

bool WalletBatch::WriteMinVersion(int nVersion) {
    return WriteIC(std::string("minversion"), nVersion);
}

bool WalletBatch::ReadAccount(const std::string &strAccount,
                              CAccount &account) {
    account.SetNull();
    return m_batch.Read(std::make_pair(std::string("acc"), strAccount),
                        account);
}

bool WalletBatch::WriteAccount(const std::string &strAccount,
                               const CAccount &account) {
    return WriteIC(std::make_pair(std::string("acc"), strAccount), account);
}

bool WalletBatch::EraseAccount(const std::string &strAccount) {
    return EraseIC(std::make_pair(std::string("acc"), strAccount));
}

bool WalletBatch::WriteAccountingEntry(const uint64_t nAccEntryNum,
                                     const CAccountingEntry &acentry) {
    return WriteIC(
        std::make_pair(std::string("acentry"),
                       std::make_pair(acentry.strAccount, nAccEntryNum)),
        acentry);
}

Amount WalletBatch::GetAccountCreditDebit(const std::string &strAccount) {
    std::list<CAccountingEntry> entries;
    ListAccountCreditDebit(strAccount, entries);

    Amount nCreditDebit = Amount::zero();
    for (const CAccountingEntry &entry : entries) {
        nCreditDebit += entry.nCreditDebit;
    }

    return nCreditDebit;
}

void WalletBatch::ListAccountCreditDebit(const std::string &strAccount,
                                       std::list<CAccountingEntry> &entries) {
    bool fAllAccounts = (strAccount == "*");

    Dbc *pcursor = m_batch.GetCursor();
    if (!pcursor) {
        throw std::runtime_error(std::string(__func__) +
                                 ": cannot create DB cursor");
    }
    bool setRange = true;
    while (true) {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (setRange) {
            ssKey << std::make_pair(
                std::string("acentry"),
                std::make_pair((fAllAccounts ? std::string("") : strAccount),
                               uint64_t(0)));
        }
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue, setRange);
        setRange = false;
        if (ret == DB_NOTFOUND) {
            break;
        }

        if (ret != 0) {
            pcursor->close();
            throw std::runtime_error(std::string(__func__) +
                                     ": error scanning DB");
        }

        // Unserialize
        std::string strType;
        ssKey >> strType;
        if (strType != "acentry") {
            break;
        }
        CAccountingEntry acentry;
        ssKey >> acentry.strAccount;
        if (!fAllAccounts && acentry.strAccount != strAccount) {
            break;
        }

        ssValue >> acentry;
        ssKey >> acentry.nEntryNo;
        entries.push_back(acentry);
    }

    pcursor->close();
}

class CWalletScanState {
public:
    unsigned int nCKeys{0};
    unsigned int nWatchKeys{0};
    unsigned int nKeyMeta{0};
    bool fIsEncrypted{false};
    bool fAnyUnordered{false};
    int nFileVersion{0};
    unsigned int m_unknown_records{0};
    std::vector<TxId> vWalletUpgrade;

    CWalletScanState() {}
};

bool ReadKeyValue(CWallet *pwallet, CDataStream &ssKey, CDataStream &ssValue,
                  CWalletScanState &wss, std::string &strType,
                  std::string &strErr) 
    EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet) {
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization is just the two
        // items serialized one after the other.
        ssKey >> strType;
        if (strType == "name") {
            std::string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet
                           ->mapAddressBook[DecodeDestination(
                               strAddress, pwallet->chainParams)]
                           .name;
        } else if (strType == "purpose") {
            std::string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet
                           ->mapAddressBook[DecodeDestination(
                               strAddress, pwallet->chainParams)]
                           .purpose;
        } else if (strType == "tx") {
            TxId txid;
            ssKey >> txid;
            CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
            ssValue >> wtx;
            CValidationState state;
            bool isValid = wtx.IsCoinBase()
                               ? CheckCoinbase(*wtx.tx, state)
                               : CheckRegularTransaction(*wtx.tx, state);
            if (!isValid || wtx.GetId() != txid) {
                return false;
            }

            if (wtx.nOrderPos == -1) {
                wss.fAnyUnordered = true;
            }

            pwallet->LoadToWallet(wtx);
        } else if (strType == "acentry") {
            std::string strAccount;
            ssKey >> strAccount;
            uint64_t nNumber;
            ssKey >> nNumber;
            if (nNumber > pwallet->nAccountingEntryNumber) {
                pwallet->nAccountingEntryNumber = nNumber;
            }

            if (!wss.fAnyUnordered) {
                CAccountingEntry acentry;
                ssValue >> acentry;
                if (acentry.nOrderPos == -1) {
                    wss.fAnyUnordered = true;
                }
            }
        } else if (strType == "watchs") {
            wss.nWatchKeys++;
            CScript script;
            ssKey >> script;
            char fYes;
            ssValue >> fYes;
            if (fYes == '1') {
                pwallet->LoadWatchOnly(script);
            }
        } else if (strType == "mkey") {
            unsigned int nID;
            ssKey >> nID;
            CMasterKey kMasterKey;
            ssValue >> kMasterKey;
            if (pwallet->mapMasterKeys.count(nID) != 0) {
                strErr = strprintf(
                    "Error reading wallet database: duplicate CMasterKey id %u",
                    nID);
                return false;
            }
            pwallet->mapMasterKeys[nID] = kMasterKey;
            if (pwallet->nMasterKeyMaxID < nID) {
                pwallet->nMasterKeyMaxID = nID;
            }
         } else if (strType == "ckey") {
            wss.nCKeys++;
            strErr = "Problem with wallet database: You have Crypted Keys not supported by this software";
            return false;
         } else if (strType == "keymeta") {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            if (vchPubKey.IsBLS()) {
                pwallet->LoadKeyMetadata(vchPubKey.GetBLSKeyID(), keyMeta);
            } else {
                pwallet->LoadKeyMetadata(vchPubKey.GetKeyID(), keyMeta);
            }
        } else if (strType == "watchmeta") {
            CScript script;
            ssKey >> script;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            pwallet->LoadScriptMetadata(CScriptID(script), keyMeta);
        } else if (strType == "defaultkey") {
            // We don't want or need the default key, but if there is one set,
            // we want to make sure that it is valid so that we can detect
            // corruption
            CPubKey vchPubKey;
            ssValue >> vchPubKey;
            if (!vchPubKey.IsValid()) {
                strErr = "Error reading wallet database: Default Key corrupt";
                return false;
            }
        } else if (strType == "pool") {
            int64_t nIndex;
            ssKey >> nIndex;
            CKeyPool keypool;
            ssValue >> keypool;

            pwallet->LoadKeyPool(nIndex, keypool);
        } else if (strType == "version") {
            ssValue >> wss.nFileVersion;
        } else if (strType == "cscript") {
            uint160 hash;
            ssKey >> hash;
            CScript script;
            ssValue >> script;
            if (!pwallet->LoadCScript(script)) {
                strErr = "Error reading wallet database: LoadCScript failed";
                return false;
            }
        } else if (strType == "orderposnext") {
            ssValue >> pwallet->nOrderPosNext;
        } else if (strType == "destdata") {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
            pwallet->LoadDestData(
                                  DecodeDestination(strAddress, pwallet->chainParams), strKey, strValue);
        } else if (strType == "chdchain") {
            CHDChain chain;
            ssValue >> chain;
            if (!pwallet->SetCryptedHDChain(chain)) {
                strErr = "Error reading wallet database: SetHDCrypted failed";
                return false;
            }
        } else if (strType == "hdpubkey")  { // Later change this to blspubkey
            CPubKey vchPubKey;
            ssKey >> vchPubKey;

            CHDPubKey hdPubKey;
            ssValue >> hdPubKey;

            if(vchPubKey != hdPubKey.extPubKey.pubkey)
            {
                strErr = "Error reading wallet database: CHDPubKey corrupt";
                return false;
            }
            
            if (!pwallet->LoadHDPubKey(hdPubKey))
            {
                strErr = "Error reading wallet database: LoadHDPubKey failed";
                return false;
            }
        } else if (strType == "flags") {
            WalletFlag flags;
            ssValue >> flags;
            pwallet->SetWalletFlags(flags);
        } else if (strType != "bestblock" && strType != "bestblock_nomerkle") {
            wss.m_unknown_records++;
        }
    } catch (...) {
        return false;
    }
    return true;
}
bool WalletBatch::ReadHDPubKeyTest(CPubKey& pubkey, CHDPubKey& hdpubkey) {
    return m_batch.Read(std::make_pair(std::string("hdpubkey"), pubkey), hdpubkey);
}
bool WalletBatch::WriteHDPubKeyTest(CHDPubKey& hdPubKey) {
    return (!WriteIC(std::make_pair(std::string("hdpubkey"), hdPubKey.extPubKey.pubkey), hdPubKey, false));
}

bool WalletBatch::IsKeyType(const std::string &strType) {
    return (strType == "mkey" ||
            strType == "chdchain" ||
            strType == "ckey");
}

DBErrors WalletBatch::LoadWallet(CWallet *pwallet) {
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    DBErrors result = DBErrors::LOAD_OK;

    LOCK(pwallet->cs_wallet);
    try {
        int nMinVersion = 0;
        if (m_batch.Read((std::string) "minversion", nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST) {
                return DBErrors::TOO_NEW;
            }
            pwallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc *pcursor = m_batch.GetCursor();
        if (!pcursor) {
            pwallet->WalletLogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true) {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND) {
                break;
            }

            if (ret != 0) {
                pwallet->WalletLogPrintf(
                    "Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr)) {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (IsKeyType(strType) || strType == "defaultkey") {
                    result = DBErrors::CORRUPT;
                } else if (strType == "flags") {
                    // Reading the wallet flags can only fail if unknown flags
                    // are present.
                    result = DBErrors::TOO_NEW;
                } else {
                    // Leave other errors alone, if we try to fix them we might
                    // make things worse. But do warn the user there is
                    // something wrong.
                    fNoncriticalErrors = true;
                    if (strType == "tx") {
                        // Rescan if there is a bad transaction record:
                        gArgs.SoftSetBoolArg("-rescan", true);
                    }
                }
            }
            if (!strErr.empty()) {
                pwallet->WalletLogPrintf("%s\n", strErr);
            }
        }
        pcursor->close();
        
    } catch (const thread_interrupted &) {
        throw;
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    if (fNoncriticalErrors && result == DBErrors::LOAD_OK) {
        result = DBErrors::NONCRITICAL_ERROR;
    }

    // Any wallet corruption at all: skip any rewriting or upgrading, we don't
    // want to make it worse.
    if (result != DBErrors::LOAD_OK) {
        return result;
    }

    pwallet->WalletLogPrintf("nFileVersion = %d\n", wss.nFileVersion);

    if (wss.nCKeys) {
        InitWarning(_("You have one or more private keys in your wallet that are potentially not backed"
                      " by your seed phrase. You should revert to an older wallet that suppor them as"
                      " they are not supported by this software version"));
    }


    pwallet->WalletLogPrintf("Keys: %u encrypted, %u w/ metadata, unknown wallet records: %u\n", wss.nCKeys, wss.nKeyMeta,
              wss.m_unknown_records);

    // nTimeFirstKey is only reliable if all keys have metadata
    if ((wss.nCKeys + wss.nWatchKeys) != wss.nKeyMeta) {
        pwallet->UpdateTimeFirstKey(1);
    }

    for (const TxId &txid : wss.vWalletUpgrade) {
        WriteTx(pwallet->mapWallet.at(txid));
    }

    if (wss.nFileVersion < CLIENT_VERSION) {
        // Update
        WriteVersion(CLIENT_VERSION);
    }

    if (wss.fAnyUnordered) {
        result = pwallet->ReorderTransactions();
    }

    pwallet->laccentries.clear();
    ListAccountCreditDebit("*", pwallet->laccentries);
    for (CAccountingEntry &entry : pwallet->laccentries) {
        pwallet->wtxOrdered.insert(
            std::make_pair(entry.nOrderPos, CWallet::TxPair(nullptr, &entry)));
    }

    return result;
}

DBErrors WalletBatch::FindWalletTx(std::vector<TxId> &txIds,
                                 std::vector<CWalletTx> &vWtx) {
    DBErrors result = DBErrors::LOAD_OK;

    try {
        int nMinVersion = 0;
        if (m_batch.Read((std::string) "minversion", nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST) {
                return DBErrors::TOO_NEW;
            }
        }

        // Get cursor
        Dbc *pcursor = m_batch.GetCursor();
        if (!pcursor) {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true) {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND) {
                break;
            }

            if (ret != 0) {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            std::string strType;
            ssKey >> strType;
            if (strType == "tx") {
                TxId txid;
                ssKey >> txid;

                CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
                ssValue >> wtx;

                txIds.push_back(txid);
                vWtx.push_back(wtx);
            }
        }
        pcursor->close();
    } catch (const thread_interrupted &) {
        throw;
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::ZapSelectTx(std::vector<TxId> &txIdsIn,
                                std::vector<TxId> &txIdsOut) {
    // Build list of wallet TXs and hashes.
    std::vector<TxId> txIds;
    std::vector<CWalletTx> vWtx;
    DBErrors err = FindWalletTx(txIds, vWtx);
    if (err != DBErrors::LOAD_OK) {
        return err;
    }

    std::sort(txIds.begin(), txIds.end());
    std::sort(txIdsIn.begin(), txIdsIn.end());

    // Erase each matching wallet TX.
    bool delerror = false;
    auto it = txIdsIn.begin();
    for (const TxId &txid : txIds) {
        while (it < txIdsIn.end() && (*it) < txid) {
            it++;
        }
        if (it == txIdsIn.end()) {
            break;
        }

        if ((*it) == txid) {
            if (!EraseTx(txid)) {
                LogPrint(BCLog::DB,
                         "Transaction was found for deletion but returned "
                         "database error: %s\n",
                         txid.GetHex());
                delerror = true;
            }
            txIdsOut.push_back(txid);
        }
    }

    if (delerror) {
        return DBErrors::CORRUPT;
    }
    return DBErrors::LOAD_OK;
}

DBErrors WalletBatch::ZapWalletTx(std::vector<CWalletTx> &vWtx) {
    // Build list of wallet TXs.
    std::vector<TxId> txIds;
    DBErrors err = FindWalletTx(txIds, vWtx);
    if (err != DBErrors::LOAD_OK) {
        return err;
    }

    // Erase each wallet TX.
    for (const TxId &txid : txIds) {
        if (!EraseTx(txid)) {
            return DBErrors::CORRUPT;
        }
    }

    return DBErrors::LOAD_OK;
}

void MaybeCompactWalletDB() {
    static std::atomic<bool> fOneThread;
    if (fOneThread.exchange(true)) {
        return;
    }
    if (!gArgs.GetBoolArg("-flushwallet", DEFAULT_FLUSHWALLET)) {
        return;
    }

    for (const std::shared_ptr<CWallet> &pwallet : GetWallets()) {
        WalletDatabase &dbh = pwallet->GetDBHandle();

        unsigned int nUpdateCounter = dbh.nUpdateCounter;

        if (dbh.nLastSeen != nUpdateCounter) {
            dbh.nLastSeen = nUpdateCounter;
            dbh.nLastWalletUpdate = GetTime();
        }

        if (dbh.nLastFlushed != nUpdateCounter &&
            GetTime() - dbh.nLastWalletUpdate >= 2) {
            if (BerkeleyBatch::PeriodicFlush(dbh)) {
                dbh.nLastFlushed = nUpdateCounter;
            }
        }
    }

    fOneThread = false;
}

//
// Try to (very carefully!) recover wallet file if there is a problem.
//
bool WalletBatch::Recover(const fs::path &wallet_path, void *callbackDataIn,
                          bool (*recoverKVcallback)(void *callbackData,
                                                    CDataStream ssKey,
                                                    CDataStream ssValue),
                          std::string &out_backup_filename) {
    return BerkeleyBatch::Recover(wallet_path, callbackDataIn,
                                  recoverKVcallback, out_backup_filename);
}

bool WalletBatch::Recover(const fs::path &wallet_path,
                          std::string &out_backup_filename) {
    // recover without a key filter callback
    // results in recovering all record types
    return WalletBatch::Recover(wallet_path, nullptr, nullptr,
                                out_backup_filename);
}

bool WalletBatch::RecoverKeysOnlyFilter(void *callbackData, CDataStream ssKey,
                                      CDataStream ssValue) {
    auto *dummyWallet = reinterpret_cast<CWallet *>(callbackData);
    CWalletScanState dummyWss;
    std::string strType, strErr;
    bool fReadOK;
    {
        // Required in LoadKeyMetadata():
        LOCK(dummyWallet->cs_wallet);
        fReadOK = ReadKeyValue(dummyWallet, ssKey, ssValue, dummyWss, strType,
                               strErr);
    }
    if (!IsKeyType(strType) && strType != "hdpubkey") {
        return false;
    }
    if (!fReadOK) {
        LogPrintf("WARNING: WalletBatch::Recover skipping %s: %s\n", strType,
                  strErr);
        return false;
    }

    return true;
}

bool WalletBatch::VerifyEnvironment(const fs::path &wallet_path,
                                    std::string &errorStr) {
    return BerkeleyBatch::VerifyEnvironment(wallet_path, errorStr);
}

bool WalletBatch::VerifyDatabaseFile(const fs::path &wallet_path,
                                     std::string &warningStr,
                                     std::string &errorStr) {
    return BerkeleyBatch::VerifyDatabaseFile(wallet_path, warningStr, errorStr,
                                             WalletBatch::Recover);
}

bool WalletBatch::WriteDestData(const CTxDestination &address,
                              const std::string &key,
                              const std::string &value) {
    if (!IsValidDestination(address)) {
        return false;
    }
    return WriteIC(
        std::make_pair(
            std::string("destdata"),
            std::make_pair(EncodeDestination(address), key)),
        value);
}

bool WalletBatch::EraseDestData(const CTxDestination &address,
                              const std::string &key) {
    if (!IsValidDestination(address)) {
        return false;
    }
    return EraseIC(std::make_pair(
        std::string("destdata"),
        std::make_pair(EncodeDestination(address), key)));
}

bool WalletBatch::WriteWalletFlags(const WalletFlag& flags) {
    return WriteIC(std::string("flags"), flags);
}

bool WalletBatch::TxnBegin() {
    return m_batch.TxnBegin();
}

bool WalletBatch::TxnCommit() {
    return m_batch.TxnCommit();
}

bool WalletBatch::TxnAbort() {
    return m_batch.TxnAbort();
}

bool WalletBatch::ReadVersion(int &nVersion) {
    return m_batch.ReadVersion(nVersion);
}

bool WalletBatch::WriteVersion(int nVersion) {
    return m_batch.WriteVersion(nVersion);
}

bool WalletBatch::WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta)
{
  if (!WriteIC(std::make_pair(std::string("keymeta"), hdPubKey.extPubKey.pubkey), keyMeta, false))
        return false;

  if (!WriteIC(std::make_pair(std::string("hdpubkey"), hdPubKey.extPubKey.pubkey), hdPubKey, false))
          return false;
    
  return true;
}

bool WalletBatch::WriteCryptedHDChain(const CHDChain& chain)
{
    if (!WriteIC(std::string("chdchain"), chain))
        return false;

    return true;
}


bool WalletBatch::WriteHDPubKeys(const std::vector<CHDPubKey>& hdPubKey,
                                 std::map<CKeyID, CKeyMetadata>& mapKeyMeta,
                                 std::map<BKeyID, CKeyMetadata>& mapBLSKeyMeta) {
    bool ok = true;
    
    // We will store keys under both "hdpubkey" which is backward compatible and also
    // "blspubkey" which can be used after we drop support for EC pubkeys in new wallets
    
    for (const auto& k : hdPubKey) {
        if (k.extPubKey.IsBLS()) {
            BKeyID id = k.extPubKey.pubkey.GetBLSKeyID();
            auto keyMeta = mapBLSKeyMeta[id];
            if (!m_batch.Write(std::make_pair(std::string("keymeta"), k.extPubKey.pubkey), keyMeta, false)) ok = false;
        } else {
            CKeyID id = k.extPubKey.pubkey.GetKeyID();
            auto keyMeta = mapKeyMeta[id];
            if (!m_batch.Write(std::make_pair(std::string("keymeta"), k.extPubKey.pubkey), keyMeta, false)) ok = false;
        }
        if (!m_batch.Write(std::make_pair(std::string("hdpubkey"), k.extPubKey.pubkey), k, false)) ok = false;
    }
    // Not sure flush is useful here or not
    m_batch.Flush();
    return ok;
}

bool WalletBatch::WritePool(const std::vector<CKeyPool> & keys, int64_t index) {
    bool ok=true;
    for (const auto& k : keys) {
        ++index;
        if (!m_batch.Write(std::make_pair(std::string("pool"), index), k)) ok = false;
    }
    // Not sure flush is useful here or not
    m_batch.Flush();
    return ok;
}

DBErrors WalletBatch::FindLabelledAddresses(std::map<std::string, std::string>& mapLabels) {

    DBErrors result = DBErrors::LOAD_OK;

    try {
        // Get cursor
        Dbc *pcursor = m_batch.GetCursor();
        if (!pcursor) {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true) {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND) {
                break;
            }

            if (ret != 0) {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            std::string strType;
            ssKey >> strType;
            if (strType == "label") {
                std::string label;
                std::string addr;
                ssKey >> label;
                ssValue >> addr;

                mapLabels.insert(std::make_pair(label,addr));
            }
        }
        pcursor->close();
    } catch (const thread_interrupted &) {
        throw;
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}
