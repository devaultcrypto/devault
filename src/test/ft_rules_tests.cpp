// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Phase 5C of DEVAULT_FT_SPEC.md: the fungible-token consensus core — the stateless emission
// schedule (§6.1), deploy validity (§6.2), mint validity (§6.3) and the CheckTxTokens ex-nihilo
// carve-out (§6.4). These drive dnft::CheckFtRules / dnft::ValidateFtDeploy directly against a
// hand-built CCoinsViewCache and a real (in-memory) deploy registry.

#include <devault/ft.h>
#include <devault/ft_envelope.h>
#include <devault/ft_registry.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <consensus/tokens.h>
#include <consensus/validation.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_flags.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <limits>
#include <optional>
#include <string>
#include <vector>

using namespace dnft;

namespace {

constexpr uint32_t TOKEN_FLAGS = SCRIPT_ENABLE_TOKENS;
constexpr int DEPLOY_HEIGHT = 1000;
constexpr int DU1 = 100;
constexpr int FTFORK = 200;

/// RAII: set the DU1/FT activation overrides and install a fresh in-memory registry.
struct FtTestEnv {
    std::optional<int32_t> origDU1, origFT;
    FtTestEnv() : origDU1(g_DU1HeightOverride), origFT(g_FTForkHeightOverride) {
        g_DU1HeightOverride = DU1;
        g_FTForkHeightOverride = FTFORK;
        g_ftRegistry = std::make_unique<CFtRegistry>(
            std::make_unique<CFtRegistryDB>(fs::path(), 1 << 20, /*fMemory=*/true, /*fWipe=*/true));
    }
    ~FtTestEnv() {
        g_DU1HeightOverride = origDU1;
        g_FTForkHeightOverride = origFT;
        g_ftRegistry.reset();
    }
};

std::vector<uint8_t> S(const std::string &s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

TxId MakeTxId(uint8_t seed) {
    uint256 h;
    h.begin()[0] = seed;
    h.begin()[1] = 0xAA;
    return TxId(h);
}

token::OutputDataPtr FtToken(const token::Id &id, int64_t amount) {
    return token::OutputDataPtr{token::OutputData(id, token::SafeAmount::fromInt(amount).value())};
}

CTxOut PlainOut(Amount v = 10 * COIN) {
    CTxOut o;
    o.nValue = v;
    o.scriptPubKey = CScript() << OP_TRUE;
    return o;
}

CTxOut TokenOut(const token::Id &id, int64_t amount) {
    CTxOut o = PlainOut();
    o.tokenDataPtr = FtToken(id, amount);
    return o;
}

CTxOut EnvelopeOut(const CScript &spk) {
    CTxOut o;
    o.nValue = Amount::zero();
    o.scriptPubKey = spk;
    return o;
}

//! Canonical open-mint deploy params (Q=100, M=5, start=DEPLOY_HEIGHT+1, N=1000, premine=0).
FtDeployParams OpenParams(uint32_t start, uint64_t q = 100, uint64_t m = 5, uint64_t n = 1000,
                          uint64_t premine = 0) {
    FtDeployParams p;
    p.symbol = S("GOLD");
    p.name = S("Gold");
    p.decimals = 2;
    p.mode = FT_MODE_OPEN;
    p.quantity_per_mint = q;
    p.per_block_limit = m;
    p.start_height = start;
    p.max_mints = n;
    p.premine = premine;
    return p;
}

/// A transaction whose vin[0] spends (genesisTxid, 0) — so the deployed category is genesisTxid.
CMutableTransaction DeployTx(const TxId &genesisTxid, const CScript &envelope,
                             const std::vector<CTxOut> &extraOuts = {}) {
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout = COutPoint(genesisTxid, 0);
    for (const CTxOut &o : extraOuts) {
        tx.vout.push_back(o);
    }
    tx.vout.push_back(EnvelopeOut(envelope));
    return tx;
}

/// Test harness: a coins view we can seed with input coins.
struct Harness {
    CCoinsView base;
    CCoinsViewCache view{&base};

    //! Add a spendable coin at `op`, optionally carrying `amount` fungible tokens of `id`.
    void AddCoin(const COutPoint &op, const token::Id *id = nullptr, int64_t amount = 0) {
        CTxOut out = PlainOut();
        if (id) {
            out.tokenDataPtr = FtToken(*id, amount);
        }
        view.AddCoin(op, Coin(out, 1, false), false);
    }

    bool Run(const CMutableTransaction &mtx, int nHeight, FtBlockContext *ctx,
             token::FtMintAllowance *allowance, std::string *reason = nullptr) {
        const CTransaction tx(mtx);
        CValidationState state;
        token::FtMintAllowance local;
        const bool ok = CheckFtRules(tx, state, view, TOKEN_FLAGS, nHeight,
                                     ::Params().GetConsensus(), ctx,
                                     allowance ? allowance : &local);
        if (reason) *reason = state.GetRejectReason();
        return ok;
    }

    //! Run CheckFtRules then CheckTxTokens exactly as ConnectBlock does (FT first, allowance fed in).
    bool RunFull(const CMutableTransaction &mtx, int nHeight, FtBlockContext *ctx,
                 std::string *reason = nullptr) {
        const CTransaction tx(mtx);
        CValidationState state;
        token::FtMintAllowance allowance;
        if (!CheckFtRules(tx, state, view, TOKEN_FLAGS, nHeight, ::Params().GetConsensus(), ctx,
                          &allowance)) {
            if (reason) *reason = state.GetRejectReason();
            return false;
        }
        const bool ok = CheckTxTokens(tx, state, view, TOKEN_FLAGS, /*firstTokenBlockHeight=*/1,
                                      &allowance);
        if (reason) *reason = state.GetRejectReason();
        return ok;
    }
};

//! Register an open deploy in the live registry and return (deploy txid, category).
std::pair<TxId, token::Id> RegisterOpenDeploy(Harness &h, uint8_t seed, const FtDeployParams &params,
                                              int height = DEPLOY_HEIGHT) {
    const TxId genesis = MakeTxId(seed);
    h.AddCoin(COutPoint(genesis, 0));
    CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(params));
    if (params.premine > 0) {
        mtx.vout.insert(mtx.vout.begin(), TokenOut(token::Id{genesis}, int64_t(params.premine)));
    }
    const CTransaction tx(mtx);
    FtDeployRecord rec;
    bool isOpen = false;
    CValidationState state;
    BOOST_REQUIRE_MESSAGE(ValidateFtDeploy(tx, height, rec, isOpen, state), state.GetRejectReason());
    BOOST_REQUIRE(isOpen);
    FtBlockContext ctx;
    ctx.pendingDeploys.emplace_back(tx.GetId(), rec);
    g_ftRegistry->ApplyBlock(ctx, BlockHash());
    return {tx.GetId(), token::Id{genesis}};
}

//! A mint tx: marker naming `deploy`, creating `outAmount` of `cat`, optionally spending `inAmount`.
CMutableTransaction MintTx(const TxId &deploy, const token::Id &cat, int64_t outAmount,
                           const COutPoint &fundingCoin, const COutPoint *tokenIn = nullptr,
                           int64_t inAmount = 0) {
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout = fundingCoin;
    if (tokenIn) {
        tx.vin.emplace_back();
        tx.vin[1].prevout = *tokenIn;
    }
    (void)inAmount;
    std::array<uint8_t, FT_DEPLOY_TXID_LENGTH> d{};
    std::memcpy(d.data(), deploy.begin(), d.size());
    if (outAmount > 0) {
        tx.vout.push_back(TokenOut(cat, outAmount));
    }
    tx.vout.push_back(EnvelopeOut(BuildFtMintEnvelope(d)));
    return tx;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(ft_rules_tests, BasicTestingSetup)

// ---------------------------------------------------------------- the stateless schedule

// The heart of the design (spec §6.1): the per-height allowance sums to EXACTLY N over the window,
// so the cumulative cap is enforced with no stored counter anywhere.
BOOST_AUTO_TEST_CASE(schedule_limit_sums_to_exactly_n) {
    auto rec = [](uint64_t n, uint64_t m, uint32_t s) {
        FtDeployRecord r;
        r.maxMints = n;
        r.perBlockLimit = m;
        r.startHeight = s;
        r.quantity = 1;
        return r;
    };

    // (N, M) pairs incl. non-multiples (the exact-N final block, O5) and N < M.
    for (const auto &[n, m] : std::vector<std::pair<uint64_t, uint64_t>>{
             {1000, 5}, {1003, 5}, {1, 1}, {3, 10}, {10, 10}, {17, 4}, {999983, 7}}) {
        const uint32_t s = 500;
        const FtDeployRecord r = rec(n, m, s);
        uint64_t total = 0;
        // Walk well past the implied window end.
        for (uint64_t h = s; h <= s + n / m + 5; ++h) {
            const uint64_t lim = FtScheduleLimit(r, int(h));
            BOOST_CHECK_LE(lim, m);
            total += lim;
        }
        BOOST_CHECK_MESSAGE(total == n,
                            strprintf("N=%d M=%d: schedule summed to %d, expected exactly N", n, m,
                                      total));
        // Before the window: closed. Far past it: closed.
        BOOST_CHECK_EQUAL(FtScheduleLimit(r, int(s) - 1), 0u);
        BOOST_CHECK_EQUAL(FtScheduleLimit(r, int(s + n / m + 100)), 0u);
    }
}

// (h - s) * M must be computed in 128-bit: with 64-bit arithmetic it WRAPS and reopens a window
// that consensus must consider closed. This case has teeth — see the comment on the last check.
BOOST_AUTO_TEST_CASE(schedule_limit_no_overflow) {
    const uint64_t U64MAX = std::numeric_limits<uint64_t>::max();
    FtDeployRecord r;
    r.maxMints = U64MAX;      // N
    r.perBlockLimit = U64MAX; // M == N, so the entire cap is consumable in the very first block
    r.startHeight = 0;
    r.quantity = 1;

    BOOST_CHECK_EQUAL(FtScheduleLimit(r, -1), 0u);       // before the window
    BOOST_CHECK_EQUAL(FtScheduleLimit(r, 0), U64MAX);    // h == s: the full cap is available
    BOOST_CHECK_EQUAL(FtScheduleLimit(r, 1), 0u);        // ...and is then exhausted
    // h - s = 2e9. In 64-bit, (h - s) * M wraps to 2^64 - 2e9, making `remaining` compute to
    // 1999999999 > 0 — a REOPENED window and an unbounded over-mint. The 128-bit path sees the true
    // product (~3.7e28 >> N) and correctly reports the window closed.
    BOOST_CHECK_EQUAL(FtScheduleLimit(r, 2000000000), 0u);

    // A non-degenerate large-height case: the window is genuinely open far from the start.
    FtDeployRecord w;
    w.maxMints = 1000;
    w.perBlockLimit = 5;
    w.startHeight = 1999999000;
    w.quantity = 1;
    BOOST_CHECK_EQUAL(FtScheduleLimit(w, 1999999000), 5u);
    BOOST_CHECK_EQUAL(FtScheduleLimit(w, 1999999199), 5u); // last full block (200 blocks x 5 = 1000)
    BOOST_CHECK_EQUAL(FtScheduleLimit(w, 1999999200), 0u); // cap exhausted
}

// ---------------------------------------------------------------- deploy validity

BOOST_AUTO_TEST_CASE(deploy_valid_open_and_fixed) {
    FtTestEnv env;
    Harness h;

    // Open deploy with a premine: the record is derived exactly from the envelope.
    const TxId genesis = MakeTxId(1);
    h.AddCoin(COutPoint(genesis, 0));
    const FtDeployParams p = OpenParams(DEPLOY_HEIGHT + 1, 100, 5, 1000, 777);
    CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(p),
                                       {TokenOut(token::Id{genesis}, 777)});
    FtDeployRecord rec;
    bool isOpen = false;
    CValidationState state;
    BOOST_REQUIRE_MESSAGE(ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, rec, isOpen, state),
                          state.GetRejectReason());
    BOOST_CHECK(isOpen);
    BOOST_CHECK(rec.category == token::Id{genesis});
    BOOST_CHECK_EQUAL(rec.quantity, 100u);
    BOOST_CHECK_EQUAL(rec.perBlockLimit, 5u);
    BOOST_CHECK_EQUAL(rec.startHeight, uint32_t(DEPLOY_HEIGHT + 1));
    BOOST_CHECK_EQUAL(rec.maxMints, 1000u);
    BOOST_CHECK_EQUAL(rec.premine, 777u);
    BOOST_CHECK_EQUAL(rec.deployHeight, uint32_t(DEPLOY_HEIGHT));

