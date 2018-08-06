// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <core_io.h>
#include <dstencode.h>
#include <outputtype.h>
#include <interfaces/chain.h>
#include <merkleblock.h>
#include <rpc/server.h>
#include <wallet/rpcwallet.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>
#include <util/fs_util.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <wallet/mnemonic.h>
#include <util/splitstring.h>
#include <devault/coinreward.h>
#include <devault/rewards.h>

#include <string.h> // for memcpy
#include <univalue.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip> // for get_time


static std::string EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (uint8_t c : str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
        } else {
            ret << c;
        }
    }
    return ret.str();
}

std::string DecodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        uint8_t c = str[pos];
        if (c == '%' && pos + 2 < str.length()) {
            c = (((str[pos + 1] >> 6) * 9 + ((str[pos + 1] - '0') & 15)) << 4) |
                ((str[pos + 2] >> 6) * 9 + ((str[pos + 2] - '0') & 15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

bool GetWalletAddressesForKey(const Config &config, CWallet *const pwallet,
                              const CKeyID &keyid, std::string &strAddr,
                              std::string &strLabel) {
    bool fLabelFound = false;
    CKey key;
    pwallet->GetKey(keyid, key);
    for (const auto &dest : GetAllDestinationsForKey(key.GetPubKey())) {
        if (pwallet->mapAddressBook.count(dest)) {
            if (!strAddr.empty()) {
                strAddr += ",";
            }
            strAddr += EncodeDestination(dest, config);
            strLabel = EncodeDumpString(pwallet->mapAddressBook[dest].name);
            fLabelFound = true;
        }
    }
    if (!fLabelFound) {
        strAddr = EncodeDestination(
            GetDestinationForKey(key.GetPubKey(),
                                 g_address_type), 
            config);
    }
    return fLabelFound;
}

static const int64_t TIMESTAMP_MIN = 0;

#ifdef NOT_USED_YET
static void RescanWallet(CWallet &wallet, const WalletRescanReserver &reserver,
                         int64_t time_begin = TIMESTAMP_MIN,
                         bool update = true) {
    int64_t scanned_time = wallet.RescanFromTime(time_begin, reserver, update);
    if (wallet.IsAbortingRescan()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted by user.");
    } else if (scanned_time > time_begin) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Rescan was unable to fully rescan the blockchain. "
                           "Some transactions may be missing.");
    }
}
#endif

UniValue abortrescan(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() > 0) {
        throw std::runtime_error("abortrescan\n"
                                 "\nStops current wallet rescan triggered by "
                                 "an RPC call, e.g. by an importprivkey call.\n"
                                 "\nExamples:\n"
                                 "\nImport a private key\n" +
                                 HelpExampleCli("importprivkey", "\"mykey\"") +
                                 "\nAbort the running wallet rescan\n" +
                                 HelpExampleCli("abortrescan", "") +
                                 "\nAs a JSON-RPC call\n" +
                                 HelpExampleRpc("abortrescan", ""));
    }

    if (!pwallet->IsScanning() || pwallet->IsAbortingRescan()) {
        return false;
    }
    pwallet->AbortRescan();
    return true;
}

void ImportAddress(CWallet *, const CTxDestination &dest,
                   const std::string &strLabel);
void ImportScript(CWallet *const pwallet, const CScript &script,
                  const std::string &strLabel, bool isRedeemScript) 
    EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet) {
    if (!isRedeemScript && ::IsMine(*pwallet, script) == ISMINE_SPENDABLE) {
        throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the "
                                             "private key for this address or "
                                             "script");
    }

    pwallet->MarkDirty();

    if (!pwallet->HaveWatchOnly(script) &&
        !pwallet->AddWatchOnly(script, 0 /* nCreateTime */)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
    }

    if (isRedeemScript) {
        if (!pwallet->HaveCScript(script) && !pwallet->AddCScript(script)) {
            throw JSONRPCError(RPC_WALLET_ERROR,
                               "Error adding p2sh redeemScript to wallet");
        }
        ImportAddress(pwallet, CScriptID(script), strLabel);
    } else {
        CTxDestination destination;
        if (ExtractDestination(script, destination)) {
            pwallet->SetAddressBook(destination, strLabel, "receive");
        }
    }
}

