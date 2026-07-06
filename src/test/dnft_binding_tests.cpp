// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/dnft.h>
#include <devault/dnft_envelope.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/activation.h>
#include <consensus/validation.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_flags.h>
#include <uint256.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

using namespace dnft;

// Phase 4C: the DNFT binding rule (DEVAULT_NFT_SPEC.md §6 + §7). These tests drive CheckDnftRules
// directly with a hand-built coins view, isolating the DNFT layer from the inherited CashTokens
// conservation rules (CheckTxTokens, validated separately in token_tests and end-to-end in M9).

namespace {

// The DNFT rules run only when SCRIPT_ENABLE_TOKENS is set. Set both DU1 and the FT fork active for
// the whole suite so the (separately tested) FT-deferral gate never interferes with binding tests.
struct BindingSetup : public BasicTestingSetup {
    std::optional<int32_t> origDU1, origFT;
    BindingSetup() {
        origDU1 = g_DU1HeightOverride;
        origFT = g_FTForkHeightOverride;
        g_DU1HeightOverride = 0;
        g_FTForkHeightOverride = 0;
    }
    ~BindingSetup() {
        g_DU1HeightOverride = origDU1;
        g_FTForkHeightOverride = origFT;
    }
};

token::Id Cat(const char *hex) { return token::Id(uint256S(hex)); }

token::NFTCommitment Commit(const std::array<uint8_t, COMMITMENT_LENGTH> &a) {
    return token::NFTCommitment(a.begin(), a.end());
}
token::NFTCommitment CommitBytes(const std::vector<uint8_t> &v) {
    return token::NFTCommitment(v.begin(), v.end());
}

// token::OutputData factories
token::OutputDataPtr Inscribed(const token::Id &cat, const token::NFTCommitment &commit) {
    return token::OutputDataPtr{
        token::OutputData(cat, token::SafeAmount::fromInt(0).value(), commit, /*hasNFT*/ true)};
}
token::OutputDataPtr InscribedWithAmount(const token::Id &cat, const token::NFTCommitment &commit, int64_t amt) {
    return token::OutputDataPtr{
        token::OutputData(cat, token::SafeAmount::fromInt(amt).value(), commit, true)};
}
token::OutputDataPtr MutableNft(const token::Id &cat, const token::NFTCommitment &commit) {
    return token::OutputDataPtr{token::OutputData(cat, token::SafeAmount::fromInt(0).value(), commit,
                                                  true, /*mutable*/ true, false)};
}
token::OutputDataPtr MintingNft(const token::Id &cat, const token::NFTCommitment &commit) {
    return token::OutputDataPtr{token::OutputData(cat, token::SafeAmount::fromInt(0).value(), commit,
                                                  true, false, /*minting*/ true)};
}
token::OutputDataPtr FungibleOnly(const token::Id &cat, int64_t amt) {
    return token::OutputDataPtr{token::OutputData(cat, token::SafeAmount::fromInt(amt).value())};
}

const CScript kLock = CScript() << OP_TRUE; // dummy non-envelope spk for token outputs

// A scenario: a transaction plus a coins view holding its input coins.
struct Scene {
    CMutableTransaction tx;
    CCoinsView base;
    CCoinsViewCache view{&base};

    // Add an input spending `op`, backed by a coin with optional token data.
    void addInput(const COutPoint &op, token::OutputDataPtr td = {}) {
        tx.vin.emplace_back(op);
        view.AddCoin(op, Coin(CTxOut(COIN, kLock, std::move(td)), 100, false), false);
    }
    void addTokenOutput(const token::OutputDataPtr &td) { tx.vout.emplace_back(COIN, kLock, td); }
    void addEnvelope(const CScript &spk, token::OutputDataPtr td = {}) {
        tx.vout.emplace_back(COIN, spk, std::move(td));
    }
    void addPlainOutput() { tx.vout.emplace_back(COIN, kLock); }

