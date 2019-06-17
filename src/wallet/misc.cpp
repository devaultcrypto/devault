// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/rpcutil.h>
#include <cashaddrenc.h>
#include <clientversion.h>
#include <config.h>
#include <chain.h>
#include <dstencode.h>
#include <init.h>
#include <net.h>
#include <netbase.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <timedata.h>
#include <txmempool.h>
#include <util.h>
#include <utilstrencodings.h>
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

#ifdef ENABLE_WALLET
    CWallet *const pwallet = GetWalletForJSONRPCRequest(request);

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", CLIENT_VERSION);
    obj.pushKV("protocolversion", PROTOCOL_VERSION);
#ifdef ENABLE_WALLET
    if (pwallet) {
        obj.pushKV("walletversion", pwallet->GetVersion());
        obj.pushKV("balance", ValueFromAmount(pwallet->GetBalance()));
    }
#endif
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
#ifdef ENABLE_WALLET
    if (pwallet) {
        obj.pushKV("keypoololdest", pwallet->GetOldestKeyPoolTime());
        obj.pushKV("keypoolsize", (int)pwallet->GetKeyPoolSize());
    }
    if (pwallet && pwallet->IsCrypted()) {
        obj.pushKV("unlocked_until", pwallet->nRelockTime);
    }
    obj.pushKV("paytxfee", ValueFromAmount(payTxFee.GetFeePerK()));
#endif
    obj.pushKV("relayfee",
               ValueFromAmount(config.GetMinFeePerKB().GetFeePerK()));
    obj.pushKV("errors", GetWarnings("statusbar"));
    return obj;
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor
#ifdef HAVE_VARIANT
  : public std::variant<UniValue>
#else
  : public boost::static_visitor<UniValue>
#endif
{
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
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
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
#endif

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
    CWallet *const pwallet = GetWalletForJSONRPCRequest(request);

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    CTxDestination dest =
        DecodeDestination(request.params[0].get_str(), config.GetChainParams());
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
#ifdef HAVE_VARIANT
        UniValue detail = std::visit(DescribeAddressVisitor(pwallet), dest);
#else
        UniValue detail = boost::apply_visitor(DescribeAddressVisitor(pwallet), dest);
 #endif
        ret.pushKVs(detail);
        if (pwallet && pwallet->mapAddressBook.count(dest)) {
            ret.pushKV("account", pwallet->mapAddressBook[dest].name);
        }
        if (pwallet) {
            const CKeyMetadata *meta = nullptr;
#ifdef HAVE_VARIANT
            if (const CKeyID *key_id = &std::get<CKeyID>(dest)) {
#else
            if (const CKeyID *key_id = boost::get<CKeyID>(&dest)) {
#endif
              auto it = pwallet->mapKeyMetadata.find(*key_id);
              if (it != pwallet->mapKeyMetadata.end()) meta = &it->second;
              // inside if
              if (meta) {
                  CHDChain hdChain;
                  pwallet->GetHDChain(hdChain);
                  if (!key_id->IsNull() && pwallet->mapHdPubKeys.count(*key_id)) {
                  ret.push_back(Pair("hdkeypath", pwallet->mapHdPubKeys[*key_id].GetKeyPath()));
                  ret.push_back(Pair("hdchainid", hdChain.GetID().GetHex()));
                }
            }
        }
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
            CWallet *const pwallet = GetWalletForJSONRPCRequest(request);
            if (IsDeprecatedRPCEnabled(gArgs, "createmultisig") &&
                EnsureWalletIsAvailable(pwallet, false)) {
                pubkeys.push_back(AddrToPubKey(config.GetChainParams(), pwallet,
                                               keys[i].get_str()));
            } else
#endif
                throw JSONRPCError(
                    RPC_INVALID_ADDRESS_OR_KEY,
                    strprintf("Invalid public key: %s\nNote that from v0.19.6, "
                              "createmultisig no longer accepts addresses."
                              " Clients must transition to using "
                              "addmultisigaddress to create multisig addresses "
                              "with addresses known to the wallet before "
                              "upgrading to v0.20."
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

static UniValue verifymessage(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 3) {
        throw std::runtime_error(
            "verifymessage \"address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"address\"         (string, required) The DeVault address to "
            "use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided "
            "by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was "
            "signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n" +
            HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n" +
            HelpExampleCli(
                "signmessage",
                R"("1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX" "my message")") +
            "\nVerify the signature\n" +
            HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4"
                                            "XX\" \"signature\" \"my "
                                            "message\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4"
                                            "XX\", \"signature\", \"my "
                                            "message\""));
    }

    LOCK(cs_main);

    std::string strAddress = request.params[0].get_str();
    std::string strSign = request.params[1].get_str();
    std::string strMessage = request.params[2].get_str();

    CTxDestination destination =
        DecodeDestination(strAddress, config.GetChainParams());
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    }
#ifdef HAVE_VARIANT
    const CKeyID *keyID = &std::get<CKeyID>(destination);
#else
    const CKeyID *keyID = boost::get<CKeyID>(&destination);
#endif
    if (!keyID) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
    }

    bool fInvalid = false;
    std::vector<uint8_t> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Malformed base64 encoding");
    }

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig)) {
        return false;
    }

    return (pubkey.GetID() == *keyID);
}

