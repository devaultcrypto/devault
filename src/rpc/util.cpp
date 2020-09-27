// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dstencode.h>
#include <keystore.h>
#include <pubkey.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <univalue.h>
#include <wallet/wallet.h>

InitInterfaces *g_rpc_interfaces = nullptr;

// Converts a hex string to a public key if possible
CPubKey HexToPubKey(const std::string &hex_in) {
    if (!IsHex(hex_in)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Invalid public key: " + hex_in);
    }
    CPubKey vchPubKey(ParseHex(hex_in));
    if (!vchPubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Invalid public key: " + hex_in);
    }
    return vchPubKey;
}

// Retrieves a public key for an address from the given CKeyStore
CPubKey AddrToPubKey(const CChainParams &chainparams, CKeyStore *const keystore,
                     const std::string &addr_in) {
    CTxDestination dest = DecodeDestination(addr_in, chainparams);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Invalid address: " + addr_in);
    }
    CKeyID keyID;
    BKeyID keyID1;
    try {
        keyID = std::get<CKeyID>(dest);
    }
    catch (...) {  keyID.SetNull(); }
    try {
        keyID1 = std::get<BKeyID>(dest);
    }
    catch (...) { keyID1.SetNull(); }
    if (keyID.IsNull() && keyID1.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
    }
    CPubKey vchPubKey;
    if (!keyID.IsNull()) {
        if (!keystore->GetPubKey(keyID, vchPubKey)) {
            throw JSONRPCError(
                               RPC_INVALID_ADDRESS_OR_KEY,
                               strprintf("no full public key for address %s", addr_in));
        }
    } else {
        if (!keystore->GetPubKey(keyID1, vchPubKey)) {
            throw JSONRPCError(
                               RPC_INVALID_ADDRESS_OR_KEY,
                               strprintf("no full public key for address %s", addr_in));
        }
    }
    if (!vchPubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR,
                           "Wallet contains an invalid public key");
    }
    return vchPubKey;
}

// Creates a multisig redeemscript from a given list of public keys and number
// required.
CScript CreateMultisigRedeemscript(const int required,
                                   const std::vector<CPubKey> &pubkeys) {
    // Gather public keys
    if (required < 1) {
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            "a multisignature address must require at least one key to redeem");
    }
    if ((int)pubkeys.size() < required) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("not enough keys supplied (got %u keys, "
                                     "but need at least %d to redeem)",
                                     pubkeys.size(), required));
    }
    if (pubkeys.size() > 16) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Number of keys involved in the multisignature "
                           "address creation > 16\nReduce the number");
    }

    CScript result = GetScriptForMultisig(required, pubkeys);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            (strprintf("redeemScript exceeds size limit: %d > %d",
                       result.size(), MAX_SCRIPT_ELEMENT_SIZE)));
    }

    return result;
}

class DescribeAddressVisitor : public std::variant<UniValue> {
public:
    explicit DescribeAddressVisitor() {}

    UniValue operator()(const CNoDestination &dest) const {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        return obj;
    }
  
    UniValue operator()(const BKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        return obj;
    }
};

#ifdef ENABLE_WALLET
class DescribeWalletAddressVisitor : public std::variant<UniValue> {
public:
    CWallet *const pwallet;

    void ProcessSubScript(const CScript &subscript, UniValue &obj,
                          bool include_addresses = false) const {
        // Always present: script type and redeemscript
        txnouttype which_type;
        std::vector<std::vector<uint8_t>> solutions_data;
        Solver(subscript, which_type, solutions_data);
        obj.pushKV("script", GetTxnOutputType(which_type));
        obj.pushKV("hex", HexStr(subscript.begin(), subscript.end()));

        CTxDestination embedded;
        UniValue a(UniValue::VARR);
        if (ExtractDestination(subscript, embedded)) {
            // Only when the script corresponds to an address.
            UniValue subobj(UniValue::VOBJ);
            UniValue detail = std::visit(DescribeAddressVisitor(), embedded);
            subobj.pushKVs(detail);
            UniValue wallet_detail = std::visit(*this, embedded);
            subobj.pushKVs(wallet_detail);
            subobj.pushKV("address", EncodeDestination(embedded));
            subobj.pushKV("scriptPubKey",
                          HexStr(subscript.begin(), subscript.end()));
            // Always report the pubkey at the top level, so that
            // `getnewaddress()['pubkey']` always works.
            if (subobj.exists("pubkey")) {
                obj.pushKV("pubkey", subobj["pubkey"]);
            }
            obj.pushKV("embedded", std::move(subobj));
            if (include_addresses) {
                a.push_back(EncodeDestination(embedded));
            }
        } else if (which_type == TX_MULTISIG) {
            // Also report some information on multisig scripts (which do not
            // have a corresponding address).
            // TODO: abstract out the common functionality between this logic
            // and ExtractDestinations.
            obj.pushKV("sigsrequired", solutions_data[0][0]);
            UniValue pubkeys(UniValue::VARR);
            for (size_t i = 1; i < solutions_data.size() - 1; ++i) {
                CPubKey key(solutions_data[i].begin(), solutions_data[i].end());
                if (include_addresses) {
                    a.push_back(EncodeDestination(key.GetKeyID()));
                }
                pubkeys.push_back(HexStr(key.begin(), key.end()));
            }
            obj.pushKV("pubkeys", std::move(pubkeys));
        }

        // The "addresses" field is confusing because it refers to public keys
        // using their P2PKH address. For that reason, only add the 'addresses'
        // field when needed for backward compatibility. New applications can
        // use the 'pubkeys' field for inspecting multisig participants.
        if (include_addresses) {
            obj.pushKV("addresses", std::move(a));
        }
    }

    explicit DescribeWalletAddressVisitor(CWallet *_pwallet)
        : pwallet(_pwallet) {}

    UniValue operator()(const CNoDestination &dest) const {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        if (pwallet && pwallet->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }
   UniValue operator()(const BKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        if (pwallet && pwallet->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", "false");
        }
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        if (pwallet && pwallet->GetCScript(scriptID, subscript)) {
            ProcessSubScript(subscript, obj, true);
        }
        return obj;
    }
};

UniValue DescribeAddress(const CTxDestination& dest)
{
  return std::visit(DescribeAddressVisitor(), dest);
}

UniValue DescribeWalletAddress(CWallet* pwallet, const CTxDestination& dest)
{
    UniValue ret(UniValue::VOBJ);
    UniValue detail = DescribeAddress(dest);
    ret.pushKVs(detail);
    ret.pushKVs(std::visit(DescribeWalletAddressVisitor(pwallet), dest));
    return ret;
}

#endif
