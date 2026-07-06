// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_DNFT_H
#define DEVAULT_DEVAULT_DNFT_H

#include <cstdint>

class CCoinsViewCache;
class CTransaction;
class CValidationState;

namespace Consensus {
struct Params;
}

/**
 * DeVault DNFT consensus rules (DEVAULT_NFT_SPEC.md) — the DeVault-only layer ABOVE the inherited
 * CashTokens rules (consensus/tokens.cpp CheckTxTokens). Kept separate so the inherited code stays
 * diff-clean against upstream and each layer is independently testable.
 *
 * Phase 4A contents: the FT-deferral gate (spec §10.8) — from DU1 activation until the future
 * fungible-token activation, any token output carrying a fungible amount is consensus-invalid,
 * so no stock-CashTokens FT state can exist before the DeVault FT redesign lands.
 * Phase 4C adds: the envelope↔commitment binding rules (spec §6) and parent-claim checks (§7).
 */
namespace dnft {

/**
 * Validate the DeVault DNFT rules for one transaction. Stateless: a pure function of the
 * transaction, its input coins (view) and the activation heights — no database, nothing to undo
 * on disconnect; -reindex/-reindex-chainstate revalidate identically.
 *
 * Called from ConnectBlock and AcceptToMemoryPool alongside CheckTxTokens.
 *
 * @param view     input-coin access; unused in 4A, used by the 4C binding/parent rules.
 * @param scriptFlags  the block/mempool script-verification flags. SCRIPT_ENABLE_TOKENS present
 *                 means DU1 is active (the only place that sets it — validation.cpp
 *                 GetNextBlockScriptFlags); when absent this function is a no-op (the PATFO rules
 *                 own the pre-activation space).
 * @param nHeight  the height of the block (to be) containing the tx: pindex->nHeight in
 *                 ConnectBlock, tip-height + 1 for mempool acceptance.
 * @return true if valid; false with state filled in otherwise.
 */
bool CheckDnftRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                    uint32_t scriptFlags, int nHeight, const Consensus::Params &params);

} // namespace dnft

#endif // DEVAULT_DEVAULT_DNFT_H