void ImportAddress(CWallet *const pwallet, const CTxDestination &dest,
                   const std::string &strLabel) 
    EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet) {
    CScript script = GetScriptForDestination(dest);
    ImportScript(pwallet, script, strLabel, false);
    // add to address book or update label
    if (IsValidDestination(dest)) {
        pwallet->SetAddressBook(dest, strLabel, "receive");
    }
}

UniValue importaddress(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 4) {
        throw std::runtime_error(
            "importaddress \"address\" ( \"label\" rescan p2sh )\n"
            "\nAdds a script (in hex) or address that can be watched as if it "
            "were in your wallet but cannot be used to spend. Requires a new "
            "wallet backup.\n"
            "\nArguments:\n"
            "1. \"script\"           (string, required) The hex-encoded script "
            "(or address)\n"
            "2. \"label\"            (string, optional, default=\"\") An "
            "optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan "
            "the wallet for transactions\n"
            "4. p2sh                 (boolean, optional, default=false) Add "
            "the P2SH version of the script as well\n"
            "\nNote: This call can take minutes to complete if rescan is true, "
            "during that time, other rpc calls\n"
            "may report that the imported address exists but related "
            "transactions are still missing, leading to temporarily "
            "incorrect/bogus balances and unspent outputs until rescan "
            "completes.\n"
            "If you have the full public key, you should call importpubkey "
            "instead of this.\n"
            "\nNote: If you import a non-standard raw script in hex form, "
            "outputs sending to it will be treated\n"
            "as change, and not show up in many RPCs.\n"
            "\nExamples:\n"
            "\nImport a script with rescan\n" +
            HelpExampleCli("importaddress", "\"myscript\"") +
            "\nImport using a label without rescan\n" +
            HelpExampleCli("importaddress", R"("myscript" "testing" false)") +
            "\nAs a JSON-RPC call\n" +
            HelpExampleRpc("importaddress",
                           R"("myscript", "testing", false)"));
    }

    std::string strLabel;
    if (!request.params[1].isNull()) {
        strLabel = request.params[1].get_str();
    }

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull()) {
        fRescan = request.params[2].get_bool();
    }

    if (fRescan && fPruneMode) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Rescan is disabled in pruned mode");
    }

    WalletRescanReserver reserver(pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(
            RPC_WALLET_ERROR,
            "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    // Whether to import a p2sh version, too
    bool fP2SH = false;
    if (!request.params[3].isNull()) {
        fP2SH = request.params[3].get_bool();
    }

    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);

        CTxDestination dest = DecodeDestination(request.params[0].get_str(),
                                                config.GetChainParams());
        if (IsValidDestination(dest)) {
            if (fP2SH) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                                   "Cannot use the p2sh flag with an address - "
                                   "use a script instead");
            }
            ImportAddress(pwallet, dest, strLabel);
        } else if (IsHex(request.params[0].get_str())) {
            std::vector<uint8_t> data(ParseHex(request.params[0].get_str()));
            ImportScript(pwallet, CScript(data.begin(), data.end()), strLabel,
                         fP2SH);
        } else {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               "Invalid DeVault address or script");
        }
    }
    if (fRescan) {
        pwallet->RescanFromTime(TIMESTAMP_MIN, reserver, true /* update */);
        pwallet->ReacceptWalletTransactions();
    }

    return NullUniValue;
}

