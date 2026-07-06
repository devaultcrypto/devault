// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2020-2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <amount.h>
#include <pubkey.h>
#include <script/script_flags.h>
#include <uint256.h>

#include <cstdint>
#include <variant>

class CKeyID;
class CScript;

/** A reference to a CScript: the Hash160 or Hash256 of its serialization (see script.h) */
class ScriptID {
    using var_t = std::variant<uint160, uint256>;
    var_t var;
public:
    ScriptID() noexcept : var{uint160()} {}
    ScriptID(const CScript &in, bool is32);
    ScriptID(const uint160 &in) noexcept : var{in} {}
    ScriptID(const uint256 &in) noexcept : var{in} {}

    ScriptID & operator=(const uint160 &in) noexcept { var = in; return *this; }
    ScriptID & operator=(const uint256 &in) noexcept { var = in; return *this; }

    bool operator==(const ScriptID &o) const { return var == o.var; }
    bool operator!=(const ScriptID &o) const { return var != o.var; }
    bool operator<(const ScriptID &o) const { return var < o.var; }
    bool operator==(const uint160 &o) const { return IsP2SH_20() && std::get<uint160>(var) == o; }
    bool operator==(const uint256 &o) const { return IsP2SH_32() && std::get<uint256>(var) == o; }

    uint8_t *begin() { return std::visit([](auto &&alt) { return alt.begin(); }, var); }
    uint8_t *end() { return std::visit([](auto &&alt) { return alt.end(); }, var); }
    uint8_t *data() { return std::visit([](auto &&alt) { return alt.data(); }, var); }
    const uint8_t *begin() const { return const_cast<ScriptID *>(this)->begin(); }
    const uint8_t *end() const { return const_cast<ScriptID *>(this)->end(); }
    const uint8_t *data() const { return const_cast<ScriptID *>(this)->data(); }

    size_t size() const { return end() - begin(); }
    uint8_t & operator[](size_t i) { return data()[i]; }
    const uint8_t & operator[](size_t i) const { return data()[i]; }

    bool IsP2SH_20() const { return std::get_if<uint160>(&var) != nullptr; }
    bool IsP2SH_32() const { return std::get_if<uint256>(&var) != nullptr; }
};

/**
 * Default setting for nMaxDatacarrierBytes. 220 bytes of data, +1 for OP_RETURN, +2 for the pushdata opcodes.
 */
static constexpr uint32_t MAX_OP_RETURN_RELAY = 223;

/**
 * A data carrying output is an unspendable output containing data. The script type is designated as TX_NULL_DATA.
 */
extern uint32_t nMaxDatacarrierBytes;

/**
 * Post-upgrade-12 only. The maximum length of a standard tx "bare script" aka "p2s". In other words, after upgrade 12
 * activates, any scriptPubKey that doesn't match p2pk, p2pkh, p2sh[32], bare multisig, and/or OP_RETURN, and is less
 * than or equal to this length is considerd "standard" and is categorized as "p2s".
 *
 * Before upgrade12, such scripts were categorized as non-standard.
 */
static constexpr size_t MAX_P2S_SCRIPT_SIZE = 201;

enum txnouttype {
    TX_NONSTANDARD,
    // 'standard' transaction types:
    TX_PUBKEY,     ///< p2pk
    TX_PUBKEYHASH, ///< p2pkh
    TX_SCRIPTHASH, ///< p2sh or p2sh32
    TX_MULTISIG,   ///< bare multisig
    // unspendable OP_RETURN script that carries data
    TX_NULL_DATA,
    // after upgrade12: p2s (pay-to-script); non-null-data bare scripts that don't match above and are <= 201 bytes
    TX_SCRIPT,     ///< p2s
    // DeVault (DU1, spec Q15): a DNFT content envelope (OP_RETURN + "DNFT" magic). Its own standard
    // type once tokens are active so it is exempt from the plain-OP_RETURN datacarrier cap; sized by
    // the post-DU1 standard tx size cap instead, and consensus-validated at mint (CheckDnftRules).
    TX_DNFT_ENVELOPE,
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &, const CNoDestination &) {
        return true;
    }
    friend bool operator!=(const CNoDestination &, const CNoDestination &) {
        return false;
    }
    friend bool operator<(const CNoDestination &, const CNoDestination &) {
        return false;
    }
};

/**
 * A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * CKeyID: TX_PUBKEYHASH destination
 *  * ScriptID: TX_SCRIPTHASH destination
 *  A CTxDestination is the internal data type encoded in a Bitcoin Cash address
 */
using CTxDestination = std::variant<CNoDestination, CKeyID, ScriptID>;

/** Check whether a CTxDestination is a CNoDestination. */
bool IsValidDestination(const CTxDestination &dest);

/** Get the name of a txnouttype as a C string, or nullptr if unknown. */
const char *GetTxnOutputType(txnouttype t);

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @param[in]   flags          Script execution flags to use for solving.
 *                             Currently only SCRIPT_ENABLE_P2SH_32 is used,
 *                             so if wishing to disable p2sh_32, it's ok to
 *                             pass 0 for flags.
 * @return                     The script type. TX_NONSTANDARD represents a
 * failed solve.
 */
txnouttype Solver(const CScript &scriptPubKey, std::vector<std::vector<uint8_t>> &vSolutionsRet, uint32_t flags);

/**
 * Parse a standard scriptPubKey for the destination address. Assigns result to
 * the addressRet parameter and returns true if successful. For multisig
 * scripts, instead use ExtractDestinations. Currently only works for P2PK,
 * P2PKH, P2SH and P2SH_32 scripts.
 *
 * @param[in] flags  If SCRIPT_ENABLE_P2SH_32 is present, then allow p2sh_32
 *                   destinations, otherwise legacy p2sh_20 behavior only.
 *                   Ok to pass 0 here for legacy behavior.
 */
bool ExtractDestination(const CScript &scriptPubKey, CTxDestination &addressRet, uint32_t flags);

/**
 * Parse a standard scriptPubKey with one or more destination addresses. For
 * multisig scripts, this populates the addressRet vector with the pubkey IDs
 * and nRequiredRet with the n required to spend. For other destinations,
 * addressRet is populated with a single value and nRequiredRet is set to 1.
 * Returns true if successful.
 *
 * Note: this function confuses destinations (a subset of CScripts that are
 * encodable as an address) with key identifiers (of keys involved in a
 * CScript), and its use should be phased out.
 *
 * @param[in] flags  If SCRIPT_ENABLE_P2SH_32 is present, then allow p2sh_32
 *                   destinations, otherwise legacy p2sh_20 behavior only.
 *                   Ok to pass 0 here for legacy behavior.
 */
bool ExtractDestinations(const CScript &scriptPubKey, txnouttype &typeRet,
                         std::vector<CTxDestination> &addressRet,
                         int &nRequiredRet, uint32_t flags);

/**
 * Generate a Bitcoin scriptPubKey for the given CTxDestination. Returns a P2PKH
 * script for a CKeyID destination, a P2SH script for a ScriptID, and an empty
 * script for CNoDestination.
 */
CScript GetScriptForDestination(const CTxDestination &dest);

/** Generate a P2PK script for the given pubkey. */
CScript GetScriptForRawPubKey(const CPubKey &pubkey);

/** Generate a multisig script. */
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey> &keys);
