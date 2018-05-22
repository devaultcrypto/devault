// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/util.h>
#include <core_io.h>
#include <clientversion.h>
#include <config.h>
#include <chain.h>
#include <dstencode.h>
#include <init.h>
#include <net.h>
#include <netbase.h>
#include <utxo_functions.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <timedata.h>
#include <txmempool.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <utxo_functions.h>
#include <validation.h>
#include <devault/coinreward.h>
#include <devault/rewards.h>

#include <wallet/misc.h>
#ifdef ENABLE_WALLET
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#endif
#include <warnings.h>

#include <wallet/misc.h>

#ifndef ENABLE_WALLET
#error "ENABLE_WALLET define required for this file"
#endif

#include <univalue.h>

#include <iostream>
#include <fstream>
#include <cstdint>
#ifdef HAVE_MALLOC_INFO
#include <malloc.h>
#endif

/**
 * @note Do not add or change anything in the information returned by this
 * method. `getinfo` exists for backwards-compatibility only. It combines
 * information from wildly different sources in the program, which is a mess,
 * and is thus planned to be deprecated eventually.
 *
 * Based on the source of the information, new information should be added to:
 * - `getblockchaininfo`,
 * - `getnetworkinfo` or
 * - `getwalletinfo`
 *
 * Or alternatively, create a specific query method for the information.
 **/
static UniValue getinfo(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getinfo\n"
            "\nDEPRECATED. Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total DeVault "
            "balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of "
            "blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of "
            "connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used "
            "by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
            "  \"testnet\": true|false,      (boolean) if the server is using "
            "testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds "
            "since Unix epoch) of the oldest pre-generated key in the key "
            "pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are "
            "pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in "
            "seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is "
            "unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set "
            "in " +
            CURRENCY_UNIT +
            "/kB\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for "
            "non-free transactions in " +
            CURRENCY_UNIT +
            "/kB\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getinfo", "") + HelpExampleRpc("getinfo", ""));
    }
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", CLIENT_VERSION);
    obj.pushKV("protocolversion", PROTOCOL_VERSION);
    if (pwallet) {
        obj.pushKV("walletversion", pwallet->GetVersion());
        obj.pushKV("balance", ValueFromAmount(pwallet->GetBalance()));
    }
    obj.pushKV("blocks", (int)chainActive.Height());
    obj.pushKV("timeoffset", GetTimeOffset());
    if (g_connman) {
        obj.pushKV("connections",
                   (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL));
    }
    obj.pushKV("proxy", (proxy.IsValid() ? proxy.proxy.ToStringIPPort()
                                         : std::string()));
    obj.pushKV("difficulty", double(GetDifficulty(chainActive.Tip())));
    obj.pushKV("testnet", config.GetChainParams().NetworkIDString() ==
                              CBaseChainParams::TESTNET);
    if (pwallet) {
        LOCK(pwallet->cs_wallet);
        obj.pushKV("keypoololdest", pwallet->GetOldestKeyPoolTime());
        obj.pushKV("keypoolsize", (int)pwallet->GetKeyPoolSize());
    }
    if (pwallet) {
        obj.pushKV("unlocked_until", pwallet->nRelockTime);
    }
    obj.pushKV("paytxfee", ValueFromAmount(payTxFee.GetFeePerK()));
    obj.pushKV("relayfee",
               ValueFromAmount(config.GetMinFeePerKB().GetFeePerK()));
    obj.pushKV("errors", GetWarnings("statusbar"));
    return obj;
}


class DescribeAddressVisitor  : public std::variant<UniValue> {
public:
    CWallet *const pwallet;

    explicit DescribeAddressVisitor(CWallet *_pwallet) : pwallet(_pwallet) {}

    UniValue operator()(const CNoDestination &dest) const {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        if (pwallet && pwallet->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", true);
        }
        return obj;
    }

    UniValue operator()(const BKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        if (pwallet && pwallet->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
        }
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        obj.pushKV("isscript", true);
        if (pwallet && pwallet->GetCScript(scriptID, subscript)) {
            std::vector<CTxDestination> addresses;
            txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.pushKV("script", GetTxnOutputType(whichType));
            obj.pushKV("hex", HexStr(subscript.begin(), subscript.end()));
            UniValue a(UniValue::VARR);
            for (const CTxDestination &addr : addresses) {
                a.push_back(EncodeDestination(addr));
            }
            obj.pushKV("addresses", a);
            if (whichType == TX_MULTISIG) {
                obj.pushKV("sigsrequired", nRequired);
            }
        }
        return obj;
    }
};