    const COutPoint &input0() const { return tx.vin[0].prevout; }
};

std::pair<bool, std::string> Run(const Scene &s) {
    CValidationState state;
    const bool ok = dnft::CheckDnftRules(CTransaction(s.tx), state, s.view, SCRIPT_ENABLE_TOKENS,
                                         1000, ::Params().GetConsensus());
    return {ok, state.GetRejectReason()};
}

CScript SimpleEnvelope(const std::string &ct = "text/plain", const std::string &body = "hi") {
    EnvelopeFields f;
    f.content_type = std::vector<uint8_t>(ct.begin(), ct.end());
    return BuildDnftEnvelope(f, Span<const uint8_t>((const uint8_t *)body.data(), body.size()));
}

// The commitment a mint output must carry to bind to `envSpk` at output index `nftIdx` given input0.
token::NFTCommitment BindingCommit(const CScript &envSpk, const COutPoint &input0, uint32_t nftIdx) {
    return Commit(ComputeDnftCommitment(envSpk, input0, nftIdx));
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(dnft_binding_tests, BindingSetup)

// ---- valid cases ----

BOOST_AUTO_TEST_CASE(valid_single_mint) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("a0")), 0)); // plain funding input (input0 -> salt)
    const CScript env = SimpleEnvelope("image/png", "PNGDATA");
    s.addEnvelope(env);                              // vout 0
    // inscribed output at vout 1
    s.addTokenOutput(Inscribed(Cat("11"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK_MESSAGE(ok, "unexpected reject: " + why);
}

BOOST_AUTO_TEST_CASE(valid_batch_mint) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("a1")), 0));
    const CScript e0 = SimpleEnvelope("t/1", "one");
    const CScript e1 = SimpleEnvelope("t/2", "two");
    s.addEnvelope(e0);                                                  // vout 0
    s.addTokenOutput(Inscribed(Cat("21"), BindingCommit(e0, s.input0(), 1))); // vout 1
    s.addEnvelope(e1);                                                  // vout 2
    s.addTokenOutput(Inscribed(Cat("22"), BindingCommit(e1, s.input0(), 3))); // vout 3
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_CASE(valid_batch_order_independent) {
    // Layout [nft0, nft1, env0, env1]: pairing is by RELATIVE order in each list.
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("a2")), 0));
    const CScript e0 = SimpleEnvelope("t/1", "one");
    const CScript e1 = SimpleEnvelope("t/2", "two");
    // nft0 at vout 0, nft1 at vout 1, env0 at vout 2, env1 at vout 3
    s.addTokenOutput(Inscribed(Cat("31"), BindingCommit(e0, s.input0(), 0)));
    s.addTokenOutput(Inscribed(Cat("32"), BindingCommit(e1, s.input0(), 1)));
    s.addEnvelope(e0);
    s.addEnvelope(e1);
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_CASE(valid_later_mint_via_minting_token) {
    // Spend a minting-capability token; create a new inscribed item of the same category.
    Scene s;
    const token::Id coll = Cat("40");
    s.addInput(COutPoint(TxId(uint256S("a3")), 0), MintingNft(coll, CommitBytes({0xde, 0xad})));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);                                                // vout 0
    s.addTokenOutput(Inscribed(coll, BindingCommit(env, s.input0(), 1))); // vout 1 (new item)
    // (a real tx also re-creates the minting token to keep the collection open; not needed here)
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_CASE(valid_transfer_move_no_envelope) {
    // Move an existing inscribed item: input and output share the (category, commitment).
    Scene s;
    const token::Id cat = Cat("50");
    const auto commit = BindingCommit(SimpleEnvelope(), COutPoint(TxId(uint256S("zz")), 0), 0);
    s.addInput(COutPoint(TxId(uint256S("a4")), 0), Inscribed(cat, commit));
    s.addTokenOutput(Inscribed(cat, commit)); // same item, moved to a new UTXO; no envelope
    const auto [ok, why] = Run(s);
    BOOST_CHECK_MESSAGE(ok, "move rejected: " + why);
}

