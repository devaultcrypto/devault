// Copyright (c) 2022 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <primitives/token.h>

#include <cstdint>

class CCoinsViewCache;
class CTransaction;
class CValidationState;

namespace token {
/**
 * DeVault fungible tokens (DEVAULT_FT_SPEC.md §6.4) — the open-mint conservation carve-out.
 *
 * Standard CashTokens creates fungible tokens ONLY in a genesis transaction; every later
 * transaction may conserve or burn them but never increase them. A DeVault open-mint deliberately
 * breaks that one invariant: a consensus-validated mint creates exactly `amount` new tokens of
 * `category` ex nihilo.
 *
 * dnft::CheckFtRules (which MUST run before CheckTxTokens) has, by the time this is populated,
 * already verified the mint marker, resolved the deploy in the registry, enforced the emission
 * schedule, and pinned the transaction's NET creation of `category` to exactly `amount`. All that
 * remains is to let the inherited conservation machinery permit it — which CheckTxTokens does by
 * crediting this allowance as a *virtual input*, reusing the existing safeSub / in-belowout /
 * overflow paths verbatim. The carve-out is scoped to this single category and this single amount,
 * so it cannot leak into ordinary token validation.
 */
struct FtMintAllowance {
    Id category;
    int64_t amount = 0; //!< > 0 when a valid mint was detected; 0 means "no carve-out"
    bool IsSet() const { return amount > 0; }
};
} // namespace token

/**
 * Check all consensus rules for token spends. Note that this must be called regardless of whether SCRIPT_ENABLE_TOKENS
 * is set in `scriptFlags` because even pre-activation we must preserve "unupgraded" behavior of node (reject all
 * inputs that had PREFIX_BYTE at wrappedScriptPubKey[0]).
 *
 * If SCRIPT_ENABLE_TOKENS is set, then this function will validate that the txn spends and/or mints tokens properly.
 * All txns in the block (including the coinbase tx) and/or all txns coming into the mempool should be checked against
 * this function.
 *
 * This function does *not* do dupe input checks, or non-token amount checks. As such, it is prudent to call this
 * function before script checks are done (e.g. before CheckInputs()) but do call this function *after*
 * CheckRegularTransaction() and Consensus::CheckTxInputs() are called on a txn.
 *
 * @pre `tx` has already had CheckRegularTransaction() and Consensus::CheckTxInputs() called on it. `tx` may be any
 *      coinbase or non-coinbase txn.
 * @param firstTokenEnabledBlockHeight - The height of the first block that can contain legitimate tokens.  Pass
 *        std::numeric_limits<int64_t>::max() if upgrade9 is not activated yet.
 * @return true if the tx passed all token-related consensus checks, false otherwise.
 * @param ftMintAllowance - DeVault DFT open-mint carve-out (see token::FtMintAllowance). Pass nullptr
 *        (or an unset allowance) for every transaction that is not a validated fungible-token mint.
 */
bool CheckTxTokens(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view, uint32_t scriptFlags,
                   int64_t firstTokenEnabledBlockHeight,
                   const token::FtMintAllowance *ftMintAllowance = nullptr);