UniValue importprunedfunds(const Config &config,
                           const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 2) {
        throw std::runtime_error(
            "importprunedfunds\n"
            "\nImports funds without rescan. Corresponding address or script "
            "must previously be included in wallet. Aimed towards pruned "
            "wallets. The end-user is responsible to import additional "
            "transactions that subsequently spend the imported outputs or "
            "rescan after the point in the blockchain the transaction is "
            "included.\n"
            "\nArguments:\n"
            "1. \"rawtransaction\" (string, required) A raw transaction in hex "
            "funding an already-existing address in wallet\n"
            "2. \"txoutproof\"     (string, required) The hex output from "
            "gettxoutproof that contains the transaction\n");
    }

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    uint256 txid = tx.GetId();
    CWalletTx wtx(pwallet, MakeTransactionRef(std::move(tx)));

    CDataStream ssMB(ParseHexV(request.params[1], "proof"), SER_NETWORK,
                     PROTOCOL_VERSION);
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    // Search partial merkle tree in proof for our transaction and index in
    // valid block
    std::vector<uint256> vMatch;
    std::vector<size_t> vIndex;
    size_t txnIndex = 0;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) ==
        merkleBlock.header.hashMerkleRoot) {
        //LOCK(cs_main);
        auto locked_chain = pwallet->chain().lock();

        if (!mapBlockIndex.count(merkleBlock.header.GetHash()) ||
            !chainActive.Contains(
                mapBlockIndex[merkleBlock.header.GetHash()])) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               "Block not found in chain");
        }

        std::vector<uint256>::const_iterator it;
        if ((it = std::find(vMatch.begin(), vMatch.end(), txid)) ==
            vMatch.end()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               "Transaction given doesn't exist in proof");
        }

        txnIndex = vIndex[it - vMatch.begin()];
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Something wrong with merkleblock");
    }

    wtx.nIndex = txnIndex;
    wtx.hashBlock = merkleBlock.header.GetHash();

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    if (pwallet->IsMine(*wtx.tx)) {
        pwallet->AddToWallet(wtx, false);
        return NullUniValue;
    }

    throw JSONRPCError(
        RPC_INVALID_ADDRESS_OR_KEY,
        "No addresses in wallet correspond to included transaction");
}

UniValue removeprunedfunds(const Config &config,
                           const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "removeprunedfunds \"txid\"\n"
            "\nDeletes the specified transaction from the wallet. Meant for "
            "use with pruned wallets and as a companion to importprunedfunds. "
            "This will effect wallet balances.\n"
            "\nArguments:\n"
            "1. \"txid\"           (string, required) The hex-encoded id of "
            "the transaction you are deleting\n"
            "\nExamples:\n" +
            HelpExampleCli("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1"
                                                "ce581bebf46446a512166eae762873"
                                                "4ea0a5\"") +
            "\nAs a JSON-RPC call\n" +
            HelpExampleRpc("removeprunedfunds",
                           "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166"
                           "eae7628734ea0a5\""));
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    TxId txid;
    txid.SetHex(request.params[0].get_str());
    std::vector<TxId> txIds;
    txIds.push_back(txid);
    std::vector<TxId> txIdsOut;

    if (pwallet->ZapSelectTx(txIds, txIdsOut) != DBErrors::LOAD_OK) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Could not properly delete the transaction.");
    }

    if (txIdsOut.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Transaction does not exist in wallet.");
    }

    return NullUniValue;
}

UniValue importpubkey(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 4) {
        throw std::runtime_error(
            "importpubkey \"pubkey\" ( \"label\" rescan )\n"
            "\nAdds a public key (in hex) that can be watched as if it were in "
            "your wallet but cannot be used to spend. Requires a new wallet "
            "backup.\n"
            "\nArguments:\n"
            "1. \"pubkey\"           (string, required) The hex-encoded public "
            "key\n"
            "2. \"label\"            (string, optional, default=\"\") An "
            "optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan "
            "the wallet for transactions\n"
            "\nNote: This call can take minutes to complete if rescan is true, "
            "during that time, other rpc calls\n"
            "may report that the imported pubkey exists but related "
            "transactions are still missing, leading to temporarily "
            "incorrect/bogus balances and unspent outputs until rescan "
            "completes.\n"
            "\nExamples:\n"
            "\nImport a public key with rescan\n" +
            HelpExampleCli("importpubkey", "\"mypubkey\"") +
            "\nImport using a label without rescan\n" +
            HelpExampleCli("importpubkey", R"("mypubkey" "testing" false)") +
            "\nAs a JSON-RPC call\n" +
            HelpExampleRpc("importpubkey", R"("mypubkey", "testing", false)"));
    }

    std::string strLabel;
    if (!request.params[1].isNull()) {
        strLabel = request.params[1].get_str();
    }

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull()) {
        fRescan = request.params[2].get_bool();
    }

    if (fRescan && fPruneMode) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Rescan is disabled in pruned mode");
    }

    WalletRescanReserver reserver(pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(
            RPC_WALLET_ERROR,
            "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    if (!IsHex(request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Pubkey must be a hex string");
    }
    std::vector<uint8_t> data(ParseHex(request.params[0].get_str()));
    CPubKey pubKey(data.begin(), data.end());
    if (!pubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Pubkey is not a valid public key");
    }

    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);

        ImportAddress(pwallet, pubKey.GetKeyID(), strLabel);
        ImportScript(pwallet, GetScriptForRawPubKey(pubKey), strLabel, false);
    }
    if (fRescan) {
        pwallet->RescanFromTime(TIMESTAMP_MIN, reserver, true /* update */);
        pwallet->ReacceptWalletTransactions();
    }

    return NullUniValue;
}