static UniValue signmessagewithprivkey(const Config &config,
                                       const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 2) {
        throw std::runtime_error(
            "signmessagewithprivkey \"privkey\" \"message\"\n"
            "\nSign a message with the private key of an address\n"
            "\nArguments:\n"
            "1. \"privkey\"         (string, required) The private key to sign "
            "the message with.\n"
            "2. \"message\"         (string, required) The message to create a "
            "signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message "
            "encoded in base 64\n"
            "\nExamples:\n"
            "\nCreate the signature\n" +
            HelpExampleCli("signmessagewithprivkey",
                           R"("privkey" "my message")") +
            "\nVerify the signature\n" +
            HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4"
                                            "XX\" \"signature\" \"my "
                                            "message\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("signmessagewithprivkey",
                           R"("privkey", "my message")"));
    }

    std::string strPrivkey = request.params[0].get_str();
    std::string strMessage = request.params[1].get_str();

    CKey key = DecodeSecret(strPrivkey);
    if (!key.IsValid()) {
      throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                         "Private key invalid or outside allowed range");
    }
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    std::vector<uint8_t> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");
    }

    return EncodeBase64(&vchSig[0], vchSig.size());
}

static UniValue setmocktime(const Config &config,
                            const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "setmocktime timestamp\n"
            "\nSet the local time to given timestamp (-regtest only)\n"
            "\nArguments:\n"
            "1. timestamp  (integer, required) Unix seconds-since-epoch "
            "timestamp\n"
            "   Pass 0 to go back to using the system time.");
    }

    if (!config.GetChainParams().MineBlocksOnDemand()) {
        throw std::runtime_error(
            "setmocktime for regression testing (-regtest mode) only");
    }

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all callsites of GetTime() are accessing this safely.
    LOCK(cs_main);

    RPCTypeCheck(request.params, {UniValue::VNUM});
    SetMockTime(request.params[0].get_int64());

    return NullUniValue;
}

static UniValue RPCLockedMemoryInfo() {
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", uint64_t(stats.used));
    obj.pushKV("free", uint64_t(stats.free));
    obj.pushKV("total", uint64_t(stats.total));
    obj.pushKV("locked", uint64_t(stats.locked));
    obj.pushKV("chunks_used", uint64_t(stats.chunks_used));
    obj.pushKV("chunks_free", uint64_t(stats.chunks_free));
    return obj;
}

#ifdef HAVE_MALLOC_INFO
static std::string RPCMallocInfo() {
    char *ptr = nullptr;
    size_t size = 0;
    FILE *f = open_memstream(&ptr, &size);
    if (f) {
        malloc_info(0, f);
        fclose(f);
        if (ptr) {
            std::string rv(ptr, size);
            free(ptr);
            return rv;
        }
    }
    return "";
}
#endif

