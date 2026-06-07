// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2021-2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <consensus/consensus.h>
#include <feerate.h>
#include <script/standard.h>

#include <string>

class CCoinsViewCache;
class CTransaction;
class CTxIn;
class CTxOut;

/**
 * Default for -maxgbttime, which sets the max amount of time (in milliseconds) the mining code will spend adding
 * transactions to block templates.
 */
static constexpr int64_t DEFAULT_MAX_GBT_TIME = 0;
/**
 * Default for -maxinitialgbttime, which sets the max amount of time (in milliseconds) the mining code will spend
 * adding transactions to the first block template after the chaintip changes.
 */
static constexpr int64_t DEFAULT_MAX_INITIAL_GBT_TIME = 0;
/**
 * Default for -blockmintxfee, which sets the minimum feerate for a transaction
 * in blocks created by mining code.
 */
static constexpr Amount DEFAULT_BLOCK_MIN_TX_FEE_PER_KB(COIN / 2); // DeVault spock-scale (legacy)
/**
 * Default for -gbtcheckvalidity, which determines whether we call
 * TestBlockValidity() on the generated block template.
 */
static constexpr bool DEFAULT_GBT_CHECK_VALIDITY = true;
/**
 * Default for -allowunconnectedmining, which determines whether we ensure
 * that the node is connected to at least 1 peer for getblocktemplate[light]
 * RPC.
 */
static constexpr bool DEFAULT_ALLOW_UNCONNECTED_MINING = false;
/**
 * The maximum size for transactions we're willing to relay/mine.
 */
static constexpr unsigned int MAX_STANDARD_TX_SIZE = 100000;

/**
 * Before upgrade 12 only:
 *
 * Biggest 'standard' txin is a 15-of-15 P2SH multisig with compressed
 * keys (remember the 520 byte limit on redeemScript size). That works
 * out to a (15*(33+1))+3=513 byte redeemScript, 513+1+15*(73+1)+3=1627
 * bytes of scriptSig, which we round off to 1650 bytes for some minor
 * future-proofing. That's also enough to spend a 20-of-20 CHECKMULTISIG
 * scriptPubKey, though such a scriptPubKey is not considered standard.
 *
 * After upgrade 12:
 *
 * We use MAX_SCRIPT_SIZE from <script/vm_limts.h>
 */
static constexpr unsigned int MAX_TX_IN_SCRIPT_SIG_SIZE_LEGACY = 1650;

/**
 * Default maximum megabytes of mempool memory usage per MB of configured max block size. This is used to calculate the
 * default value for for -maxmempool. A value of 10 here with a 32 MB max block size setting results in a 320 MB
 * maximum mempool size.
 */
static constexpr unsigned int DEFAULT_MAX_MEMPOOL_SIZE_PER_MB = 10;
/**
 * Default for -incrementalrelayfee, which sets the minimum feerate increase for
 * mempool limiting or BIP 125 replacement.
 */
static constexpr CFeeRate MEMPOOL_FULL_FEE_INCREMENT(1000 * SATOSHI);
/**
 * Default for -bytespersigcheck.
 */
static constexpr unsigned int DEFAULT_BYTES_PER_SIGCHECK = 50;
/**
 * Min feerate for defining dust. Historically this has been the same as the
 * minRelayTxFee, however changing the dust limit changes which transactions are
 * standard and should be done with care and ideally rarely. It makes sense to
 * only increase the dust limit after prior releases were already not creating
 * outputs below the new threshold.
 */
// DeVault: kept at BCHN's value. With the spock-quantized CFeeRate::GetFee (see feerate.cpp), the
// effective dust threshold is already a whole-spock floor (3 * 1 spock = 0.003 DVT), i.e. dust is
// spock-based, not satoshi-based. Legacy DeVault used COIN/5 (a ~0.111 DVT dust threshold); that can
// be restored if a higher dust floor is wanted, but it would require scaling the satoshi-scale test
// fixtures (dsproof/p2sh/txvalidationcache/IsStandard) that assume small outputs.
static constexpr Amount DUST_RELAY_TX_FEE(1000 * SATOSHI);

/**
 * The maximum value we accept for configuration of the -txbroadcastinterval
 * configuration parameter. This is a sanity check limit to avoid unspecified
 * or extremely undesirable behavior of the node.
 */
