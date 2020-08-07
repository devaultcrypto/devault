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
