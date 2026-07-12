// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Phase 5B of DEVAULT_FT_SPEC.md: the DVFT envelope codec (deploy + mint markers, spec §8).
// Golden vectors are shared with the independent Python reference
// (test/functional/dvt_ft_reference.py) — both implementations pin the same bytes, derived
// independently from the wire rules (the 4B discipline).

#include <devault/dnft_envelope.h>
#include <devault/ft_envelope.h>

#include <script/script.h>
#include <util/strencodings.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

using namespace dnft;

namespace {

std::vector<uint8_t> S(const std::string &s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

CScript FromHex(const std::string &hex) {
    const std::vector<uint8_t> b = ParseHex(hex);
    return CScript(b.begin(), b.end());
}

//! OP_RETURN "DVFT" header as hex.
const std::string HDR = "6a0444564654";

//! A canonical valid OPEN deploy (the golden vector's params).
FtDeployParams GoldenOpenParams() {
    FtDeployParams p;
    p.symbol = S("GOLD");
    p.name = S("Gold Token");
    p.decimals = 2;
    p.mode = FT_MODE_OPEN;
    p.quantity_per_mint = 100;
    p.per_block_limit = 5;
    p.start_height = 300;
    p.max_mints = 1000;
    p.premine = 500;
    std::array<uint8_t, FT_METADATA_POINTER_LENGTH> m;
    m.fill(0xAB);
    p.metadata = m;
    return p;
}

const std::string GOLDEN_DEPLOY_HEX =
    "6a0444564654010104474f4c4401030a476f6c6420546f6b656e0105010201070101010908640000000000000001"
    "0b080500000000000000010d042c010000010f08e803000000000000011308f4010000000000000115"
    "24abababababababababababababababababababababababababababababababababababab";

const std::string GOLDEN_MINT_HEX =
    "6a04445646540117200102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20";

const std::string GOLDEN_FIXED_HEX =
    "6a0444564654010103555344010309555320446f6c6c61720105010801070100";

std::string ExpectReject(const std::string &hex) {
    const ParsedFtEnvelope p = ParseFtEnvelope(FromHex(hex));
    BOOST_CHECK(!p.valid);
    return p.error;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(ft_envelope_tests, BasicTestingSetup)

// ---------------------------------------------------------------- goldens + round trips

BOOST_AUTO_TEST_CASE(golden_open_deploy) {
    const CScript spk = BuildFtDeployEnvelope(GoldenOpenParams());
    BOOST_CHECK_EQUAL(HexStr(spk), GOLDEN_DEPLOY_HEX);

    BOOST_CHECK(IsFtEnvelope(spk));
    const ParsedFtEnvelope p = ParseFtEnvelope(spk);
    BOOST_REQUIRE_MESSAGE(p.valid, p.error);
    BOOST_CHECK(p.is_deploy && !p.is_mint);
    BOOST_CHECK(p.symbol == S("GOLD"));
    BOOST_CHECK(p.name == S("Gold Token"));
    BOOST_CHECK_EQUAL(p.decimals, 2);
    BOOST_CHECK_EQUAL(p.mode, FT_MODE_OPEN);
    BOOST_CHECK_EQUAL(p.quantity_per_mint, 100u);
    BOOST_CHECK_EQUAL(p.per_block_limit, 5u);
    BOOST_CHECK_EQUAL(p.start_height, 300u);
    BOOST_REQUIRE(p.max_mints.has_value());
    BOOST_CHECK_EQUAL(*p.max_mints, 1000u);
    BOOST_CHECK(!p.end_height.has_value());
    BOOST_CHECK_EQUAL(p.premine, 500u);
    BOOST_REQUIRE(p.metadata.has_value());
    BOOST_CHECK(std::all_of(p.metadata->begin(), p.metadata->end(),
                            [](uint8_t b) { return b == 0xAB; }));
}

BOOST_AUTO_TEST_CASE(golden_mint) {
    std::array<uint8_t, FT_DEPLOY_TXID_LENGTH> txid;
    for (size_t i = 0; i < txid.size(); ++i) {
        txid[i] = uint8_t(i + 1); // internal bytes 0x01..0x20
    }
    const CScript spk = BuildFtMintEnvelope(txid);
    BOOST_CHECK_EQUAL(HexStr(spk), GOLDEN_MINT_HEX);

    BOOST_CHECK(IsFtEnvelope(spk));
    const ParsedFtEnvelope p = ParseFtEnvelope(spk);
    BOOST_REQUIRE_MESSAGE(p.valid, p.error);
    BOOST_CHECK(p.is_mint && !p.is_deploy);
    BOOST_CHECK(p.mint_deploy_txid == txid);
}

BOOST_AUTO_TEST_CASE(golden_fixed_deploy) {
    FtDeployParams p;
    p.symbol = S("USD");
    p.name = S("US Dollar");
    p.decimals = 8;
    p.mode = FT_MODE_FIXED;
    const CScript spk = BuildFtDeployEnvelope(p);
    BOOST_CHECK_EQUAL(HexStr(spk), GOLDEN_FIXED_HEX);

    const ParsedFtEnvelope r = ParseFtEnvelope(spk);
    BOOST_REQUIRE_MESSAGE(r.valid, r.error);
    BOOST_CHECK(r.is_deploy);
    BOOST_CHECK_EQUAL(r.mode, FT_MODE_FIXED);
    BOOST_CHECK_EQUAL(r.premine, 0u);
    BOOST_CHECK(!r.metadata.has_value());
}

BOOST_AUTO_TEST_CASE(round_trip_end_height_variant) {
    FtDeployParams p = GoldenOpenParams();
    p.max_mints.reset();
    p.end_height = 500;
    p.premine = 0; // absent
    p.metadata.reset();
    const ParsedFtEnvelope r = ParseFtEnvelope(BuildFtDeployEnvelope(p));
    BOOST_REQUIRE_MESSAGE(r.valid, r.error);
    BOOST_CHECK(!r.max_mints.has_value());
    BOOST_REQUIRE(r.end_height.has_value());
    BOOST_CHECK_EQUAL(*r.end_height, 500u);
    BOOST_CHECK_EQUAL(r.premine, 0u);
}

// The spec §8 claim: a worst-case deploy envelope relays under the plain-OP_RETURN 223-byte
// datacarrier allowance — no policy change is needed for DVFT markers.
BOOST_AUTO_TEST_CASE(worst_case_size_under_datacarrier) {
    FtDeployParams p;
    p.symbol = std::vector<uint8_t>(FT_MAX_SYMBOL_BYTES, 'S');
    p.name = std::vector<uint8_t>(FT_MAX_NAME_BYTES, 'N');
    p.decimals = FT_MAX_DECIMALS;
    p.mode = FT_MODE_OPEN;
    p.quantity_per_mint = ~0ull;
    p.per_block_limit = ~0ull;
    p.start_height = ~0u;
    p.max_mints = ~0ull;
    p.premine = ~0ull;
    std::array<uint8_t, FT_METADATA_POINTER_LENGTH> m;
    m.fill(0xAB);
    p.metadata = m;
    const CScript spk = BuildFtDeployEnvelope(p);
    BOOST_CHECK(ParseFtEnvelope(spk).valid);
    BOOST_CHECK_LE(spk.size(), 223u);
    BOOST_CHECK_EQUAL(spk.size(), 190u); // pinned: recompute if the tag set ever changes
}

// ---------------------------------------------------------------- detectors + protocol isolation

BOOST_AUTO_TEST_CASE(detector_and_no_dnft_alias) {
    // Not OP_RETURN / wrong magic / empty.
    BOOST_CHECK(!IsFtEnvelope(CScript() << OP_TRUE));
    BOOST_CHECK(!IsFtEnvelope(CScript()));
    BOOST_CHECK_EQUAL(ExpectReject("51"), "not-a-dvft-envelope");
    BOOST_CHECK_EQUAL(ExpectReject("6a0444564658"), "not-a-dvft-envelope"); // "DVFX"

    // Cross-protocol isolation: a DNFT envelope is not a DVFT envelope, and vice versa.
    const CScript dnftSpk =
        FromHex("6a04444e4654010109696d6167652f706e670006706978656c73"); // the 4B golden
    BOOST_CHECK(IsDnftEnvelope(dnftSpk));
    BOOST_CHECK(!IsFtEnvelope(dnftSpk));
    BOOST_CHECK_EQUAL(ParseFtEnvelope(dnftSpk).error, "not-a-dvft-envelope");

    const CScript dvftSpk = FromHex(GOLDEN_MINT_HEX);
    BOOST_CHECK(!IsDnftEnvelope(dvftSpk));
    BOOST_CHECK(!ParseDnftEnvelope(dvftSpk).valid);
}

// ---------------------------------------------------------------- structural reject matrix

BOOST_AUTO_TEST_CASE(reject_structure) {
    // Non-push opcode inside the envelope.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "51"), "bad-txns-ft-envelope-malformed");
    // Truncated push.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "05aa"), "bad-txns-ft-envelope-malformed");
    // Odd number of field pushes (a trailing tag with no value).
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101"), "bad-txns-ft-envelope-incomplete-field");
    // A body separator (empty push at an even element index) — DVFT envelopes are body-less.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "00"), "bad-txns-ft-envelope-has-body");
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "010101610002aabb"), "bad-txns-ft-envelope-has-body");
    // Unknown EVEN tag (binding).
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "01020141"), "bad-txns-ft-envelope-unknown-even-tag");
    // Multi-byte tag with an even first byte: also binding.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0202000141"), "bad-txns-ft-envelope-unknown-even-tag");
    // Unknown ODD tag: ignorable — but the envelope must still resolve a role, and it cannot.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "01630141"), "bad-txns-ft-envelope-role");
    // Empty envelope (magic only): no role.
    BOOST_CHECK_EQUAL(ExpectReject(HDR), "bad-txns-ft-envelope-role");
    // Duplicate field.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101016101010161"), "bad-txns-ft-envelope-duplicate-field");
}