static UniValue getmemoryinfo(const Config &config,
                              const JSONRPCRequest &request) {
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    if (request.fHelp || request.params.size() > 1) {
        throw std::runtime_error(
            "getmemoryinfo (\"mode\")\n"
            "Returns an object containing information about memory usage.\n"
            "Arguments:\n"
            "1. \"mode\" determines what kind of information is returned. This "
            "argument is optional, the default mode is \"stats\".\n"
            "  - \"stats\" returns general statistics about memory usage in "
            "the daemon.\n"
            "  - \"mallocinfo\" returns an XML string describing low-level "
            "heap state (only available if compiled with glibc 2.10+).\n"
            "\nResult (mode \"stats\"):\n"
            "{\n"
            "  \"locked\": {               (json object) Information about "
            "locked memory manager\n"
            "    \"used\": xxxxx,          (numeric) Number of bytes used\n"
            "    \"free\": xxxxx,          (numeric) Number of bytes available "
            "in current arenas\n"
            "    \"total\": xxxxxxx,       (numeric) Total number of bytes "
            "managed\n"
            "    \"locked\": xxxxxx,       (numeric) Amount of bytes that "
            "succeeded locking. If this number is smaller than total, locking "
            "pages failed at some point and key data could be swapped to "
            "disk.\n"
            "    \"chunks_used\": xxxxx,   (numeric) Number allocated chunks\n"
            "    \"chunks_free\": xxxxx,   (numeric) Number unused chunks\n"
            "  }\n"
            "}\n"
            "\nResult (mode \"mallocinfo\"):\n"
            "\"<malloc version=\"1\">...\"\n"
            "\nExamples:\n" +
            HelpExampleCli("getmemoryinfo", "") +
            HelpExampleRpc("getmemoryinfo", ""));
    }

    std::string mode = (request.params.size() < 1 || request.params[0].isNull())
                           ? "stats"
                           : request.params[0].get_str();
    if (mode == "stats") {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("locked", RPCLockedMemoryInfo());
        return obj;
    } else if (mode == "mallocinfo") {
#ifdef HAVE_MALLOC_INFO
        return RPCMallocInfo();
#else
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            "mallocinfo is only available when compiled with glibc 2.10+");
#endif
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown mode " + mode);
    }
}

static UniValue echo(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp) {
        throw std::runtime_error(
            "echo|echojson \"message\" ...\n"
            "\nSimply echo back the input arguments. This command is for "
            "testing.\n"
            "\nThe difference between echo and echojson is that echojson has "
            "argument conversion enabled in the client-side table in"
            "devault-cli and the GUI. There is no server-side difference.");
    }

    return request.params;
}

bool getAddressesFromParams(const Config &config, const UniValue& params, std::vector<std::string> &addresses)
{
    if (params[0].isStr()) {
        std::string addr = params[0].get_str();
        CTxDestination address = DecodeDestination(addr, config.GetChainParams());
        if (!IsValidDestination(address)) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        addresses.push_back(addr);
    } else if (params[0].isObject()) {
        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }
        std::vector<UniValue> values = addressValues.getValues();
        for (const auto& it : values) {
            CTxDestination address = DecodeDestination(it.get_str(), config.GetChainParams());
            if (!IsValidDestination(address)) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            std::string addr =  EncodeCashAddr(address, Params());
            addresses.push_back(addr);
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    return true;
}

bool timestampSort(std::pair<CMempoolAddrDeltaKey, CMempoolAddrDelta> a,
                   std::pair<CMempoolAddrDeltaKey, CMempoolAddrDelta> b) {
    return a.second.time < b.second.time;
}

UniValue getaddressmempool(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
                                 "getaddressmempool\n"
                                 "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n"
                                 "\nArguments:\n"
                                 "{\n"
                                 "  \"addresses\"\n"
                                 "    [\n"
                                 "      \"address\"  (string) The base58check encoded address\n"
                                 "      ,...\n"
                                 "    ]\n"
                                 "}\n"
                                 "\nResult:\n"
                                 "[\n"
                                 "  {\n"
                                 "    \"address\"  (string) The base58check encoded address\n"
                                 "    \"txid\"  (string) The related txid\n"
                                 "    \"index\"  (number) The related input or output index\n"
                                 "    \"satoshis\"  (number) The difference of satoshis\n"
                                 "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
                                 "    \"prevtxid\"  (string) The previous txid (if spending)\n"
                                 "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
                                 "  }\n"
                                 "]\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("getaddressmempool", R"('{"addresses": ["12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX"]}')")
                                 + HelpExampleRpc("getaddressmempool", R"({"addresses": ["12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX"]})")
                                 );

    std::vector<std::string> addresses;

    if (!getAddressesFromParams(GetConfig(), request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }
    
    std::vector<std::pair<CMempoolAddrDeltaKey, CMempoolAddrDelta> > indexes;
    
    if (!g_mempool.getAddrIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);
     
    for (const auto& it : indexes) {
        std::string address = it.first.addr;
        UniValue delta(UniValue::VOBJ);
        delta.push_back(Pair("address", address));
        delta.push_back(Pair("txid", it.first.txhash.GetHex()));
        delta.push_back(Pair("index", (int)it.first.index));
        delta.push_back(Pair("satoshis", ValueFromAmount(it.second.amount)));
        delta.push_back(Pair("timestamp", it.second.time));
        if (it.second.amount < Amount()) {
            delta.push_back(Pair("prevtxid", it.second.prevhash.GetHex()));
            delta.push_back(Pair("prevout", (int)it.second.prevout));
        }
        result.push_back(delta);
    }

    return result;
}

static UniValue getaddressbalance(const Config &config, const JSONRPCRequest &request) {
  
  if (request.fHelp || request.params.size() != 1)
    throw std::runtime_error(
                             "getbalance ( \"address\" )\n"
            "\nReturns the balance for an address (requires addressindex to be enabled).\n"
            "\nArguments: address\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\"  (string) The current balance in satoshis\n"
            "  \"received\"  (string) The total number of satoshis received (including change)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance","\"devault:qpzfppqqg5sk6ck8c624tk7vgxeuafaq9uumff5u2u\"")
            + HelpExampleRpc("getaddressbalance","\"devault:qpzfppqqg5sk6ck8c624tk7vgxeuafaq9uumff5u2u\""));
    

   std::string address = request.params[0].get_str();
   CTxDestination dest = DecodeDestination(address, config.GetChainParams());
   if (!IsValidDestination(dest)) {
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
   }

   std::vector<std::pair<CAddrIndexKey, Amount> > addressIndex;

   if (!GetAddrIndex(address, addressIndex)) {
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
   }
   
   Amount balance;
   Amount received;
   
   for (const auto& it : addressIndex) {
       if (it.second > Amount()) {
           received += it.second;
       }
       balance += it.second;
   }
   
   UniValue result(UniValue::VOBJ);
   result.push_back(Pair("balance", ValueFromAmount(balance)));
   result.push_back(Pair("received", ValueFromAmount(received)));
   
   return result;

}

