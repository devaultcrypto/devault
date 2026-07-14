// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/ft.h>

#include <coins.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <devault/ft_envelope.h>
#include <primitives/transaction.h>
#include <script/script_flags.h>
#include <tinyformat.h>

#include <cstring>
#include <limits>

namespace dnft {

namespace {

//! The CashTokens fungible-amount ceiling (token::SafeAmount is an int64).
constexpr uint64_t FT_AMOUNT_MAX = uint64_t(std::numeric_limits<int64_t>::max());

//! Sum the fungible amounts of `category` across a transaction's outputs. Amounts are individually
//! bounded by int64 max and there can be many outputs, so accumulate in 128-bit.
__int128 SumOutputAmounts(const CTransaction &tx, const token::Id &category) {
    __int128 total = 0;
    for (const CTxOut &out : tx.vout) {
        if (out.tokenDataPtr && out.tokenDataPtr->GetId() == category) {
            total += __int128(out.tokenDataPtr->GetAmount().getint64());
        }
    }
    return total;
}

//! Same, over the transaction's input coins.
__int128 SumInputAmounts(const CTransaction &tx, const CCoinsViewCache &view,
                         const token::Id &category) {
    __int128 total = 0;
    for (const CTxIn &in : tx.vin) {
        const Coin &coin = view.AccessCoin(in.prevout);
        if (coin.IsSpent()) {
            continue; // missing/spent inputs are rejected by CheckTxInputs/CheckTxTokens
        }
        const auto &pdata = coin.GetTxOut().tokenDataPtr;
        if (pdata && pdata->GetId() == category) {
            total += __int128(pdata->GetAmount().getint64());
        }
    }
    return total;
}

/**
 * Locate this transaction's single DVFT envelope.
 *  - Returns false (with `state` set) when the tx carries MORE THAN ONE DVFT envelope, when a DVFT
 *    envelope also carries token data, or when the single envelope fails to parse.
 *  - Sets `found` = false when the tx carries no DVFT envelope at all (the overwhelmingly common
 *    case — a fast exit).
 *
 * At most ONE DVFT envelope per transaction (spec §8 refinement, 5C): a deploy or a mint, never
 * both and never several. This removes a whole class of ambiguity (which deploy does a mint pair
 * with? which of two deploys owns the premine outputs?) at zero cost — a deployer wanting two
 * tokens simply uses two transactions.
 */
bool FindFtEnvelope(const CTransaction &tx, ParsedFtEnvelope &out, bool &found,
                    CValidationState &state) {
    found = false;
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        const CTxOut &txout = tx.vout[i];
        if (!IsFtEnvelope(txout.scriptPubKey)) {
            continue;
        }
        if (found) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-multiple-envelopes", false,
                             strprintf("transaction carries more than one DVFT envelope"));
        }
        // A DVFT marker is a pure OP_RETURN. Attaching token data to it would burn the tokens into
        // an unspendable output and make the output ambiguously both a marker and a token carrier.
        if (txout.tokenDataPtr) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-envelope-has-token", false,
                             strprintf("DVFT envelope output %d also carries token data", i));
        }
        out = ParseFtEnvelope(txout.scriptPubKey);
        if (!out.valid) {
            // The codec's precise reason (bad-txns-ft-envelope-*).
            return state.DoS(100, false, REJECT_INVALID, out.error, false,
                             strprintf("DVFT envelope output %d is invalid", i));
        }
        found = true;
    }
    return true;
}

} // namespace

uint64_t FtScheduleLimit(const FtDeployRecord &rec, const int nHeight) {
    if (rec.perBlockLimit == 0) {
        return 0; // defensive: a registered record always has M >= 1
    }
    if (nHeight < 0 || uint64_t(nHeight) < uint64_t(rec.startHeight)) {
        return 0; // the window has not opened
    }
    // remaining = N - (h - s) * M, computed in 128-bit: (h - s) <= 2^32 and M <= 2^64-1, so the
    // product would overflow 64 bits.
    const __int128 elapsed = __int128(uint64_t(nHeight) - uint64_t(rec.startHeight));
    const __int128 consumed = elapsed * __int128(rec.perBlockLimit);
    const __int128 remaining = __int128(rec.maxMints) - consumed;
    if (remaining <= 0) {
        return 0; // the window has closed (the cap is exhausted)
    }
    const __int128 limit = std::min<__int128>(remaining, __int128(rec.perBlockLimit));
    return uint64_t(limit);
}

