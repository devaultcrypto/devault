// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/dnft.h>

#include <coins.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <devault/dnft_envelope.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script_flags.h>
#include <util/strencodings.h>

#include <array>
#include <map>
#include <vector>

namespace dnft {

namespace {

// Key identifying a specific inscribed item: category(32) || commitment bytes. For an inscribed
// item the commitment is 33 bytes (BINDING_VERSION || 32-byte hash); a 65-byte parent claim value
// (category || commitment) is exactly this key form (amendment A1), so parent membership is a
// direct set lookup with no re-encoding.
std::vector<uint8_t> ItemKey(const token::Id &category, const token::NFTCommitment &commitment) {
    std::vector<uint8_t> key;
    key.reserve(category.size() + commitment.size());
    key.insert(key.end(), category.begin(), category.end());
    key.insert(key.end(), commitment.begin(), commitment.end());
    return key;
}

} // namespace

bool CheckDnftRules(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &view,
                    const uint32_t scriptFlags, const int nHeight, const Consensus::Params &params) {
    if (!(scriptFlags & SCRIPT_ENABLE_TOKENS)) {
        // Pre-DU1: no token state can exist (the PATFO rules in CheckTxTokens reject token-bearing
        // inputs by consensus and policy rejects token-forming outputs); nothing for us to do.
        return true;
    }
    if (tx.IsCoinBase()) {
        // CheckTxTokens already forbids ANY token data on a coinbase post-activation
        // ("bad-txns-coinbase-has-tokens"); the coinbase has no first-input prevout to salt a
        // binding hash with, so the DNFT rules are simply not applicable to it.
        return true;
    }
    if (tx.vin.empty()) {
        // Defense-in-depth: the binding salt is tx.vin[0].prevout and the input scan iterates
        // tx.vin. CheckRegularTransaction ("bad-txns-vin-empty") already rejects a non-coinbase
        // tx with no inputs before this function runs in both ATMP and ConnectBlock, so this is
        // unreachable — but never dereference vin[0] on faith in consensus code.
        return true;
    }

    const bool ftForkActive = IsFTForkEnabledForHeightPrev(params, nHeight - 1);
    // FT-FORK TODO (4I review #2): ftForkActive flips this gate at ftForkHeight WITHOUT changing
    // any script flag. The fork-boundary mempool purge in validation.cpp (Dis/ConnectTip) fires
    // only on a GetNextBlockScriptFlags() difference, so a reorg across ftForkHeight would NOT
    // purge now-invalid FT txs -> BlockAssembler could brick getblocktemplate. Before any real
    // ftForkHeight is set, generalize that boundary predicate to also compare
    // IsFTForkEnabledForHeightPrev. Unreachable today (ftForkHeight is the sentinel).

    // --- Single output pass (DEVAULT_NFT_SPEC.md §5, §6, §10.8) ---
    // Collect envelope outputs (the content carriers) and inscribed-item candidates (capability
    // none + a BINDING_VERSION-prefixed commitment). Apply the per-output guards inline so their
    // rejects are precise.
    std::vector<size_t> envelopeVouts;        // vout indices of envelope outputs, in order
    std::vector<size_t> candidateInscribed;   // vout indices of 0x01/none NFT outputs, in order
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        const CTxOut &out = tx.vout[i];

        if (IsDnftEnvelope(out.scriptPubKey)) {
            // An envelope output is a pure OP_RETURN content carrier. Attaching token data to it
            // would burn a token into an unspendable output AND make it ambiguously both an
            // envelope and an inscribing output; forbid it outright.
            if (out.tokenDataPtr) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-envelope-has-token", false,
                                 strprintf("envelope output %d also carries token data", i));
            }
            envelopeVouts.push_back(i);
            continue;
        }

        if (!out.tokenDataPtr) {
            continue; // plain (non-token, non-envelope) output
        }
        const token::OutputData &td = *out.tokenDataPtr;
        const token::NFTCommitment &commit = td.GetCommitment();
        const bool inscribed = td.HasNFT() && !commit.empty() && commit[0] == BINDING_VERSION;

        if (inscribed) {
            // An inscribed item MUST be immutable (capability none): a mutable/minting token with a
            // 0x01 commitment could later rewrite or reissue the "immutable" content binding.
            if (td.GetCapability() != token::Capability::None) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-binding-capability", false,
                                 strprintf("output %d has a DNFT commitment on a non-immutable token", i));
            }
            // Inscribed items carry no fungible amount (Q10) — a pure NFT object. This is enforced
            // for every inscribed output (mint or move) independent of the FT-fork status.
            if (td.HasAmount()) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-inscribed-has-amount", false,
                                 strprintf("inscribed DNFT output %d carries a fungible token amount", i));
            }
            candidateInscribed.push_back(i);
        } else if (td.HasAmount() && !ftForkActive) {
            // FT-deferral gate (§10.8, amendment A2): until the DeVault fungible-token system
            // activates, any non-inscribed token output carrying a fungible amount is invalid.
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-token-ft-deferred", false,
                             strprintf("output %d carries a fungible token amount before the"
                                       " DeVault fungible-token activation", i));
        }
    }

    // Fast exit: a transaction with neither envelopes nor inscribed-item outputs has no DNFT
    // obligations (plain payments, pure transfers of already-inscribed items with no new envelope,
    // burns, non-inscribed token ops). The per-output guards above have already run.
    if (envelopeVouts.empty() && candidateInscribed.empty()) {
        return true;
    }

    // --- Input pass: capability-none NFT items present on the inputs (DEVAULT_NFT_SPEC.md §6, §7) ---
    // Used to (a) distinguish a MINT (new commitment) from a MOVE (an existing item transferred, no
    // envelope owed), and (b) validate parent claims (spend-to-prove). Input coins are still present
    // in `view` at this point (CheckTxInputs/CheckTxTokens read them; they are spent later).
    //
    // Multiplicities matter (not just membership): a minting-capability input authorizes creating,
    // under CashTokens conservation, arbitrarily many immutable NFTs — including ones whose
    // commitment equals an inscribed item already on an input. A key→count map lets each move
    // consume exactly one input copy; any EXCESS inscribed-format output (a would-be duplicate of a
    // "1/1") then falls into the mint path below, where the §6.4 binding hash rejects it (its
    // copied commitment cannot equal a freshly salted hash). Without the count, both copies would
    // classify as moves and the duplicate would be minted for free.
    std::map<std::vector<uint8_t>, size_t> inputImmutableItems;
    for (const CTxIn &in : tx.vin) {
        const Coin &coin = view.AccessCoin(in.prevout);
        if (coin.IsSpent()) {
            continue; // defensive: CheckTxInputs guarantees inputs are available
        }
        const token::OutputDataPtr &ptd = coin.GetTxOut().tokenDataPtr;
        if (ptd && ptd->IsImmutableNFT()) { // HasNFT && capability none
            ++inputImmutableItems[ItemKey(ptd->GetId(), ptd->GetCommitment())];
        }
    }

    // Classify candidates: an inscribed output is a MOVE (a transfer of an existing item, no
    // envelope obligation — CashTokens conservation preserved the commitment verbatim) as long as
    // an unconsumed matching immutable input remains; each move consumes one. Any further copy is a
    // MINT (needs an envelope) and is rejected by the binding rule. Commitments are globally unique
    // per item (Q1 salting), so a legitimate move always has exactly one matching input.
    std::vector<size_t> inscribingVouts; // MINTs, in output order
    for (size_t idx : candidateInscribed) {
        const token::OutputData &td = *tx.vout[idx].tokenDataPtr;
        auto it = inputImmutableItems.find(ItemKey(td.GetId(), td.GetCommitment()));
        if (it != inputImmutableItems.end() && it->second > 0) {
            --it->second; // consume one input copy: this output is a MOVE
        } else {
            inscribingVouts.push_back(idx); // no input copy left -> MINT (binding rule applies)
        }
    }

    // --- Validate each envelope and collect its parent claims (DEVAULT_NFT_SPEC.md §6.5) ---
    // An invalid envelope invalidates the whole transaction (consensus at mint).
    std::vector<std::vector<std::array<uint8_t, PARENT_VALUE_LENGTH>>> envelopeParents;
    envelopeParents.reserve(envelopeVouts.size());
    for (size_t idx : envelopeVouts) {
        std::vector<std::array<uint8_t, PARENT_VALUE_LENGTH>> parents;
        std::string err;
        if (!ValidateDnftEnvelope(tx.vout[idx].scriptPubKey, &parents, &err)) {
            return state.DoS(100, false, REJECT_INVALID, err, false,
                             strprintf("invalid DNFT envelope at output %d", idx));
        }
        envelopeParents.push_back(std::move(parents));
    }

    // --- Pairing (§6.3): the i-th envelope binds to the i-th inscribing (MINT) output ---
    if (envelopeVouts.size() != inscribingVouts.size()) {
        if (envelopeVouts.size() > inscribingVouts.size()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-envelope-unclaimed", false,
                             strprintf("%d envelope outputs but only %d inscribing outputs",
                                       envelopeVouts.size(), inscribingVouts.size()));
        }
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-binding-missing", false,
                         strprintf("%d inscribing outputs but only %d envelopes",
                                   inscribingVouts.size(), envelopeVouts.size()));
    }

    // --- Binding hash (§6.4) + parent claims (§7) ---
    const COutPoint &input0 = tx.vin[0].prevout;
    for (size_t p = 0; p < inscribingVouts.size(); ++p) {
        const CScript &envelopeSpk = tx.vout[envelopeVouts[p]].scriptPubKey;
        const size_t nftIdx = inscribingVouts[p];
        const token::NFTCommitment &commit = tx.vout[nftIdx].tokenDataPtr->GetCommitment();
        const std::array<uint8_t, COMMITMENT_LENGTH> expected =
            ComputeDnftCommitment(envelopeSpk, input0, uint32_t(nftIdx));
        if (commit.size() != expected.size() ||
            !std::equal(commit.begin(), commit.end(), expected.begin())) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-binding-mismatch", false,
                             strprintf("inscribed output %d commitment does not match its envelope", nftIdx));
        }
    }

    // Parent claims: each claimed (category, commitment) must name an inscribed item spent as an
    // input of this transaction (§7 spend-to-prove). Membership-based (no consumption), so
    // duplicate claims and shared parents across envelopes are naturally handled.
    for (const auto &parents : envelopeParents) {
        for (const std::array<uint8_t, PARENT_VALUE_LENGTH> &parent : parents) {
            // The claimed commitment (parent[32..]) must itself be an inscribed commitment.
            if (parent[32] != BINDING_VERSION ||
                inputImmutableItems.count(std::vector<uint8_t>(parent.begin(), parent.end())) == 0) {
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-dnft-parent-missing", false,
                                 "a claimed DNFT parent is not an inscribed item spent by this transaction");
            }
        }
    }

    return true;
}

} // namespace dnft