/* not used - keep for ABC merging
UniValue importwallet(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet). Requires "
            "a new wallet backup to include imported keys.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n" +
            HelpExampleCli("dumpwallet", "\"test\"") + "\nImport the wallet\n" +
            HelpExampleCli("importwallet", "\"test\"") +
            "\nImport using the json rpc call\n" +
            HelpExampleRpc("importwallet", "\"test\""));
    }

    if (fPruneMode) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Importing wallets is disabled in pruned mode");
    }

    WalletRescanReserver reserver(pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(
            RPC_WALLET_ERROR,
            "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    int64_t nTimeBegin = 0;
    bool fGood = true;
    {
        LOCK2(cs_main, pwallet->cs_wallet);

        EnsureWalletIsUnlocked(pwallet);

        std::ifstream file;
        file.open(request.params[0].get_str().c_str(),
                  std::ios::in | std::ios::ate);
        if (!file.is_open()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Cannot open wallet dump file");
        }
        nTimeBegin = chainActive.Tip()->GetBlockTime();

        int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
        file.seekg(0, file.beg);

        // Use uiInterface.ShowProgress instead of pwallet.ShowProgress because
        // pwallet.ShowProgress has a cancel button tied to AbortRescan which we
        // don't want for this progress bar showing the import progress.
        // uiInterface.ShowProgress does not have a cancel button.

        // show progress dialog in GUI
        uiInterface.ShowProgress(
            strprintf("%s " + _("Importing..."), pwallet->GetDisplayName()), 0,
            false);
        while (file.good()) {
            uiInterface.ShowProgress(
                "",
                std::max(1, std::min(99, (int)(((double)file.tellg() /
                                                (double)nFilesize) *
                                               100))),
                false);
            std::string line;
            std::getline(file, line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::vector<std::string> vstr;
            boost::split(vstr, line, boost::is_any_of(" "));
            if (vstr.size() < 2) {
                continue;
            }
            CKey key = DecodeSecret(vstr[0]);
            if (key.IsValid()) {
                CPubKey pubkey = key.GetPubKey();
                assert(key.VerifyPubKey(pubkey));
                CKeyID keyid = pubkey.GetID();
                if (pwallet->HaveKey(keyid)) {
                    pwallet->WalletLogPrintf(
                        "Skipping import of %s (key already present)\n",
                        EncodeDestination(keyid, config));
                    continue;
                }
                int64_t nTime = DecodeDumpTime(vstr[1]);
                std::string strLabel;
                bool fLabel = true;
                for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
                    if (boost::algorithm::starts_with(vstr[nStr], "#")) {
                        break;
                    }
                    if (vstr[nStr] == "change=1") {
                        fLabel = false;
                    }
                    if (vstr[nStr] == "reserve=1") {
                        fLabel = false;
                    }
                    if (boost::algorithm::starts_with(vstr[nStr], "label=")) {
                        strLabel = DecodeDumpString(vstr[nStr].substr(6));
                        fLabel = true;
                    }
                }
                pwallet->WalletLogPrintf("Importing %s...\n",
                                         EncodeDestination(keyid, config));
                if (!pwallet->AddKeyPubKey(key, pubkey)) {
                    fGood = false;
                    continue;
                }
                pwallet->mapKeyMetadata[keyid].nCreateTime = nTime;
                if (fLabel) {
                    pwallet->SetAddressBook(keyid, strLabel, "receive");
                }
                nTimeBegin = std::min(nTimeBegin, nTime);
            } else if (IsHex(vstr[0])) {
                std::vector<uint8_t> vData(ParseHex(vstr[0]));
                CScript script = CScript(vData.begin(), vData.end());
                if (pwallet->HaveCScript(script)) {
                    pwallet->WalletLogPrintf(
                        "Skipping import of %s (script already present)\n",
                        vstr[0]);
                    continue;
                }
                if (!pwallet->AddCScript(script)) {
                    pwallet->WalletLogPrintf("Error importing script %s\n",
                                             vstr[0]);
                    fGood = false;
                    continue;
                }
                int64_t birth_time = DecodeDumpTime(vstr[1]);
                if (birth_time > 0) {
                    pwallet->m_script_metadata[CScriptID(script)].nCreateTime =
                        birth_time;
                    nTimeBegin = std::min(nTimeBegin, birth_time);
                }
            }
        }
        file.close();

        // hide progress dialog in GUI
        uiInterface.ShowProgress("", 100, false);
        pwallet->UpdateTimeFirstKey(nTimeBegin);
    }
    // hide progress dialog in GUI
    uiInterface.ShowProgress("", 100, false);
    RescanWallet(*pwallet, reserver, nTimeBegin, false);  // update
    pwallet->MarkDirty();

    if (!fGood) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Error adding some keys/scripts to wallet");
    }

    return NullUniValue;
}
*/