bool ValidateFtDeploy(const CTransaction &tx, const int nHeight, FtDeployRecord &recOut,
                      bool &isOpenOut, CValidationState &state) {
    isOpenOut = false;

    ParsedFtEnvelope env;
    bool found = false;
    if (!FindFtEnvelope(tx, env, found, state)) {
        return false;
    }
    if (!found || !env.is_deploy) {
        return true; // not a deploy: nothing for this function to do
    }

    // The deployed category X is the txid of the transaction's FIRST input's prevout, which MUST
    // have index 0 (making X a CashTokens genesis candidate — consensus/tokens.cpp). Pinning it to
    // vin[0] makes X unambiguous even when other inputs also happen to spend an index-0 coin, and
    // mirrors the DNFT convention (the binding salt is likewise vin[0].prevout). A coinbase can
    // never satisfy this (its prevout index is 0xFFFFFFFF), so a coinbase can never be a deploy.
    if (tx.vin.empty() || tx.vin[0].prevout.GetN() != 0) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-deploy-no-genesis-input", false,
                         strprintf("a DVFT deploy must spend an index-0 prevout as its first input"));
    }
    const token::Id category{tx.vin[0].prevout.GetTxId()};

    if (env.mode == FT_MODE_FIXED) {
        // Fixed supply is plain CashTokens genesis: conservation already guarantees the supply can
        // only ever shrink, so there is nothing further to enforce and nothing to register (a fixed
        // token can never be minted, so no lookup can ever be needed).
        return true;
    }

    // ---- open mode ----
    // O4: mints begin STRICTLY after the deploy block. This also guarantees a mint can never
    // reference a deploy in its own block, so the registry as of the previous block always suffices
    // during ConnectBlock (no intra-block deploy staging is needed, and CTOR ordering is a non-issue).
    if (uint64_t(env.start_height) <= uint64_t(nHeight)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-deploy-start-not-after-deploy",
                         false,
                         strprintf("start height %u must be greater than the deploy height %d",
                                   env.start_height, nHeight));
    }

    // Canonicalize the cap to N (the schedule needs only N, s and M; the window end is implicit —
    // schedule_limit() returns 0 once the cap is consumed).
    __int128 N;
    if (env.max_mints) {
        N = __int128(*env.max_mints); // codec guarantees >= 1
    } else {
        // The end_height variant: N = M * (e - s + 1). Both operands are attacker-chosen, so this is
        // computed in 128-bit; the supply bound below then rejects anything absurd.
        const __int128 blocks = __int128(uint64_t(*env.end_height) - uint64_t(env.start_height)) + 1;
        N = __int128(env.per_block_limit) * blocks; // codec guarantees end >= start and M >= 1
    }
    if (N < 1) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-deploy-bad-schedule", false,
                         strprintf("the emission schedule permits no mints"));
    }

    // Supply bound: premine + N*Q must fit the CashTokens fungible ceiling. Checked in 128-bit —
    // this is the guard that keeps every later amount computation inside int64.
    const __int128 maxSupply = __int128(env.premine) + N * __int128(env.quantity_per_mint);
    if (maxSupply > __int128(FT_AMOUNT_MAX)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-deploy-supply-overflow", false,
                         strprintf("max supply (premine + max_mints * quantity) exceeds the token "
                                   "amount ceiling"));
    }
    // maxSupply <= INT64_MAX and quantity >= 1, so N <= INT64_MAX: it now fits a uint64 safely.

    // The premine is exactly the fungible amount of category X this genesis transaction creates.
    // (X is a genesis category here, so every X output is newly created.) NFT outputs of X carry no
    // fungible amount and therefore do not contribute — a DFT category may also carry NFTs, which is
    // ordinary CashTokens behaviour and harmless to the FT accounting.
    const __int128 genesisAmount = SumOutputAmounts(tx, category);
    if (genesisAmount != __int128(env.premine)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-deploy-premine-mismatch", false,
                         strprintf("the transaction creates a fungible amount of its own category "
                                   "that does not equal the declared premine"));
    }

    recOut.category = category;
    recOut.quantity = env.quantity_per_mint;
    recOut.perBlockLimit = env.per_block_limit;
    recOut.startHeight = env.start_height;
    recOut.maxMints = uint64_t(N);
    recOut.premine = env.premine;
    recOut.deployHeight = uint32_t(nHeight);
    recOut.decimals = env.decimals;
    isOpenOut = true;
    return true;
}

