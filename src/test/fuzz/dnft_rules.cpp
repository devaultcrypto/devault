// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coins.h>
#include <consensus/activation.h>
#include <consensus/validation.h>
#include <devault/dnft.h>
#include <devault/dnft_envelope.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_flags.h>
#include <streams.h>
#include <version.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <vector>

// Tx-level fuzz of the DNFT consensus rules (CheckDnftRules): attacker-facing input that runs in
// ConnectBlock/ATMP for every post-DU1 transaction. The harness deserializes an arbitrary
// transaction from the fuzz stream and synthesizes its input coins from the remaining bytes.
// To reach the deep paths (move classification, parent claims, burn-vs-recreate), input coins
// can CLONE token identities from the transaction's own outputs — pure random bytes would never
// produce a matching 33-byte commitment.
//
// Invariants asserted:
//   * no crash / OOB / UB on any input (the point of the target)
//   * determinism: two identical calls return the same verdict and reject reason
//   * without SCRIPT_ENABLE_TOKENS the function is a documented no-op (always true)
//   * a tx with no token outputs, no envelope outputs, and no token-bearing inputs passes

namespace {

CScript RandomScript(FuzzedDataProvider &provider, size_t max_len) {
    const size_t len = provider.ConsumeIntegralInRange<size_t>(0, max_len);
    const std::vector<uint8_t> bytes = provider.ConsumeBytes<uint8_t>(len);
    return CScript(bytes.begin(), bytes.end());
}

token::OutputDataPtr RandomToken(FuzzedDataProvider &provider) {
    // Build a structurally coherent in-memory token (deserialization validity is CheckTxTokens'
    // domain; CheckDnftRules sees already-validated token objects).
    token::Id category;
    const std::vector<uint8_t> cat = provider.ConsumeBytes<uint8_t>(32);
    for (size_t i = 0; i < cat.size() && i < 32; ++i) {
        *(category.begin() + i) = cat[i]; // avoids memcpy(dst, nullptr, 0) UB on an empty draw
    }

    const uint8_t capability = provider.ConsumeIntegralInRange<uint8_t>(0, 2);
    const bool has_nft = provider.ConsumeBool();
    const size_t commitment_len =
        has_nft ? provider.ConsumeIntegralInRange<size_t>(0, 40) : 0;
    std::vector<uint8_t> commitment = provider.ConsumeBytes<uint8_t>(commitment_len);
    // Half the time, force an inscribed-looking commitment (0x01 prefix).
    if (has_nft && !commitment.empty() && provider.ConsumeBool()) {
        commitment[0] = dnft::BINDING_VERSION;
    }
    const int64_t amount = provider.ConsumeBool()
                               ? provider.ConsumeIntegralInRange<int64_t>(0, 1000)
                               : 0;

    uint8_t bitfield = 0;
    if (has_nft) {
        bitfield |= uint8_t(token::Structure::HasNFT) | capability;
        if (!commitment.empty()) {
            bitfield |= uint8_t(token::Structure::HasCommitmentLength);
        }
    }
    if (amount > 0) {
        bitfield |= uint8_t(token::Structure::HasAmount);
    }
    if (bitfield == 0) {
        bitfield = uint8_t(token::Structure::HasNFT);
    }

    token::OutputDataPtr ptr;
    ptr.emplace();
    ptr->SetId(category);
    token::NFTCommitment c;
    c.assign(commitment.begin(), commitment.end());
    ptr->SetCommitment(c, /*autoSetBitfield=*/false);
    ptr->SetAmount(token::SafeAmount::fromInt(amount).value_or(token::SafeAmount()),
                   /*autoSetBitfield=*/false);
    ptr->SetBitfieldUnchecked(bitfield); // last, so nothing auto-adjusts it
    return ptr;
}

} // namespace

