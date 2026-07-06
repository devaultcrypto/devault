// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/dnft.h>

#include <coins.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script_flags.h>
#include <util/strencodings.h>

namespace dnft {

bool CheckDnftRules(const CTransaction &tx, CValidationState &state,
                    const CCoinsViewCache & /* view: used by the 4C binding/parent rules */,
                    const uint32_t scriptFlags, const int nHeight, const Consensus::Params &params) {
    if (!(scriptFlags & SCRIPT_ENABLE_TOKENS)) {
        // Pre-DU1: no token state can exist (the PATFO rules in CheckTxTokens reject token-bearing
        // inputs by consensus and policy rejects token-forming outputs); nothing for us to do.
        return true;
    }

    if (tx.IsCoinBase()) {
        // CheckTxTokens already forbids ANY token data on a coinbase post-activation
        // ("bad-txns-coinbase-has-tokens"); the DNFT rules have nothing to add.
        return true;
    }

    // FT-deferral gate (DEVAULT_NFT_SPEC.md §10.8, amendment A2): from DU1 until the future
    // DeVault fungible-token system activates, any token output carrying a fungible amount is
    // invalid. Output-side only is sufficient: no FT output can ever be created while the gate is
    // active, so no FT input can exist either. Genesis and transfer are both covered (both
    // manifest as outputs with Structure::HasAmount).
    if (!IsFTForkEnabledForHeightPrev(params, nHeight - 1)) {
        for (size_t i = 0; i < tx.vout.size(); ++i) {
            const auto &tokenData = tx.vout[i].tokenDataPtr;
            if (tokenData && tokenData->HasAmount()) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-token-ft-deferred", false,
                                 strprintf("output %d carries a fungible token amount before the"
                                           " DeVault fungible-token activation", i));
            }
        }
    }

    return true;
}

} // namespace dnft