UniValue dumpprivkey(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "dumpprivkey \"address\"\n"
            "\nReveals the private key corresponding to 'address'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"address\"   (string, required) The DeVault address for the "
            "private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            HelpExampleCli("importprivkey", "\"mykey\"") +
            HelpExampleRpc("dumpprivkey", "\"myaddress\""));
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    std::string strAddress = request.params[0].get_str();
    CTxDestination dest =
        DecodeDestination(strAddress, config.GetChainParams());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Invalid DeVault address");
    }
    bool isBLS = true;
    bool isEC = true;
    CKeyID *keyID0;
    BKeyID *keyID1;
    try {
        keyID0 = &std::get<CKeyID>(dest);
    }
    catch (...) { isEC = false; }
    try {
        keyID1 = &std::get<BKeyID>(dest);
    }
    catch (...) { isBLS = false; }
    if (!keyID0) isEC = false;
    if (!keyID1) isBLS = false;

    if (isBLS || isEC) {

        CKey vchSecret;
        if (isEC) {
            if (!pwallet->GetKey(*keyID0, vchSecret)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " +
                                   strAddress + " is not known");
            }
        } else {
            if (!pwallet->GetKey(*keyID1, vchSecret)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " +
                                   strAddress + " is not known");
            }

        }
        
        std::string bech32string = EncodeSecret(vchSecret);
        
        UniValue keys(UniValue::VOBJ);
        keys.pushKV("bech32",bech32string);
        return keys;
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "DeVault address decoding problem");
    }
    return NullUniValue;

}