static UniValue getrewards(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
                                 "getrewards \"filename\"\n"
                                 "\nReturns all of the valid reward UTXO values.\n"
                                 "{\nArguments"
                                 "1. \"filename\"    (string, required) The filename with path (either absolute or relative to devaultd)\n"
                                 "\nResult:\n"
                                 "{\n"
                                 "  \"utxo\"  (string) The current balance in satoshis\n"
                                 "}\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("getrewards","")
                                 + HelpExampleRpc("getrewards",""));
    

    std::vector<CRewardValue> rewards = prewards->GetOrderedRewards();
    UniValue result(UniValue::VOBJ);

    fs::path filepath = request.params[0].get_str();
    filepath = fs::absolute(filepath);
    
    std::ofstream file;
    file.open(filepath.string().c_str());
    if (!file.is_open()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Cannot open getrewards dump file");
    }

    for (auto& val : rewards) {
        file << "Value " << val.GetValue().ToString() << " ";
        file << "active "<<  val.IsActive() << " ";
        file << "creationHeight " << val.GetCreationHeight() << " ";
        file << "OldHeight " << val.GetOldHeight() << " ";
        file << "Height " << val.GetHeight() << " ";
        file << "addr " << GetAddrFromTxOut(val.GetTxOut()) << " ";
        file << "payCount " << val.GetPayCount() << "\n";
    }   
    file.close();

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.string());

    return reply;
}




// clang-format off
static const ContextFreeRPCCommand commands[] = {
    //  category            name                      actor (function)        argNames
    //  ------------------- ------------------------  ----------------------  ----------
    { "control",            "getinfo",                getinfo,                {} }, /* uses wallet if enabled */
    { "control",            "getmemoryinfo",          getmemoryinfo,          {"mode"} },
    { "util",               "validateaddress",        validateaddress,        {"address"} }, /* uses wallet if enabled */
    { "util",               "createmultisig",         createmultisig,         {"nrequired","keys"} },
    { "util",               "verifymessage",          verifymessage,          {"address","signature","message"} },
    { "util",               "signmessagewithprivkey", signmessagewithprivkey, {"privkey","message"} },
    { "util",               "getrewards",             getrewards,             {} },
    /* Address index */
    { "addressindex",       "getaddressbalance",      getaddressbalance,      {} },

    /* Not shown in help */
    { "hidden",             "setmocktime",            setmocktime,            {"timestamp"}},
    { "hidden",             "echo",                   echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "echojson",               echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
};
// clang-format on

void RegisterMiscRPCCommands(CRPCTable &t) {
    for (auto& command : commands) { t.appendCommand(command.name, &command); }
}