    // A FIXED deploy is valid but produces NO registry record (it can never be minted).
    FtDeployParams f;
    f.symbol = S("USD");
    f.name = S("Dollar");
    f.decimals = 8;
    f.mode = FT_MODE_FIXED;
    const TxId g2 = MakeTxId(2);
    h.AddCoin(COutPoint(g2, 0));
    CMutableTransaction fx = DeployTx(g2, BuildFtDeployEnvelope(f),
                                      {TokenOut(token::Id{g2}, 21000000)});
    FtDeployRecord rec2;
    bool open2 = true;
    CValidationState st2;
    BOOST_REQUIRE_MESSAGE(ValidateFtDeploy(CTransaction(fx), DEPLOY_HEIGHT, rec2, open2, st2),
                          st2.GetRejectReason());
    BOOST_CHECK(!open2); // fixed => not registered
}

BOOST_AUTO_TEST_CASE(deploy_reject_matrix) {
    FtTestEnv env;
    Harness h;
    std::string reason;

    // vin[0] must spend an index-0 prevout (that is what makes the category unambiguous).
    {
        const TxId genesis = MakeTxId(3);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1)));
        mtx.vin[0].prevout = COutPoint(genesis, 1); // index 1
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-deploy-no-genesis-input");
    }

    // O4: start height must be STRICTLY after the deploy height.
    for (uint32_t start : {uint32_t(DEPLOY_HEIGHT), uint32_t(DEPLOY_HEIGHT - 1)}) {
        const TxId genesis = MakeTxId(4);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(start)));
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-deploy-start-not-after-deploy");
    }

    // Premine mismatch: the tx creates a fungible amount of its own category != the declared premine.
    {
        const TxId genesis = MakeTxId(5);
        const auto p = OpenParams(DEPLOY_HEIGHT + 1, 100, 5, 1000, /*premine=*/500);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(p),
                                           {TokenOut(token::Id{genesis}, 499)}); // one short
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-deploy-premine-mismatch");
    }
    // ... and declaring premine 0 while creating tokens is likewise rejected.
    {
        const TxId genesis = MakeTxId(6);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1)),
                                           {TokenOut(token::Id{genesis}, 1)});
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-deploy-premine-mismatch");
    }

    // Supply overflow: premine + N*Q must fit the token amount ceiling.
    {
        const TxId genesis = MakeTxId(7);
        const auto p = OpenParams(DEPLOY_HEIGHT + 1, /*q=*/uint64_t(1) << 40,
                                  /*m=*/1, /*n=*/uint64_t(1) << 40);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(p));
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-deploy-supply-overflow");
    }

    // At most one DVFT envelope per transaction.
    {
        const TxId genesis = MakeTxId(8);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1)));
        mtx.vout.push_back(EnvelopeOut(BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 2))));
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-multiple-envelopes");
    }

    // A DVFT envelope output may not carry token data.
    {
        const TxId genesis = MakeTxId(9);
        CMutableTransaction mtx = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1)));
        mtx.vout.back().tokenDataPtr = FtToken(token::Id{genesis}, 5);
        FtDeployRecord r;
        bool o;
        CValidationState st;
        BOOST_CHECK(!ValidateFtDeploy(CTransaction(mtx), DEPLOY_HEIGHT, r, o, st));
        BOOST_CHECK_EQUAL(st.GetRejectReason(), "bad-txns-ft-envelope-has-token");
    }
}