UniValue dumpwallet(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format to a "
            "server-side file. This does not allow overwriting existing "
            "files.\n"
            "Imported scripts are included in the dumpsfile\n"
            "Note that if your wallet contains keys which are not derived from "
            "your HD seed (e.g. imported keys), these are not covered by\n"
            "only backing up the seed itself, and must be backed up too (e.g. "
            "ensure you back up the whole dumpfile).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename with path "
            "(either absolute or relative to devaultd)\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"filename\" : {        (string) The filename with full "
            "absolute path\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpwallet", "\"test\"") +
            HelpExampleRpc("dumpwallet", "\"test\""));

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    fs::path filepath = request.params[0].get_str();
    filepath = fs::absolute(filepath);

    /**
     * Prevent arbitrary files from being overwritten. There have been reports
     * that users have overwritten wallet files this way:
     * https://github.com/bitcoin/bitcoin/issues/9934
     * It may also avoid other security issues.
     */
    if (fs::exists(filepath)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           filepath.string() + " already exists. If you are "
                                               "sure this is what you want, "
                                               "move it out of the way first");
    }

    std::ofstream file;
    file.open(filepath.string().c_str());
    if (!file.is_open()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Cannot open wallet dump file");
    }

    std::map<CTxDestination, int64_t> mapKeyBirth;
    std::map<CTxDestination, int64_t> mapBLSKeyBirth;
    const std::map<CKeyID, int64_t> &mapKeyPool = pwallet->GetAllReserveKeys();
    pwallet->GetKeyBirthTimes(*locked_chain, mapKeyBirth);

    std::set<CScriptID> scripts = pwallet->GetCScripts();
    // TODO: include scripts in GetKeyBirthTimes() output instead of separate

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID>> vKeyBirth;
    for (const auto &entry : mapKeyBirth) {
        if (const CKeyID *keyID = &std::get<CKeyID>(entry.first)) {
            // set and test
            vKeyBirth.emplace_back(entry.second, *keyID);
        }
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    pwallet->GetBLSKeyBirthTimes(*locked_chain, mapBLSKeyBirth);
    const std::map<BKeyID, int64_t> &mapBLSKeyPool = pwallet->GetAllBLSReserveKeys();
    std::vector<std::pair<int64_t, BKeyID>> vBLSKeyBirth;

    for (const auto &entry : mapBLSKeyBirth) {
        if (const BKeyID *keyID = &std::get<BKeyID>(entry.first)) {
            // set and test
            vBLSKeyBirth.emplace_back(entry.second, *keyID);
        }
    }
    mapBLSKeyBirth.clear();
    std::sort(vBLSKeyBirth.begin(), vBLSKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by DeVault %s\n", CLIENT_BUILD);
    file << strprintf("# * Created on %s\n", FormatISO8601DateTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n",
                      chainActive.Height(),
                      chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n",
                      FormatISO8601DateTime(chainActive.Tip()->GetBlockTime()));
    file << "\n";

    // add the base58check encoded extended master if the wallet uses HD
    CHDChain hdChain;
    SecureString ssMnemonic;
    pwallet->GetDecryptedHDChain(hdChain);
     
    if (!pwallet->GetMnemonic(hdChain, ssMnemonic))
        throw std::runtime_error(std::string(__func__) + ": Get Mnemonic failed");
      
    file << "# mnemonic: " << ssMnemonic << "\n";

    SecureVector vchSeed = hdChain.GetSeed();
    file << "# HD seed: " << HexStr(vchSeed) << "\n\n";

    CHDAccount acc;
    
    if (pwallet->HasBLSKeys()) {
        if(hdChain.GetAccount(BLS_ACCOUNT, acc)) {
            file << "# BLS external chain counter: " << acc.nExternalChainCounter << "\n";
            file << "# BLS internal chain counter: " << acc.nInternalChainCounter << "\n\n";
        } else {
            file << "# WARNING: BLS_ACCOUNT IS MISSING!" << "\n\n";
        }
    }

    if(hdChain.GetAccount(0, acc)) {
        file << "# EC external chain counter: " << acc.nExternalChainCounter << " for Account 0 \n";
        file << "# EC internal chain counter: " << acc.nInternalChainCounter << " for Account 0 \n\n";
    } else {
        file << "# Only BLS keys exist in wallet!" << "\n\n";
    }

    // Rather than just print out the keypaths in a somewhat random order
    // Put into a map so they can be ordered by hdkeypath
    // after map is created, print out
    std::map<uint64_t,std::string> keypaths;
    
    for (const auto& it : vKeyBirth) {
        const CKeyID &keyid = it.second;
        std::string strTime = FormatISO8601DateTime(it.first);
        std::string strAddr = EncodeDestination(keyid);
        CKey key;
        std::string fullstr="";
        if (pwallet->GetKey(keyid, key)) {
          fullstr += strprintf("%s %s ", EncodeSecret(key),strTime);
          if (pwallet->mapAddressBook.count(keyid)) {
            fullstr += strprintf("label=%s",EncodeDumpString(pwallet->mapAddressBook[keyid].name));
          } else if (mapKeyPool.count(keyid)) {
            fullstr += "reserve=1";
          } else {
            fullstr += "change=1";
          }
          {
            std::string hdkeypath="";
            if (pwallet->mapHdPubKeys.count(keyid)) hdkeypath += pwallet->mapHdPubKeys[keyid].GetKeyPath();
            fullstr += strprintf(" # addr=%s,hdkeypath=%s\n", strAddr, hdkeypath);

            // This is to make the output keys sorted by path
            // Put into a std::map based on numeric path that will be stored in sorted order
            std::vector<std::string> vParts;
            Split(vParts, hdkeypath, "/");
            uint64_t keynum = std::stoi(vParts.back());
            vParts.pop_back();
            uint64_t ext = std::stoi(vParts.back());
            uint64_t order = ext*100000+keynum;
            keypaths.insert(make_pair(order,fullstr));
          }
        }
    }

   std::map<uint64_t,std::string> blskeypaths;
    

   for (const auto& it : vBLSKeyBirth) {
        const BKeyID &keyid = it.second;
        std::string strTime = FormatISO8601DateTime(it.first);
        std::string strAddr = EncodeDestination(keyid);
        CKey key;
        std::string fullstr="";
        if (pwallet->GetKey(keyid, key)) {
          fullstr += strprintf("%s %s ", EncodeSecret(key),strTime);
          if (pwallet->mapAddressBook.count(keyid)) {
            fullstr += strprintf("label=%s",EncodeDumpString(pwallet->mapAddressBook[keyid].name));
          } else if (mapBLSKeyPool.count(keyid)) {
            fullstr += "reserve=1";
          } else {
            fullstr += "change=1";
          }
          {
            std::string hdkeypath="";
            if (pwallet->mapBLSPubKeys.count(keyid)) hdkeypath += pwallet->mapBLSPubKeys[keyid].GetKeyPath();
            fullstr += strprintf(" # addr=%s,hdkeypath=%s\n", strAddr, hdkeypath);

            // This is to make the output keys sorted by path
            // Put into a std::map based on numeric path that will be stored in sorted order
            std::vector<std::string> vParts;
            Split(vParts, hdkeypath, "/");
            uint64_t keynum = std::stoi(vParts.back());
            vParts.pop_back();
            uint64_t ext = std::stoi(vParts.back());
            uint64_t order = ext*100000+keynum;
            blskeypaths.insert(make_pair(order,fullstr));
          }
        }
    }

    // Print sorted map
    for (const auto& s : blskeypaths) {
      file << s.second;
    }
    
    for (const auto& s : keypaths) {
      file << s.second;
    }
    
    
    file << "\n";
    for (const CScriptID &scriptid : scripts) {
        CScript script;
        std::string create_time = "0";
        std::string address = EncodeDestination(scriptid);
        // get birth times for scripts with metadata
        auto it = pwallet->m_script_metadata.find(scriptid);
        if (it != pwallet->m_script_metadata.end()) {
            create_time = FormatISO8601DateTime(it->second.nCreateTime);
        }
        if (pwallet->GetCScript(scriptid, script)) {
            file << strprintf("%s %s script=1",
                              HexStr(script.begin(), script.end()),
                              create_time);
            file << strprintf(" # addr=%s\n", address);
        }
    }
    file << "\n";
    file << "# End of dump\n";
    file.close();

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.string());

    return reply;
}