// Unknown ODD tags are ignorable on an otherwise-valid envelope (forward compatibility).
BOOST_AUTO_TEST_CASE(unknown_odd_tag_ignored) {
    // Append tag 0x63 (odd, unknown) to the golden mint.
    const ParsedFtEnvelope p = ParseFtEnvelope(FromHex(GOLDEN_MINT_HEX + "016301aa"));
    BOOST_REQUIRE_MESSAGE(p.valid, p.error);
    BOOST_CHECK(p.is_mint);
    // NOP (255) is ignorable and repeatable.
    const ParsedFtEnvelope q = ParseFtEnvelope(FromHex(GOLDEN_MINT_HEX + "01ff014101ff0142"));
    BOOST_REQUIRE_MESSAGE(q.valid, q.error);
}

// ---------------------------------------------------------------- field bounds

BOOST_AUTO_TEST_CASE(reject_field_bounds) {
    auto deployHex = [](const std::string &fields) { return HDR + fields; };
    const std::string nameDecMode = "01030161" "01050100" "01070100"; // name "a", dec 0, fixed

    // Symbol: EMPTY value. Note the empty push sits at an ODD element index (it is a value, not
    // a potential body separator, which only lives at even indices) -> the bound check fires.
    BOOST_CHECK_EQUAL(ExpectReject(deployHex("010100" + nameDecMode)),
                      "bad-txns-ft-envelope-bad-symbol");
    // Symbol: 17 bytes (0x11 push of 17 x 0x66) -> over the 16-byte cap.
    BOOST_CHECK_EQUAL(ExpectReject(deployHex("0101" "11" + std::string(34, '6'))),
                      "bad-txns-ft-envelope-bad-symbol");
    // Name: 65 bytes (0x41 push) -> over the 64-byte cap.
    BOOST_CHECK_EQUAL(ExpectReject(deployHex("01010161" "0103" "41" + std::string(130, '6'))),
                      "bad-txns-ft-envelope-bad-name");

    // Decimals: 9 rejected, wrong width rejected.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101016101030161" "01050109" "01070100"),
                      "bad-txns-ft-envelope-bad-decimals");
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101016101030161" "0105020000" "01070100"),
                      "bad-txns-ft-envelope-bad-decimals");

    // Mode: 2 rejected.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101016101030161" "01050100" "01070102"),
                      "bad-txns-ft-envelope-bad-mode");

    // Mint txid: wrong length.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "011704deadbeef"), "bad-txns-ft-envelope-bad-mint");

    // Metadata: wrong length (35).
    FtDeployParams p;
    p.symbol = S("A");
    p.name = S("B");
    p.decimals = 0;
    p.mode = FT_MODE_FIXED;
    CScript base = BuildFtDeployEnvelope(p);
    CScript bad = base;
    bad << std::vector<uint8_t>{FT_TAG_METADATA} << std::vector<uint8_t>(35, 0xAB);
    BOOST_CHECK_EQUAL(ParseFtEnvelope(bad).error, "bad-txns-ft-envelope-bad-metadata");
}