inline constexpr unsigned int MAX_INV_BROADCAST_INTERVAL = 1'000'000;
/**
 * The maximum value we accept for configuration of the -txbroadcastrate
 * configuration parameter. This is a sanity check limit to avoid unspecified
 * or extremely undesirable behavior of the node.
 */
inline constexpr unsigned int MAX_INV_BROADCAST_RATE = 1'000'000;

/**
 * When transactions fail script evaluations under standard flags, this flagset
 * influences the decision of whether to drop them or to also ban the originator
 * (see CheckInputs).
 */
static constexpr uint32_t MANDATORY_SCRIPT_VERIFY_FLAGS =
    SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC |
    SCRIPT_ENABLE_SIGHASH_FORKID | SCRIPT_VERIFY_LOW_S |
    SCRIPT_VERIFY_NULLFAIL | SCRIPT_VERIFY_MINIMALDATA |
    SCRIPT_ENABLE_SCHNORR_MULTISIG | SCRIPT_ENFORCE_SIGCHECKS;

/**
 * Standard script verification flags that standard transactions will comply
 * with. However scripts violating these flags may still be present in valid
 * blocks and we must accept those blocks.
 *
 * Note that the actual mempool validation flags may be slightly different (see
 * GetStandardScriptFlags), however this constant should be set to the most
 * restrictive flag set that applies in the current / next upgrade, since it is
 * used in numerous parts of the codebase that are unable to access the
 * contextual information of which upgrades are currently active.
 */
static constexpr uint32_t STANDARD_SCRIPT_VERIFY_FLAGS =
    MANDATORY_SCRIPT_VERIFY_FLAGS | SCRIPT_VERIFY_DERSIG |
    SCRIPT_VERIFY_SIGPUSHONLY | SCRIPT_VERIFY_MINIMALDATA |
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS | SCRIPT_VERIFY_CLEANSTACK |
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY | SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
    SCRIPT_DISALLOW_SEGWIT_RECOVERY | SCRIPT_VERIFY_INPUT_SIGCHECKS;

/**
 * For convenience, standard but not mandatory verify flags.
 */
static constexpr uint32_t STANDARD_NOT_MANDATORY_VERIFY_FLAGS =
    STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS;

/**
 * Used as the flags parameter to sequence and nLocktime checks in non-consensus
 * code.
 */
static constexpr uint32_t STANDARD_LOCKTIME_VERIFY_FLAGS =
    LOCKTIME_VERIFY_SEQUENCE | LOCKTIME_MEDIAN_TIME_PAST;

Amount GetDustThreshold(const CTxOut &txout, const CFeeRate &dustRelayFee);

bool IsDust(const CTxOut &txout, const CFeeRate &dustRelayFee);

bool IsStandard(const CScript &scriptPubKey, txnouttype &whichType, uint32_t flags);

/**
 * Check for standard transaction types
 * @return True if all outputs (scriptPubKeys) use only standard transaction
 * forms
 */
bool IsStandardTx(const CTransaction &tx, std::string &reason, uint32_t flags);

/**
 * Check for standard transaction types
 * @param[in] mapInputs    Map of previous transactions that have outputs we're
 * spending
 * @return True if all inputs (scriptSigs) use only standard transaction forms
 */
bool AreInputsStandard(const CTransaction &tx, const CCoinsViewCache &mapInputs,
                       uint32_t flags);

extern CFeeRate dustRelayFee;
extern uint32_t nBytesPerSigCheck;

/**
 * Compute the virtual transaction size (size, or more if sigchecks is too large).
 */
int64_t GetVirtualTransactionSize(int64_t nSize, int64_t nSigChecks,
                                  unsigned int bytes_per_sigcheck);
int64_t GetVirtualTransactionSize(const CTransaction &tx, int64_t nSigChecks,
                                  unsigned int bytes_per_sigcheck);
int64_t GetVirtualTransactionInputSize(const CTxIn &txin, int64_t nSigChecks,
                                       unsigned int bytes_per_sigcheck);

static inline int64_t GetVirtualTransactionSize(int64_t nSize,
                                                int64_t nSigChecks) {
    return GetVirtualTransactionSize(nSize, nSigChecks, ::nBytesPerSigCheck);
}