bool CheckFtRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                  const uint32_t scriptFlags, const int nHeight, const Consensus::Params &params,
                  FtBlockContext *ctx, token::FtMintAllowance *allowanceOut) {
    if (allowanceOut) {
        *allowanceOut = token::FtMintAllowance{};
    }

    if (!(scriptFlags & SCRIPT_ENABLE_TOKENS)) {
        return true; // pre-DU1: no token state can exist at all
    }
    if (tx.IsCoinBase()) {
        // A coinbase can never be a valid deploy (its prevout index is 0xFFFFFFFF, so the vin[0]
        // rule cannot hold) nor a valid mint (CheckTxTokens forbids ANY token output on a coinbase,
        // so it can create no tokens). Exempting it here therefore cannot diverge from what an
        // indexer applying the same rules would conclude.
        return true;
    }
    if (!IsFTForkEnabledForHeightPrev(params, nHeight - 1)) {
        // Pre-FT-fork: the FT-deferral gate in CheckDnftRules already rejects every amount-bearing
        // token output, so no fungible-token state can exist for these rules to act on. A DVFT
        // envelope before the fork is simply an inert OP_RETURN.
        return true;
    }

    ParsedFtEnvelope env;
    bool found = false;
    if (!FindFtEnvelope(tx, env, found, state)) {
        return false;
    }
    if (!found) {
        return true; // fast exit: no DVFT marker, no FT rules to apply
    }

    // ---------------------------------------------------------------- deploy
    if (env.is_deploy) {
        FtDeployRecord rec;
        bool isOpen = false;
        if (!ValidateFtDeploy(tx, nHeight, rec, isOpen, state)) {
            return false;
        }
        if (isOpen && ctx != nullptr) {
            // Stage it: the registry is only touched once the block actually connects, so a block
            // that fails validation later can never pollute it.
            ctx->pendingDeploys.emplace_back(tx.GetId(), rec);
        }
        return true;
    }

    // ---------------------------------------------------------------- mint
    // 1. Resolve the deploy. The marker names the deploy TXID D (a category id is the spent-prevout
    //    txid and so is not txid-addressable to its deploy — spec §6.5). Only OPEN deploys are
    //    registered, so a marker naming a fixed deploy (or anything else) fails here.
    if (g_ftRegistry == nullptr) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-mint-no-registry", false,
                         strprintf("the fungible-token deploy registry is unavailable"));
    }
    uint256 deployHash;
    std::memcpy(deployHash.begin(), env.mint_deploy_txid.data(), env.mint_deploy_txid.size());
    const TxId deployTxid(deployHash);
    const FtDeployRecord *rec = g_ftRegistry->Lookup(deployTxid);
    if (rec == nullptr) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-mint-no-deploy", false,
                         strprintf("mint names %s, which is not a registered open-mint deploy",
                                   deployTxid.ToString()));
    }

    // 2. The transaction must create EXACTLY Q new tokens of the deploy's category X, net of any X
    //    it also spends. Cross-checking the minted category against registry[D].category is what
    //    binds the marker to the tokens actually produced: a marker for D cannot mint some other
    //    category. Computed in 128-bit; the deploy-time supply bound keeps every legitimate value
    //    inside int64, so an out-of-range net can only come from an invalid transaction.
    const __int128 netCreated = SumOutputAmounts(tx, rec->category) - SumInputAmounts(tx, view, rec->category);
    if (netCreated != __int128(rec->quantity)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-mint-quantity", false,
                         strprintf("mint must create exactly the deploy's quantity of its category"));
    }

    // 3. The emission schedule (spec §6.1/§6.3) — the ONE check that enforces both the per-block
    //    throttle and the cumulative cap N, with no stored counter.
    const uint64_t limit = FtScheduleLimit(*rec, nHeight);
    if (ctx != nullptr) {
        // Connect path (strict): `k` is this mint's index among the block's mints of the same
        // deploy, in block (CTOR) order. A block may contain at most schedule_limit(h) of them.
        const uint64_t k = ctx->mintCounts[deployTxid];
        if (k >= limit) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-ft-mint-schedule", false,
                             strprintf("mint %d of deploy %s exceeds this block's allowance of %d "
                                       "at height %d",
                                       k + 1, deployTxid.ToString(), limit, nHeight));
        }
        ctx->mintCounts[deployTxid] = k + 1;
    } else {
        // Mempool path (loose): the in-block index is unknowable here, so only require that the
        // window is open for the next block. The strict per-block cap is applied by BlockAssembler
        // when packing and backstopped by ConnectBlock (spec §6.6).
        if (limit == 0) {
            return state.DoS(0, false, REJECT_INVALID, "bad-txns-ft-mint-schedule", false,
                             strprintf("the emission window for deploy %s is not open at height %d",
                                       deployTxid.ToString(), nHeight));
        }
    }

    // 4. Hand the carve-out to CheckTxTokens: credit exactly Q of X as a virtual input so the
    //    inherited conservation machinery permits this ex-nihilo creation — and nothing more
    //    (spec §6.4). Step 2 already pinned the net to exactly Q, so this cannot over-permit.
    if (allowanceOut) {
        allowanceOut->category = rec->category;
        allowanceOut->amount = int64_t(rec->quantity);
    }
    return true;
}

} // namespace dnft