// Integer strictness: zero values and wrong widths for the schedule fields.
BOOST_AUTO_TEST_CASE(reject_integer_strictness) {
    auto openWith = [](const std::string &qty, const std::string &m, const std::string &s,
                       const std::string &n) {
        return HDR + std::string("0101016101030161" "01050100" "01070101") + qty + m + s + n;
    };
    const std::string Q = "010908" + std::string("0100000000000000");
    const std::string M = "010b08" + std::string("0100000000000000");
    const std::string SH = "010d04" + std::string("2c010000");
    const std::string N = "010f08" + std::string("0100000000000000");

    // Baseline is valid.
    {
        const ParsedFtEnvelope p = ParseFtEnvelope(FromHex(openWith(Q, M, SH, N)));
        BOOST_REQUIRE_MESSAGE(p.valid, p.error);
    }
    // quantity = 0.
    BOOST_CHECK_EQUAL(ExpectReject(openWith("010908" "0000000000000000", M, SH, N)),
                      "bad-txns-ft-envelope-bad-quantity");
    // quantity wrong width (4 bytes).
    BOOST_CHECK_EQUAL(ExpectReject(openWith("010904" "01000000", M, SH, N)),
                      "bad-txns-ft-envelope-bad-quantity");
    // per_block_limit = 0 (O1: required >= 1).
    BOOST_CHECK_EQUAL(ExpectReject(openWith(Q, "010b08" "0000000000000000", SH, N)),
                      "bad-txns-ft-envelope-bad-per-block-limit");
    // start wrong width (8 bytes).
    BOOST_CHECK_EQUAL(ExpectReject(openWith(Q, M, "010d08" "2c01000000000000", N)),
                      "bad-txns-ft-envelope-bad-start-height");
    // max_mints = 0.
    BOOST_CHECK_EQUAL(ExpectReject(openWith(Q, M, SH, "010f08" "0000000000000000")),
                      "bad-txns-ft-envelope-bad-max-mints");
}