UniValue dumpphrase(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp)
        throw std::runtime_error(
            "dumpphrase\n"
            "\nShows 12 word phrase - wallet must be unlocked first - WARNING DO NOT SHARE This output\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpphrase","") +
            HelpExampleRpc("dumpphrase",""));

    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    // add the base58check encoded extended master if the wallet uses HD
    CHDChain hdChain;
    SecureString ssMnemonic;
    pwallet->GetDecryptedHDChain(hdChain);
     
    if (!pwallet->GetMnemonic(hdChain, ssMnemonic))
        throw std::runtime_error(std::string(__func__) + ": Get Mnemonic failed");

    std::string phrase(ssMnemonic);
    UniValue reply(UniValue::VOBJ);
    reply.pushKV("WARNING: DO NOT SHARE THIS PHRASE WITH ANYONE", phrase);

    return reply;
}

int64_t GetImportTimestamp(const UniValue &data, int64_t now) {
    if (data.exists("timestamp")) {
        const UniValue &timestamp = data["timestamp"];
        if (timestamp.isNum()) {
            return timestamp.get_int64();
        } else if (timestamp.isStr() && timestamp.get_str() == "now") {
            return now;
        }
        throw JSONRPCError(RPC_TYPE_ERROR,
                           strprintf("Expected number or \"now\" timestamp "
                                     "value for key. got type %s",
                                     uvTypeName(timestamp.type())));
    }
    throw JSONRPCError(RPC_TYPE_ERROR,
                       "Missing required timestamp field for key");
}