// ---------------------------------------------------------------- mint validity

BOOST_AUTO_TEST_CASE(mint_valid_and_carve_out_is_exact) {
    FtTestEnv env;
    Harness h;
    const auto [deploy, cat] = RegisterOpenDeploy(h, 20, OpenParams(DEPLOY_HEIGHT + 1));
    const int mintHeight = DEPLOY_HEIGHT + 1;

    const COutPoint funding(MakeTxId(21), 3);
    h.AddCoin(funding);

    // A clean mint of exactly Q: valid, and the allowance names (X, Q).
    {
        FtBlockContext ctx;
        token::FtMintAllowance allowance;
        std::string reason;
        BOOST_CHECK_MESSAGE(h.Run(MintTx(deploy, cat, 100, funding), mintHeight, &ctx, &allowance,
                                  &reason),
                            reason);
        BOOST_CHECK(allowance.IsSet());
        BOOST_CHECK(allowance.category == cat);
        BOOST_CHECK_EQUAL(allowance.amount, 100);
        // ...and the whole ConnectBlock pair (FT rules then CheckTxTokens) accepts it: the carve-out
        // lets exactly Q through the inherited conservation.
        FtBlockContext ctx2;
        BOOST_CHECK_MESSAGE(h.RunFull(MintTx(deploy, cat, 100, funding), mintHeight, &ctx2, &reason),
                            reason);
    }

    // Q+1 and Q-1 are both rejected by the exact-net rule (spec §6.3).
    for (int64_t bad : {int64_t(99), int64_t(101)}) {
        FtBlockContext ctx;
        std::string reason;
        BOOST_CHECK(!h.Run(MintTx(deploy, cat, bad, funding), mintHeight, &ctx, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-quantity");
    }

    // Net accounting: a mint that ALSO spends 50 of X must output 150 (net creation still Q).
    {
        const COutPoint tokenIn(MakeTxId(22), 0);
        h.AddCoin(tokenIn, &cat, 50);
        FtBlockContext ctx;
        std::string reason;
        BOOST_CHECK_MESSAGE(
            h.RunFull(MintTx(deploy, cat, 150, funding, &tokenIn), mintHeight, &ctx, &reason),
            reason);
        // ...but outputting 151 (net Q+1) is rejected.
        FtBlockContext ctx2;
        BOOST_CHECK(!h.RunFull(MintTx(deploy, cat, 151, funding, &tokenIn), mintHeight, &ctx2, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-quantity");
    }
}

BOOST_AUTO_TEST_CASE(mint_reject_matrix) {
    FtTestEnv env;
    Harness h;
    const auto [deploy, cat] = RegisterOpenDeploy(h, 30, OpenParams(DEPLOY_HEIGHT + 1));
    const int mintHeight = DEPLOY_HEIGHT + 1;
    const COutPoint funding(MakeTxId(31), 3);
    h.AddCoin(funding);
    std::string reason;

    // A marker naming an unregistered deploy (e.g. a fixed deploy, or garbage).
    {
        FtBlockContext ctx;
        BOOST_CHECK(!h.Run(MintTx(MakeTxId(99), cat, 100, funding), mintHeight, &ctx, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-no-deploy");
    }

    // A marker for a real deploy that mints a DIFFERENT category: the registry cross-check makes
    // the net creation of registry[D].category zero, so the exact-Q rule rejects it.
    {
        const token::Id other{MakeTxId(98)};
        FtBlockContext ctx;
        BOOST_CHECK(!h.Run(MintTx(deploy, other, 100, funding), mintHeight, &ctx, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-quantity");
    }

    // Window edges: before start, and past the end of the cap.
    {
        FtBlockContext ctx;
        BOOST_CHECK(!h.Run(MintTx(deploy, cat, 100, funding), DEPLOY_HEIGHT, &ctx, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-schedule");
        // N=1000, M=5 => the window spans 200 blocks; well past that it is closed.
        FtBlockContext ctx2;
        BOOST_CHECK(!h.Run(MintTx(deploy, cat, 100, funding), DEPLOY_HEIGHT + 1 + 200, &ctx2, nullptr,
                           &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-schedule");
    }

    // Per-block cap: M = 5 mints of this deploy fit in one block; the 6th does not.
    {
        FtBlockContext ctx;
        for (int i = 0; i < 5; ++i) {
            BOOST_CHECK_MESSAGE(h.Run(MintTx(deploy, cat, 100, funding), mintHeight, &ctx, nullptr,
                                      &reason),
                                strprintf("mint %d should fit: %s", i, reason));
        }
        BOOST_CHECK(!h.Run(MintTx(deploy, cat, 100, funding), mintHeight, &ctx, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-schedule");
    }

    // The mempool path (null ctx) does NOT enforce the per-block count — it only requires the window
    // to be open (spec §6.6). Many mints in a row are individually acceptable there.
    {
        for (int i = 0; i < 20; ++i) {
            BOOST_CHECK(h.Run(MintTx(deploy, cat, 100, funding), mintHeight, /*ctx=*/nullptr, nullptr,
                              &reason));
        }
        // ...but a closed window is still rejected in the mempool.
        BOOST_CHECK(!h.Run(MintTx(deploy, cat, 100, funding), DEPLOY_HEIGHT, nullptr, nullptr, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-ft-mint-schedule");
    }
}

// The carve-out must not leak: an UNMARKED transaction can still never increase any FT supply,
// and a marked mint's carve-out applies to its own category only.
BOOST_AUTO_TEST_CASE(carve_out_does_not_leak) {
    FtTestEnv env;
    Harness h;
    const auto [deploy, cat] = RegisterOpenDeploy(h, 40, OpenParams(DEPLOY_HEIGHT + 1));
    const int mintHeight = DEPLOY_HEIGHT + 1;
    const COutPoint funding(MakeTxId(41), 3);
    h.AddCoin(funding);
    std::string reason;

    // (a) No marker at all: creating X out of thin air is rejected by plain CashTokens conservation.
    {
        CMutableTransaction mtx;
        mtx.nVersion = 2;
        mtx.vin.resize(1);
        mtx.vin[0].prevout = funding;
        mtx.vout.push_back(TokenOut(cat, 100));
        FtBlockContext ctx;
        BOOST_CHECK(!h.RunFull(mtx, mintHeight, &ctx, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-token-invalid-category");
    }

    // (b) A valid mint of X may NOT also conjure a second category ex nihilo.
    {
        const token::Id other{MakeTxId(42)};
        CMutableTransaction mtx = MintTx(deploy, cat, 100, funding);
        mtx.vout.insert(mtx.vout.begin(), TokenOut(other, 5)); // uninvited second category
        FtBlockContext ctx;
        BOOST_CHECK(!h.RunFull(mtx, mintHeight, &ctx, &reason));
        BOOST_CHECK_EQUAL(reason, "bad-txns-token-invalid-category");
    }

    // (c) A mint may still legitimately spend and forward OTHER categories it actually owns.
    {
        const token::Id other{MakeTxId(43)};
        const COutPoint otherIn(MakeTxId(44), 0);
        h.AddCoin(otherIn, &other, 7);
        CMutableTransaction mtx = MintTx(deploy, cat, 100, funding);
        mtx.vin.emplace_back();
        mtx.vin[1].prevout = otherIn;
        mtx.vout.insert(mtx.vout.begin(), TokenOut(other, 7)); // conserved, not created
        FtBlockContext ctx;
        BOOST_CHECK_MESSAGE(h.RunFull(mtx, mintHeight, &ctx, &reason), reason);
    }
}

// Activation gating: pre-DU1 and pre-FT-fork the rules are a no-op, and a coinbase is exempt.
BOOST_AUTO_TEST_CASE(gating) {
    FtTestEnv env;
    Harness h;
    const auto [deploy, cat] = RegisterOpenDeploy(h, 50, OpenParams(DEPLOY_HEIGHT + 1));
    const COutPoint funding(MakeTxId(51), 3);
    h.AddCoin(funding);

    // A transaction that would be a schedule-violating mint is simply ignored before the FT fork
    // (the FT-deferral gate in CheckDnftRules owns that space).
    const CMutableTransaction bad = MintTx(deploy, cat, 12345, funding);
    {
        const CTransaction tx(bad);
        CValidationState state;
        token::FtMintAllowance allowance;
        // height below FTFORK+1 => FT rules not yet in force
        BOOST_CHECK(CheckFtRules(tx, state, h.view, TOKEN_FLAGS, FTFORK, ::Params().GetConsensus(),
                                 nullptr, &allowance));
        BOOST_CHECK(!allowance.IsSet());
        // pre-DU1 (no token flag) => no-op as well
        BOOST_CHECK(CheckFtRules(tx, state, h.view, SCRIPT_VERIFY_NONE, DEPLOY_HEIGHT + 1,
                                 ::Params().GetConsensus(), nullptr, &allowance));
    }

    // Coinbase is exempt (it can never satisfy the vin[0] deploy rule, and CheckTxTokens forbids any
    // token output on it, so it can never be a mint either).
    {
        CMutableTransaction cb;
        cb.nVersion = 2;
        cb.vin.resize(1);
        cb.vin[0].prevout = COutPoint(); // null prevout => coinbase
        cb.vout.push_back(EnvelopeOut(BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1))));
        const CTransaction tx(cb);
        BOOST_REQUIRE(tx.IsCoinBase());
        CValidationState state;
        token::FtMintAllowance allowance;
        FtBlockContext ctx;
        BOOST_CHECK(CheckFtRules(tx, state, h.view, TOKEN_FLAGS, DEPLOY_HEIGHT + 1,
                                 ::Params().GetConsensus(), &ctx, &allowance));
        BOOST_CHECK(ctx.pendingDeploys.empty()); // nothing registered from a coinbase
    }
}

// The registry is write-once and reorg-exact: UndoBlock erases precisely the deploys a block added.
BOOST_AUTO_TEST_CASE(registry_apply_and_undo) {
    FtTestEnv env;
    Harness h;
    const auto [deploy, cat] = RegisterOpenDeploy(h, 60, OpenParams(DEPLOY_HEIGHT + 1));
    BOOST_CHECK(g_ftRegistry->Lookup(deploy) != nullptr);
    BOOST_CHECK_EQUAL(g_ftRegistry->Size(), 1u);

    // A block that contains the deploy tx (plus an unrelated tx) is disconnected: the deploy goes,
    // the unrelated erase is a harmless no-op.
    CBlock block;
    {
        CMutableTransaction unrelated;
        unrelated.nVersion = 2;
        unrelated.vin.resize(1);
        unrelated.vin[0].prevout = COutPoint(MakeTxId(61), 1);
        unrelated.vout.push_back(PlainOut());

        // Rebuild the exact deploy tx so the block carries its txid.
        const TxId genesis = MakeTxId(60);
        CMutableTransaction dep = DeployTx(genesis, BuildFtDeployEnvelope(OpenParams(DEPLOY_HEIGHT + 1)));
        BOOST_REQUIRE(CTransaction(dep).GetId() == deploy);

        block.vtx.push_back(MakeTransactionRef(unrelated));
        block.vtx.push_back(MakeTransactionRef(dep));
    }
    g_ftRegistry->UndoBlock(block, BlockHash());
    BOOST_CHECK(g_ftRegistry->Lookup(deploy) == nullptr);
    BOOST_CHECK_EQUAL(g_ftRegistry->Size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
