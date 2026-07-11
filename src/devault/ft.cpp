// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/ft.h>

#include <consensus/activation.h>
#include <consensus/params.h>
#include <primitives/transaction.h>
#include <script/script_flags.h>

namespace dnft {

bool CheckFtRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                  uint32_t scriptFlags, int nHeight, const Consensus::Params &params) {
    // Phase 5A: wired-in no-op. Until ftForkHeight the FT-deferral gate (CheckDnftRules) already
    // rejects every amount-bearing token output, so no fungible-token state can exist for these
    // rules to act on. Phase 5C fills this in with:
    //   - DVFT deploy-envelope validation + registration in the deploy registry (spec §6.2, §6.5),
    //   - the stateless mint-emission schedule (§6.1, §6.3),
    //   - the CheckTxTokens ex-nihilo carve-out for a valid mint (§6.4).
    (void)tx;
    (void)state;
    (void)view;
    (void)scriptFlags;
    (void)nHeight;
    (void)params;
    return true;
}

} // namespace dnft
