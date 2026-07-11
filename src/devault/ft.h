// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_FT_H
#define DEVAULT_DEVAULT_FT_H

#include <cstdint>

class CCoinsViewCache;
class CTransaction;
class CValidationState;

namespace Consensus {
struct Params;
}

/**
 * DeVault fungible-token (DFT) consensus rules (DEVAULT_FT_SPEC.md) — the DeVault-only layer ABOVE
 * the inherited CashTokens conservation (consensus/tokens.cpp CheckTxTokens), a sibling to
 * dnft::CheckDnftRules. Kept in its own module so the FT rules, the deploy registry, and the
 * mint-emission schedule are independently testable and the inherited CashTokens code stays
 * diff-clean.
 *
 * Phase 5A contents: a wired-in NO-OP. The activation seam (this function called from ConnectBlock
 * and AcceptToMemoryPool, right after CheckDnftRules) is established here so that Phase 5C is purely
 * additive — it fills the body with the deploy-envelope validation, the stateless mint-emission
 * schedule (spec §6.1), the write-once deploy registry lookup (§6.5), and the CheckTxTokens
 * ex-nihilo carve-out (§6.4). The signature will grow in 5C (a registry handle; a per-block mint
 * counter on the ConnectBlock path).
 */
namespace dnft {

/**
 * Validate the DeVault fungible-token rules for one transaction.
 *
 * 5A: returns true unconditionally (the FT-deferral gate in CheckDnftRules still forbids every
 * amount-bearing token output until ftForkHeight, so there is nothing for this to enforce yet).
 *
 * @param view        input-coin access (used by the 5C mint net-amount + deploy checks).
 * @param scriptFlags block/mempool script-verification flags (SCRIPT_ENABLE_TOKENS ⇒ DU1 active).
 * @param nHeight     height of the block (to be) containing the tx: pindex->nHeight in ConnectBlock,
 *                    tip-height + 1 for mempool acceptance.
 * @return true if valid; false with state filled in otherwise.
 */
bool CheckFtRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                  uint32_t scriptFlags, int nHeight, const Consensus::Params &params);

} // namespace dnft

#endif // DEVAULT_DEVAULT_FT_H