static UniValue getmyrewardinfo(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
                                 "getmyrewardinfo \"filename\"\n"
                                 "\nReturns status for all of my valid reward UTXOs.\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("getmyrewardinfo","")
                                 + HelpExampleRpc("getmyrewardinfo",""));
    
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    const int nMinRewardInCoins = config.GetChainParams().GetConsensus().nMinReward.toIntCoins();
    const int nMinBlocks = config.GetChainParams().GetConsensus().nMinRewardBlocks;
    const int nBlocksPerYear = config.GetChainParams().GetConsensus().nBlocksPerYear;
    const int nPowTargetSpacing = config.GetChainParams().GetConsensus().nPowTargetSpacing;
    const int nMaxYearIndex = config.GetChainParams().GetConsensus().nPerCentPerYear.size()-1;

    UniValue result(UniValue::VARR);
    std::vector<CRewardValue> rewards = prewards->GetOrderedRewards();
  
    UniValue total(UniValue::VOBJ);
    total.push_back(Pair("Total Number of Rewards", (int)prewards->GetNumberOfCandidates()));
    result.push_back(total);

    std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Minimum balances for 1 month payout given current reward % of 15,12,9,7,5
    std::vector<double> nMinBalance;
    for (auto pc :  config.GetChainParams().GetConsensus().nPerCentPerYear) {
        double val = nMinRewardInCoins * 12 * 100.0 / pc;
        nMinBalance.push_back(val);
    }
    
    for (auto& val : rewards) {
        isminetype mine = ::IsMine(*pwallet, val.scriptPubKey());
        if (mine == ISMINE_SPENDABLE && val.IsActive()) {
            const uint32_t nMyHeight = val.GetHeight();
            UniValue delta(UniValue::VOBJ);
            delta.push_back(Pair("Addr",GetAddrFromTxOut(val.GetTxOut())));
            delta.push_back(Pair("Value",ValueFromAmount(val.GetValue())));
            delta.push_back(Pair("created at Height",(int)val.GetCreationHeight()));
            if (val.GetPayCount() > 0) {
                delta.push_back(Pair("Last paid at Height",(int)val.GetHeight()));
            }
            delta.push_back(Pair("paid times",(int)val.GetPayCount()));

            int nNumOlder = 0;
            for (auto& inner_val : rewards) {
                if ((inner_val.GetHeight() < nMyHeight) && inner_val.IsActive()) nNumOlder++;
            }

            delta.push_back(Pair("reward candidates older than this one",nNumOlder));

            // Use nMinBlocks unless there are more older candidates that need to get paid out
            int nYear = nMyHeight/nBlocksPerYear;
            if (nYear > nMaxYearIndex) nYear = nMaxYearIndex;
            // Check if balance is below minimum required for 1 month payout,
            // if it is, then extend the needed number of blocks for payout
            int neededBlocks = nMinBlocks;
            if (val.GetValue().toIntCoins() < (int)nMinBalance[nYear]) {
                neededBlocks *= std::ceil(nMinBalance[nYear]/val.GetValue().toIntCoins());
            }
            // In event of very large number of older payouts that need to be made,
            // extend by even more blocks
            nNumOlder = std::max(nNumOlder,neededBlocks);
            int payoutHeight = nMyHeight+nNumOlder;
            int blocksNeeded = payoutHeight - chainActive.Height();
            // Use blocksNeeded to estimate date
            std::time_t nexttime = cftime + blocksNeeded*nPowTargetSpacing;
            delta.push_back(Pair("estimated reward block", payoutHeight));
            delta.push_back(Pair("estimated next reward date", FormatISO8601Date(nexttime)));
            
            result.push_back(delta);
        }
    }
#else
    UniValue result(UniValue::VARR);
    throw JSONRPCError(RPC_WALLET_ERROR, "Wallet required for this function to provide information");
#endif
    return result;
}

// clang-format off
static const ContextFreeRPCCommand commands[] = {
    //  category            name                        actor (function)          argNames
    //  ------------------- ------------------------    ----------------------    ----------
    { "wallet",             "abortrescan",              abortrescan,              {} },
    { "wallet",             "dumpprivkey",              dumpprivkey,              {"address"}  },
    { "wallet",             "dumpwallet",               dumpwallet,               {"filename"} },
    { "wallet",             "dumpphrase",               dumpphrase,               {} },
    { "wallet",             "importaddress",            importaddress,            {"address","label","rescan","p2sh"} },
    { "wallet",             "importprunedfunds",        importprunedfunds,        {"rawtransaction","txoutproof"} },
    { "wallet",             "importpubkey",             importpubkey,             {"pubkey","label","rescan"} },
    { "wallet",             "removeprunedfunds",        removeprunedfunds,        {"txid"} },
    { "wallet",             "getmyrewardinfo",        getmyrewardinfo,            {} },
};
// clang-format on

void RegisterDumpRPCCommands(CRPCTable &t) {
    for (auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