static UniValue validateaddress(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "validateaddress \"address\"\n"
            "\nReturn information about the given DeVault address.\n"
            "\nArguments:\n"
            "1. \"address\"     (string, required) The DeVault address to "
            "validate\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,       (boolean) If the address is "
            "valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"address\", (string) The DeVault address "
            "validated\n"
            "  \"scriptPubKey\" : \"hex\",       (string) The hex encoded "
            "scriptPubKey generated by the address\n"
            "  \"ismine\" : true|false,        (boolean) If the address is "
            "yours or not\n"
            "  \"iswatchonly\" : true|false,   (boolean) If the address is "
            "watchonly\n"
            "  \"isscript\" : true|false,      (boolean) If the key is a "
            "script\n"
            "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the "
            "raw public key\n"
            "  \"iscompressed\" : true|false,  (boolean) If the address is "
            "compressed\n"
            "  \"account\" : \"account\"         (string) DEPRECATED. The "
            "account associated with the address, \"\" is the default account\n"
            "  \"timestamp\" : timestamp,        (number, optional) The "
            "creation time of the key if available in seconds since epoch (Jan "
            "1 1970 GMT)\n"
            "  \"hdkeypath\" : \"keypath\"       (string, optional) The HD "
            "keypath if the key is HD and available\n"
            "  \"hdchainid\" : \"<hash160>\" (string, optional) The "
            "Hash160 of the HD master pubkey\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("validateaddress",
                           "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"") +
            HelpExampleRpc("validateaddress",
                           "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\""));
    }
    
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#endif

    CTxDestination dest = DecodeDestination(request.params[0].get_str(), config.GetChainParams());
    bool isValid = IsValidDestination(dest);

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("isvalid", isValid);
    if (isValid) {
        std::string currentAddress = EncodeDestination(dest);
        ret.pushKV("address", currentAddress);

        CScript scriptPubKey = GetScriptForDestination(dest);
        ret.pushKV("scriptPubKey",
                   HexStr(scriptPubKey.begin(), scriptPubKey.end()));

#ifdef ENABLE_WALLET
        isminetype mine = pwallet ? IsMine(*pwallet, dest) : ISMINE_NO;
        ret.pushKV("ismine", (mine & ISMINE_SPENDABLE) ? true : false);
        ret.pushKV("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true : false);
        UniValue detail = std::visit(DescribeAddressVisitor(pwallet), dest);
        ret.pushKVs(detail);
        if (pwallet && pwallet->mapAddressBook.count(dest)) {
            ret.pushKV("account", pwallet->mapAddressBook[dest].name);
        }
        if (pwallet) {
            const CKeyMetadata *meta = nullptr;
            CKeyID *key_id;
            BKeyID *key1_id;
            
            try {
                key_id = &std::get<CKeyID>(dest);
                auto it = pwallet->mapKeyMetadata.find(*key_id);
                if (it != pwallet->mapKeyMetadata.end()) meta = &it->second;
                // inside if
                if (meta) {
                    CHDChain hdChain;
                    pwallet->GetDecryptedHDChain(hdChain);
                    if (!key_id->IsNull() && pwallet->mapHdPubKeys.count(*key_id)) {
                        ret.push_back(Pair("hdkeypath", pwallet->mapHdPubKeys[*key_id].GetKeyPath()));
                        ret.push_back(Pair("hdchainid", hdChain.GetID().GetHex()));
                    }
                }
            }
            catch (...) { ; }
            try {
                key1_id = &std::get<BKeyID>(dest);
                auto it1 = pwallet->mapBLSKeyMetadata.find(*key1_id);
                if (it1 != pwallet->mapBLSKeyMetadata.end()) meta = &it1->second;
                // inside if
                if (meta) {
                    CHDChain hdChain;
                    pwallet->GetDecryptedHDChain(hdChain);
                    if (!key1_id->IsNull() && pwallet->mapBLSPubKeys.count(*key1_id)) {
                        ret.push_back(Pair("hdkeypath", pwallet->mapBLSPubKeys[*key1_id].GetKeyPath()));
                        ret.push_back(Pair("hdchainid", hdChain.GetID().GetHex()));
                    }
                }
            }
            catch (...) { ; }
            }
#endif
    }
    return ret;
}