BOOST_AUTO_TEST_CASE(valid_burn) {
    Scene s;
    const token::Id cat = Cat("51");
    const auto commit = BindingCommit(SimpleEnvelope(), COutPoint(TxId(uint256S("zz")), 0), 0);
    s.addInput(COutPoint(TxId(uint256S("a5")), 0), Inscribed(cat, commit));
    s.addPlainOutput(); // token destroyed
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_CASE(valid_mixed_move_and_mint) {
    Scene s;
    const token::Id catA = Cat("60");
    const auto commitA = BindingCommit(SimpleEnvelope(), COutPoint(TxId(uint256S("zz")), 0), 0);
    s.addInput(COutPoint(TxId(uint256S("a6")), 0), Inscribed(catA, commitA)); // to be moved
    s.addTokenOutput(Inscribed(catA, commitA));                               // vout 0: move A
    const CScript env = SimpleEnvelope("t/x", "newitem");
    s.addEnvelope(env);                                                       // vout 1
    s.addTokenOutput(Inscribed(Cat("61"), BindingCommit(env, s.input0(), 2)));// vout 2: mint B
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_CASE(valid_parent_present) {
    Scene s;
    const token::Id parentCat = Cat("70");
    const auto parentCommit = BindingCommit(SimpleEnvelope(), COutPoint(TxId(uint256S("zz")), 0), 0);
    // input0 = the parent inscribed item (spend-to-prove)
    s.addInput(COutPoint(TxId(uint256S("a7")), 0), Inscribed(parentCat, parentCommit));
    // parent claim value = category(32) || commitment(33)
    std::vector<uint8_t> claim(parentCat.begin(), parentCat.end());
    claim.insert(claim.end(), parentCommit.begin(), parentCommit.end());
    EnvelopeFields f;
    f.content_type = std::vector<uint8_t>{'a'};
    f.parents.push_back(claim);
    const CScript env = BuildDnftEnvelope(f, Span<const uint8_t>{});
    s.addEnvelope(env);                                                        // vout 0
    s.addTokenOutput(Inscribed(Cat("71"), BindingCommit(env, s.input0(), 1))); // vout 1: child
    const auto [ok, why] = Run(s);
    BOOST_CHECK_MESSAGE(ok, "parent-present rejected: " + why);
}

// ---- reject cases ----

BOOST_AUTO_TEST_CASE(reject_mint_without_envelope) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b0")), 0));
    const CScript env = SimpleEnvelope();
    s.addTokenOutput(Inscribed(Cat("80"), BindingCommit(env, s.input0(), 0))); // inscribing, no envelope
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-missing");
}

BOOST_AUTO_TEST_CASE(reject_envelope_without_mint) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b1")), 0));
    s.addEnvelope(SimpleEnvelope());
    s.addPlainOutput();
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-envelope-unclaimed");
}