void test_one_input(Span<const uint8_t> buffer) {
    static const Consensus::Params *params = [] {
        SelectParams(CBaseChainParams::REGTEST);
        return &::Params().GetConsensus();
    }();

    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // ---- the transaction: a length-prefixed serialized chunk from the stream ----
    const size_t tx_len =
        provider.ConsumeIntegralInRange<size_t>(0, provider.remaining_bytes());
    const std::vector<uint8_t> tx_bytes = provider.ConsumeBytes<uint8_t>(tx_len);
    CMutableTransaction mtx;
    try {
        GenericVectorReader vr(SER_NETWORK, INIT_PROTO_VERSION, tx_bytes, 0);
        vr >> mtx;
    } catch (const std::ios_base::failure &) {
        return;
    }
    if (mtx.vin.empty() || mtx.vin.size() > 64 || mtx.vout.size() > 64) {
        return; // keep the per-input coin synthesis bounded
    }
    const CTransaction tx(mtx);

    // ---- activation context ----
    g_DU1HeightOverride = 0;
    g_FTForkHeightOverride =
        provider.ConsumeBool() ? std::optional<int32_t>(0) : std::nullopt;
    const int nHeight = provider.ConsumeIntegralInRange<int>(1, 1'000'000);
    const uint32_t flags =
        provider.ConsumeBool() ? SCRIPT_ENABLE_TOKENS : 0;

    // ---- synthesize input coins ----
    CCoinsView dummy;
    CCoinsViewCache view(&dummy);
    bool any_token_input = false;
    for (const CTxIn &in : tx.vin) {
        const Amount value = int64_t(provider.ConsumeIntegralInRange<uint32_t>(1, 1'000'000)) * SATOSHI;
        CScript spk = RandomScript(provider, 64);
        token::OutputDataPtr tok;
        switch (provider.ConsumeIntegralInRange<int>(0, 3)) {
            case 0:
                break; // plain coin
            case 1:
                tok = RandomToken(provider);
                break;
            default: {
                // Clone a token identity from one of this tx's OWN outputs — this is what makes
                // the move / parent / burn classification paths reachable.
                std::vector<const token::OutputData *> outs;
                for (const CTxOut &o : tx.vout) {
                    if (o.tokenDataPtr) {
                        outs.push_back(&*o.tokenDataPtr);
                    }
                }
                if (!outs.empty()) {
                    const size_t pick =
                        provider.ConsumeIntegralInRange<size_t>(0, outs.size() - 1);
                    tok.emplace(*outs[pick]);
                    if (provider.ConsumeBool()) {
                        // Sometimes flip the capability so identity matches but class differs.
                        tok->SetBitfieldUnchecked(
                            (tok->GetBitfieldByte() & 0xf0u) |
                            provider.ConsumeIntegralInRange<uint8_t>(0, 2));
                    }
                }
                break;
            }
        }
        if (tok) {
            any_token_input = true;
        }
        CTxOut out(value, spk);
        out.tokenDataPtr = tok;
        view.AddCoin(in.prevout, Coin(out, 1, /*coinbase=*/false), /*overwrite=*/true);
    }

    // ---- the calls + invariants ----
    CValidationState state1;
    const bool ret1 = dnft::CheckDnftRules(tx, state1, view, flags, nHeight, *params);
    CValidationState state2;
    const bool ret2 = dnft::CheckDnftRules(tx, state2, view, flags, nHeight, *params);

    // Determinism: identical inputs, identical verdict + reason.
    assert(ret1 == ret2);
    assert(state1.GetRejectReason() == state2.GetRejectReason());

    // Documented no-op without the token flag.
    if (!(flags & SCRIPT_ENABLE_TOKENS)) {
        assert(ret1);
    }

    // A tx that touches no token/envelope surface must pass.
    if (!tx.IsCoinBase() && (flags & SCRIPT_ENABLE_TOKENS)) {
        bool touches = any_token_input;
        for (const CTxOut &o : tx.vout) {
            if (o.tokenDataPtr || dnft::IsDnftEnvelope(o.scriptPubKey)) {
                touches = true;
                break;
            }
        }
        if (!touches) {
            assert(ret1);
        }
    }

    g_DU1HeightOverride.reset();
    g_FTForkHeightOverride.reset();
}