// ---------------------------------------------------------------- roles + schedule

BOOST_AUTO_TEST_CASE(reject_roles_and_schedule) {
    // Mixed role: a mint marker carrying a deploy field (symbol "a").
    BOOST_CHECK_EQUAL(ExpectReject(GOLDEN_MINT_HEX + "01010161"),
                      "bad-txns-ft-envelope-role");
    // Mode + mint together.
    BOOST_CHECK_EQUAL(
        ExpectReject(HDR + "01070100" + std::string("011720") +
                     std::string(64, '0')),
        "bad-txns-ft-envelope-role");

    // Deploy missing required fields (no symbol).
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "01030161" "01050100" "01070100"),
                      "bad-txns-ft-envelope-missing-field");
    // Open deploy missing the schedule entirely.
    BOOST_CHECK_EQUAL(ExpectReject(HDR + "0101016101030161" "01050100" "01070101"),
                      "bad-txns-ft-envelope-missing-field");

    FtDeployParams open = GoldenOpenParams();
    open.metadata.reset();
    open.premine = 0;

    // Both max_mints and end_height.
    {
        CScript spk = BuildFtDeployEnvelope(open);
        spk << std::vector<uint8_t>{FT_TAG_END_HEIGHT} << std::vector<uint8_t>{0x90, 0x01, 0, 0};
        BOOST_CHECK_EQUAL(ParseFtEnvelope(spk).error, "bad-txns-ft-envelope-schedule");
    }
    // Neither (strip max_mints by building with end_height then removing... build directly):
    BOOST_CHECK_EQUAL(
        ExpectReject(HDR + "0101016101030161" "01050100" "01070101"
                     "010908" "0100000000000000" "010b08" "0100000000000000"
                     "010d04" "2c010000"),
        "bad-txns-ft-envelope-schedule");
    // end < start.
    {
        FtDeployParams p = open;
        p.max_mints.reset();
        p.end_height = p.start_height - 1;
        BOOST_CHECK_EQUAL(ParseFtEnvelope(BuildFtDeployEnvelope(p)).error,
                          "bad-txns-ft-envelope-schedule");
    }
    // Open-only fields on a fixed deploy (premine).
    {
        FtDeployParams p;
        p.symbol = S("A");
        p.name = S("B");
        p.decimals = 0;
        p.mode = FT_MODE_FIXED;
        CScript spk = BuildFtDeployEnvelope(p);
        spk << std::vector<uint8_t>{FT_TAG_PREMINE}
            << std::vector<uint8_t>{1, 0, 0, 0, 0, 0, 0, 0};
        BOOST_CHECK_EQUAL(ParseFtEnvelope(spk).error, "bad-txns-ft-envelope-schedule");
    }
}

BOOST_AUTO_TEST_SUITE_END()