BOOST_AUTO_TEST_CASE(reject_binding_mismatch_tampered_commit) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b2")), 0));
    const CScript env = SimpleEnvelope();
    auto commit = ComputeDnftCommitment(env, s.input0(), 1);
    commit[10] ^= 0xff; // tamper one byte of the hash
    s.addEnvelope(env);
    s.addTokenOutput(Inscribed(Cat("82"), Commit(commit)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-mismatch");
}

BOOST_AUTO_TEST_CASE(reject_binding_mismatch_wrong_index) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b3")), 0));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);
    // commitment computed for index 5, but the output is at index 1
    s.addTokenOutput(Inscribed(Cat("83"), BindingCommit(env, s.input0(), 5)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-mismatch");
}

BOOST_AUTO_TEST_CASE(reject_inscribed_on_mutable) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b4")), 0));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);
    s.addTokenOutput(MutableNft(Cat("84"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-capability");
}

BOOST_AUTO_TEST_CASE(reject_inscribed_on_minting) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b5")), 0));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);
    s.addTokenOutput(MintingNft(Cat("85"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-capability");
}

BOOST_AUTO_TEST_CASE(reject_inscribed_has_amount) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b6")), 0));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);
    // capability none + 0x01 commitment + a fungible amount -> Q10 violation (FT fork is active here)
    s.addTokenOutput(InscribedWithAmount(Cat("86"), BindingCommit(env, s.input0(), 1), 5));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-inscribed-has-amount");
}

BOOST_AUTO_TEST_CASE(reject_envelope_has_token) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b7")), 0));
    // envelope output that also carries token data
    s.addEnvelope(SimpleEnvelope(), Inscribed(Cat("87"), CommitBytes({BINDING_VERSION, 1, 2})));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-envelope-has-token");
}

BOOST_AUTO_TEST_CASE(reject_invalid_envelope_parse) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b8")), 0));
    // envelope with an unknown EVEN tag (tag 4) -> invalid parse
    CScript env;
    env << OP_RETURN << std::vector<uint8_t>(MAGIC.begin(), MAGIC.end());
    env << std::vector<uint8_t>{4} << std::vector<uint8_t>{'x'} << std::vector<uint8_t>{};
    s.addEnvelope(env);
    s.addTokenOutput(Inscribed(Cat("88"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-envelope-unknown-even-tag");
}

BOOST_AUTO_TEST_CASE(reject_count_two_mints_one_envelope) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("b9")), 0));
    const CScript env = SimpleEnvelope();
    s.addEnvelope(env);                                                        // vout 0
    s.addTokenOutput(Inscribed(Cat("90"), BindingCommit(env, s.input0(), 1))); // vout 1
    s.addTokenOutput(Inscribed(Cat("91"), BindingCommit(env, s.input0(), 2))); // vout 2 (2nd mint)
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-binding-missing");
}

BOOST_AUTO_TEST_CASE(reject_parent_absent) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("ba")), 0)); // plain input; no parent spent
    std::vector<uint8_t> claim(32, 0x11);
    claim.push_back(BINDING_VERSION);
    claim.insert(claim.end(), 32, 0x22); // a 65-byte parent claim naming an item not on any input
    EnvelopeFields f;
    f.content_type = std::vector<uint8_t>{'a'};
    f.parents.push_back(claim);
    const CScript env = BuildDnftEnvelope(f, Span<const uint8_t>{});
    s.addEnvelope(env);
    s.addTokenOutput(Inscribed(Cat("a0a0"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-parent-missing");
}

BOOST_AUTO_TEST_CASE(reject_parent_not_inscribed) {
    // A capability-none NFT input exists with a NON-0x01 commitment; claiming it as a parent must
    // fail (parents must be inscribed items).
    Scene s;
    const token::Id cat = Cat("bb");
    const std::vector<uint8_t> nonInscribedCommit(33, 0x02); // 33 bytes, version 0x02 (not inscribed)
    s.addInput(COutPoint(TxId(uint256S("bb1")), 0), Inscribed(cat, CommitBytes(nonInscribedCommit)));
    std::vector<uint8_t> claim(cat.begin(), cat.end());
    claim.insert(claim.end(), nonInscribedCommit.begin(), nonInscribedCommit.end());
    EnvelopeFields f;
    f.content_type = std::vector<uint8_t>{'a'};
    f.parents.push_back(claim);
    const CScript env = BuildDnftEnvelope(f, Span<const uint8_t>{});
    s.addEnvelope(env);
    s.addTokenOutput(Inscribed(Cat("bc"), BindingCommit(env, s.input0(), 1)));
    const auto [ok, why] = Run(s);
    BOOST_CHECK(!ok);
    BOOST_CHECK_EQUAL(why, "bad-txns-dnft-parent-missing");
}

BOOST_AUTO_TEST_CASE(valid_parent_cross_category_and_shared) {
    // One parent input; two child envelopes both claim it (shared parent, dedup by membership).
    Scene s;
    const token::Id parentCat = Cat("c0");
    const auto parentCommit = BindingCommit(SimpleEnvelope(), COutPoint(TxId(uint256S("zz")), 0), 0);
    s.addInput(COutPoint(TxId(uint256S("c01")), 0), Inscribed(parentCat, parentCommit));
    std::vector<uint8_t> claim(parentCat.begin(), parentCat.end());
    claim.insert(claim.end(), parentCommit.begin(), parentCommit.end());

    EnvelopeFields f0;
    f0.content_type = std::vector<uint8_t>{'a'};
    f0.parents.push_back(claim);
    const CScript e0 = BuildDnftEnvelope(f0, Span<const uint8_t>{});
    EnvelopeFields f1;
    f1.content_type = std::vector<uint8_t>{'b'};
    f1.parents.push_back(claim); // same parent, different (cross-category) child
    const CScript e1 = BuildDnftEnvelope(f1, Span<const uint8_t>{});

    s.addEnvelope(e0);                                                          // vout 0
    s.addTokenOutput(Inscribed(Cat("c1"), BindingCommit(e0, s.input0(), 1)));   // vout 1
    s.addEnvelope(e1);                                                          // vout 2
    s.addTokenOutput(Inscribed(Cat("c2"), BindingCommit(e1, s.input0(), 3)));   // vout 3
    const auto [ok, why] = Run(s);
    BOOST_CHECK_MESSAGE(ok, "shared/cross-category parent rejected: " + why);
}

// ---- gating / exemptions ----

BOOST_AUTO_TEST_CASE(pre_du1_is_noop) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("d0")), 0));
    s.addEnvelope(SimpleEnvelope()); // would be "unclaimed" if the rules ran
    CValidationState state;
    // No SCRIPT_ENABLE_TOKENS -> CheckDnftRules is a no-op.
    BOOST_CHECK(dnft::CheckDnftRules(CTransaction(s.tx), state, s.view, SCRIPT_VERIFY_NONE, 1000,
                                     ::Params().GetConsensus()));
}

BOOST_AUTO_TEST_CASE(coinbase_is_exempt) {
    Scene s;
    s.tx.vin.emplace_back(COutPoint()); // null prevout => coinbase
    s.addEnvelope(SimpleEnvelope());    // an unclaimed envelope, but coinbase is exempt
    CValidationState state;
    BOOST_CHECK(dnft::CheckDnftRules(CTransaction(s.tx), state, s.view, SCRIPT_ENABLE_TOKENS, 1000,
                                     ::Params().GetConsensus()));
}

BOOST_AUTO_TEST_CASE(plain_tx_no_dnft) {
    Scene s;
    s.addInput(COutPoint(TxId(uint256S("d1")), 0));
    s.addPlainOutput();
    BOOST_CHECK(Run(s).first);
}

BOOST_AUTO_TEST_SUITE_END()
