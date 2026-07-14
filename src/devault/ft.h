// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_FT_H
#define DEVAULT_DEVAULT_FT_H

#include <consensus/tokens.h> // token::FtMintAllowance
#include <devault/ft_registry.h>

#include <cstdint>

class CCoinsViewCache;
class CTransaction;
class CValidationState;

namespace Consensus {
struct Params;
}

/**
 * DeVault fungible-token (DFT) consensus rules — DEVAULT_FT_SPEC.md §6. The DeVault-only layer
 * ABOVE the inherited CashTokens conservation (consensus/tokens.cpp CheckTxTokens); a sibling to
 * dnft::CheckDnftRules.
 *
 * THE EMISSION RULE IS STATELESS. A mint's validity is a pure function of
 *   (the deploy's immutable params, the block height, the in-block index of this mint)
 * — there is no `mints_so_far` counter anywhere. The cumulative cap N is enforced structurally: the
 * per-height schedule (§6.1) sums to exactly N over the window, so no valid chain can exceed it.
 * The only persistent object is the static, write-once deploy registry (ft_registry.h), which is a
 * params lookup, not an emission counter. Consequence: reorgs roll back NOTHING emission-related —
 * each mint simply re-derives its validity at whatever height it lands on.
 */
namespace dnft {

/**
 * The per-height mint allowance (spec §6.1):
 *
 *     schedule_limit(h) = clamp( N - (h - s) * M, 0, M )   for h >= s ;  0 otherwise
 *
 * Every block from `s` allows M mints until the final one, which allows `N mod M` — so the schedule
 * delivers EXACTLY N mints in total (sum over h = N), an exact cap with no stored counter. Computed
 * in 128-bit so `(h - s) * M` can never overflow.
 */
uint64_t FtScheduleLimit(const FtDeployRecord &rec, int nHeight);

/**
 * Validate a DVFT deploy transaction and produce its registry record.
 *
 * VIEW-FREE BY DESIGN — a pure function of (tx, nHeight). The deployed category is
 * `X = tx.vin[0].prevout.txid` (which requires `tx.vin[0].prevout.n == 0`, making X a CashTokens
 * genesis candidate), and the premine check sums only the transaction's OWN outputs. Because no
 * input coins are consulted, the registry can be rebuilt by replaying block data alone — no
 * chainstate, no undo data (spec §6.5). Both the connect path and the startup/rebuild replay call
 * this same function, so they cannot diverge.
 *
 * Only OPEN-mint deploys produce a record (fixed-supply tokens can never be minted, so nothing ever
 * looks them up); `recOut` is left untouched and `isOpenOut` is false for a fixed deploy.
 *
 * @return true if the tx is a VALID deploy (or carries no DVFT deploy envelope at all — see
 *         CheckFtRules for the caller's classification); false with `state` filled in otherwise.
 */
bool ValidateFtDeploy(const CTransaction &tx, int nHeight, FtDeployRecord &recOut, bool &isOpenOut,
                      CValidationState &state);

/**
 * Validate the DeVault fungible-token rules for one transaction.
 *
 * MUST be called BEFORE CheckTxTokens, because a valid open-mint needs a conservation carve-out:
 * `allowanceOut` receives the (category, amount) that CheckTxTokens must credit as a virtual input
 * so the mint's ex-nihilo Q is permitted (spec §6.4). The carve-out is scoped to that one category
 * and that one Q — nothing else about token validation changes.
 *
 * @param view          input coins (used by the mint branch to compute the net creation of X).
 * @param scriptFlags   SCRIPT_ENABLE_TOKENS present => DU1 active. Pre-DU1 this is a no-op.
 * @param nHeight       height of the block (to be) containing the tx: pindex->nHeight in
 *                      ConnectBlock, tip-height + 1 for mempool acceptance.
 * @param ctx           per-block state (ConnectBlock). NULL on the mempool path, where the in-block
 *                      mint index is unknowable: ATMP then does the loose check (the window must be
 *                      open) and leaves the strict per-block cap to BlockAssembler + ConnectBlock
 *                      (spec §6.6). On the ConnectBlock path this also STAGES the block's deploys.
 * @param allowanceOut  out: set when this tx is a valid mint; must be handed to CheckTxTokens.
 * @return true if valid; false with `state` filled in otherwise.
 */
bool CheckFtRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                  uint32_t scriptFlags, int nHeight, const Consensus::Params &params,
                  FtBlockContext *ctx, token::FtMintAllowance *allowanceOut);

} // namespace dnft

#endif // DEVAULT_DEVAULT_FT_H