// Needed even with !ENABLE_WALLET, to pass (ignored) pointers around
class CWallet;
static UniValue createmultisig(const Config &config,
                               const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 2 ||
        request.params.size() > 2) {
        std::string msg =
            "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys "
            "required.\n"
            "It returns a json object with the address and redeemScript.\n"
            "DEPRECATION WARNING: Using addresses with createmultisig is "
            "deprecated. Clients must\n"
            "transition to using addmultisigaddress to create multisig "
            "addresses with addresses known\n"
            "to the wallet before upgrading to v0.20. To use the deprecated "
            "functionality, start bitcoind with -deprecatedrpc=createmultisig\n"
            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required "
            "signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of hex-encoded "
            "public keys\n"
            "     [\n"
            "       \"key\"    (string) The hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new "
            "multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of "
            "the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 public keys\n" +
            HelpExampleCli("createmultisig",
                           "2 "
                           "\"["
                           "\\\"03789ed0bb717d88f7d321a368d905e7430207ebbd82bd3"
                           "42cf11ae157a7ace5fd\\\","
                           "\\\"03dbc6764b8884a92e871274b87583e6d5c2a58819473e1"
                           "7e107ef3f6aa5a61626\\\"]\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("createmultisig",
                           "2, "
                           "\"["
                           "\\\"03789ed0bb717d88f7d321a368d905e7430207ebbd82bd3"
                           "42cf11ae157a7ace5fd\\\","
                           "\\\"03dbc6764b8884a92e871274b87583e6d5c2a58819473e1"
                           "7e107ef3f6aa5a61626\\\"]\"");
        throw std::runtime_error(msg);
    }

    int required = request.params[0].get_int();

    // Get the public keys
    const UniValue &keys = request.params[1].get_array();
    std::vector<CPubKey> pubkeys;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (IsHex(keys[i].get_str()) && (keys[i].get_str().length() == 66 ||
                                         keys[i].get_str().length() == 130)) {
            pubkeys.push_back(HexToPubKey(keys[i].get_str()));
        } else {
#ifdef ENABLE_WALLET
          std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
          CWallet *const pwallet = wallet.get();
            if (IsDeprecatedRPCEnabled(gArgs, "createmultisig") &&
                EnsureWalletIsAvailable(pwallet, false)) {
                pubkeys.push_back(AddrToPubKey(config.GetChainParams(), pwallet,
                                               keys[i].get_str()));
            } else
#endif
                throw JSONRPCError(
                    RPC_INVALID_ADDRESS_OR_KEY,
                    strprintf("Invalid public key: %s\nNote that from v1.0.2, "
                              "createmultisig no longer accepts addresses."
                              " Clients must transition to using "
                              "addmultisigaddress to create multisig addresses "
                              "with addresses known to the wallet before "
                              "upgrading to v1.0.3"
                              " To use the deprecated functionality, start "
                              "bitcoind with -deprecatedrpc=createmultisig",
                              keys[i].get_str()));
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner = CreateMultisigRedeemscript(required, pubkeys);
    CScriptID innerID(inner);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", EncodeDestination(innerID));
    result.pushKV("redeemScript", HexStr(inner.begin(), inner.end()));

    return result;
}

// clang-format off
static const ContextFreeRPCCommand commands[] = {
    //  category            name                      actor (function)        argNames
    //  ------------------- ------------------------  ----------------------  ----------
    { "control",            "getinfo",                getinfo,                {} }, /* uses wallet if enabled */
    { "util",               "validateaddress",        validateaddress,        {"address"} }, /* uses wallet if enabled */
    { "util",               "createmultisig",         createmultisig,         {"nrequired","keys"} },

};
// clang-format on

void RegisterMiscRPCCommands(CRPCTable &t) {
    for (auto& command : commands) { t.appendCommand(command.name, &command); }
}
