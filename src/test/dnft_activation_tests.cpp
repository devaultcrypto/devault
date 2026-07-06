// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/dnft.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_flags.h>
#include <validation.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <limits>
#include <optional>

// Phase 4A of DEVAULT_NFT_SPEC.md: DU1 activation scaffolding + the FT-deferral gate (§10.8).
// The 4C binding-rule tests live in dnft_binding_tests (future).

namespace {

/// RAII helper: set the DU1 / FT-fork height overrides for the scope of a test.
struct ActivationOverrides {
    std::optional<int32_t> origDU1, origFT;
    ActivationOverrides(std::optional<int32_t> du1, std::optional<int32_t> ft)
        : origDU1(g_DU1HeightOverride), origFT(g_FTForkHeightOverride) {
        g_DU1HeightOverride = du1;
        g_FTForkHeightOverride = ft;
    }
    ~ActivationOverrides() {
        g_DU1HeightOverride = origDU1;
        g_FTForkHeightOverride = origFT;
    }
};

/// Build a 1-in/1-out tx whose output optionally carries token data.
CMutableTransaction MakeTxWithTokenOutput(const token::OutputDataPtr &tokenData) {
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout = COutPoint(TxId(uint256S("11")), 1); // any non-null, non-vout-0 prevout
    tx.vout.resize(1);
    tx.vout[0].nValue = 1000 * COIN;
    tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    tx.vout[0].tokenDataPtr = tokenData;
    return tx;
}

token::OutputDataPtr MakeFtTokenData() {
    // Fungible-only token data: amount, no NFT.
    return token::OutputDataPtr{
        token::OutputData(token::Id{uint256S("22")}, token::SafeAmount::fromInt(100).value())};
}

token::OutputDataPtr MakeNftTokenData() {
    // Pure NFT (immutable, no fungible amount).
    return token::OutputDataPtr{token::OutputData(token::Id{uint256S("33")},
                                                  token::SafeAmount::fromInt(0).value(),
                                                  token::NFTCommitment{}, /* hasNFT = */ true)};
}

bool RunCheck(const CMutableTransaction &mtx, uint32_t flags, int nHeight, std::string *reason = nullptr) {
    const CTransaction tx(mtx);
    CValidationState state;
    CCoinsView dummyBase;
    CCoinsViewCache dummyView(&dummyBase);
    const bool ok =
        dnft::CheckDnftRules(tx, state, dummyView, flags, nHeight, ::Params().GetConsensus());
    if (reason) *reason = state.GetRejectReason();
    return ok;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(dnft_activation_tests, BasicTestingSetup)

// The DU1/FT activation predicates: override plumbing + exact boundary semantics
// (activation height H = the last pre-fork block; rules apply to blocks with prev height >= H).
BOOST_AUTO_TEST_CASE(activation_predicates) {
    const auto &params = ::Params().GetConsensus();

    // Defaults are the chainparams sentinels (disabled).
    BOOST_CHECK_EQUAL(GetDU1ActivationHeight(params), params.du1Height);
    BOOST_CHECK_EQUAL(GetFTForkActivationHeight(params), params.ftForkHeight);
    BOOST_CHECK_EQUAL(params.du1Height, 2000000000);
    BOOST_CHECK_EQUAL(params.ftForkHeight, 2000000000);

    {
        ActivationOverrides ov(1000, 2000);
        BOOST_CHECK_EQUAL(GetDU1ActivationHeight(params), 1000);
        BOOST_CHECK_EQUAL(GetFTForkActivationHeight(params), 2000);

        // Boundary exactness: prev height 999 -> not active; 1000 -> active (first new-rules
        // block is 1001, "one past the activation block", matching the upgrade9 convention).
        BOOST_CHECK(!IsDU1EnabledForHeightPrev(params, 999));
        BOOST_CHECK(IsDU1EnabledForHeightPrev(params, 1000));
        BOOST_CHECK(IsDU1EnabledForHeightPrev(params, 1001));
        BOOST_CHECK(!IsFTForkEnabledForHeightPrev(params, 1999));
        BOOST_CHECK(IsFTForkEnabledForHeightPrev(params, 2000));
    }

    // Overrides restored.
    BOOST_CHECK_EQUAL(GetDU1ActivationHeight(params), params.du1Height);
    BOOST_CHECK_EQUAL(GetFTForkActivationHeight(params), params.ftForkHeight);
}

// GetNextBlockScriptFlags (via the GetMemPoolScriptFlags out-param) must add exactly the DU1
// VM bundle at activation: tokens + native introspection + 64-bit ints + p2sh32 — and, per
// spec Q4, NOT the MAY2026 flag (commitments stay at the 40-byte limit).
BOOST_FIXTURE_TEST_CASE(script_flags_at_du1, TestChain100Setup) {
    const auto &params = ::Params().GetConsensus();
    const CBlockIndex *tip = [] {
        LOCK(cs_main);
        return ::ChainActive().Tip();
    }();
    BOOST_REQUIRE(tip != nullptr && tip->nHeight == 100);

    constexpr uint32_t DU1_FLAGS = SCRIPT_ENABLE_TOKENS | SCRIPT_NATIVE_INTROSPECTION |
                                   SCRIPT_64_BIT_INTEGERS | SCRIPT_ENABLE_P2SH_32;

    { // not yet active (activation block is in the future)
        ActivationOverrides ov(tip->nHeight + 1, std::nullopt);
        uint32_t nextBlockFlags = 0;
        GetMemPoolScriptFlags(params, tip, &nextBlockFlags);
        BOOST_CHECK_EQUAL(nextBlockFlags & DU1_FLAGS, 0u);
    }
    { // active as of the tip -> next block carries the whole DU1 bundle, and nothing extra
        ActivationOverrides ov(tip->nHeight, std::nullopt);
        uint32_t nextBlockFlags = 0;
        GetMemPoolScriptFlags(params, tip, &nextBlockFlags);
        BOOST_CHECK_EQUAL(nextBlockFlags & DU1_FLAGS, DU1_FLAGS);
        BOOST_CHECK_EQUAL(nextBlockFlags & SCRIPT_ENABLE_MAY2026, 0u);
    }
}

// The FT-deferral gate (spec §10.8 / amendment A2): from DU1 until the FT fork, any output
// carrying a fungible token amount is invalid; pure-NFT outputs pass.
BOOST_AUTO_TEST_CASE(ft_deferral_gate) {
    const uint32_t tokenFlags = SCRIPT_ENABLE_TOKENS;
    const int nHeight = 500; // the block being validated
    std::string reason;

    // FT fork NOT active (sentinel): FT-amount output rejected with the precise reason.
    {
        ActivationOverrides ov(100, std::nullopt); // DU1 active well below nHeight; FT sentinel
        BOOST_CHECK(!RunCheck(MakeTxWithTokenOutput(MakeFtTokenData()), tokenFlags, nHeight, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-token-ft-deferred");

        // Pure NFT output: allowed.
        BOOST_CHECK(RunCheck(MakeTxWithTokenOutput(MakeNftTokenData()), tokenFlags, nHeight));

        // No token data at all: allowed.
        BOOST_CHECK(RunCheck(MakeTxWithTokenOutput(token::OutputDataPtr{}), tokenFlags, nHeight));
    }

    // FT fork active for this height: FT-amount output passes the DNFT layer
    // (CheckTxTokens still owns the inherited token rules).
    {
        ActivationOverrides ov(100, nHeight - 1);
        BOOST_CHECK(RunCheck(MakeTxWithTokenOutput(MakeFtTokenData()), tokenFlags, nHeight));
    }

    // FT fork activates exactly at prev-height boundary: nHeight-1 >= ftFork -> allowed;
    // one block earlier -> still rejected.
    {
        ActivationOverrides ov(100, nHeight);
        BOOST_CHECK(!RunCheck(MakeTxWithTokenOutput(MakeFtTokenData()), tokenFlags, nHeight, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-token-ft-deferred");
        BOOST_CHECK(RunCheck(MakeTxWithTokenOutput(MakeFtTokenData()), tokenFlags, nHeight + 1));
    }

    // Pre-DU1 (no SCRIPT_ENABLE_TOKENS): the whole function is a no-op regardless of outputs
    // (the PATFO rules own the pre-activation space).
    {
        ActivationOverrides ov(std::nullopt, std::nullopt);
        BOOST_CHECK(RunCheck(MakeTxWithTokenOutput(MakeFtTokenData()), SCRIPT_VERIFY_NONE, nHeight));
    }
}

BOOST_AUTO_TEST_SUITE_END()
