// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/dnft_envelope.h>

#include <crypto/sha256.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <string>
#include <vector>

using namespace dnft;

namespace {

std::vector<uint8_t> V(std::initializer_list<uint8_t> l) { return std::vector<uint8_t>(l); }
std::vector<uint8_t> S(const std::string &s) { return std::vector<uint8_t>(s.begin(), s.end()); }
Span<const uint8_t> Sp(const std::vector<uint8_t> &v) { return Span<const uint8_t>(v.data(), v.size()); }

// A syntactically valid 65-byte parent claim (category || 0x01||32B commitment).
std::vector<uint8_t> MakeParent(uint8_t seed) {
    std::vector<uint8_t> p(PARENT_VALUE_LENGTH, seed);
    p[32] = BINDING_VERSION; // commitment version byte
    return p;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(dnft_envelope_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(is_envelope) {
    // OP_RETURN + "DNFT" push
    CScript ok;
    ok << OP_RETURN << V({'D', 'N', 'F', 'T'});
    BOOST_CHECK(IsDnftEnvelope(ok));

    // OP_RETURN + other data
    CScript notOurs;
    notOurs << OP_RETURN << S("hello");
    BOOST_CHECK(!IsDnftEnvelope(notOurs));

    // Not OP_RETURN
    CScript notOpReturn;
    notOpReturn << V({'D', 'N', 'F', 'T'});
    BOOST_CHECK(!IsDnftEnvelope(notOpReturn));

    // Empty script
    BOOST_CHECK(!IsDnftEnvelope(CScript{}));

    // OP_RETURN then wrong-length magic
    CScript shortMagic;
    shortMagic << OP_RETURN << V({'D', 'N', 'F'});
    BOOST_CHECK(!IsDnftEnvelope(shortMagic));
}

BOOST_AUTO_TEST_CASE(roundtrip_full) {
    EnvelopeFields f;
    f.content_type = S("image/png");
    f.parents.push_back(MakeParent(0x11));
    f.parents.push_back(MakeParent(0x22));
    f.metadata = V({0xa1, 0x01, 0x02}); // some CBOR-ish bytes
    f.metaprotocol = S("dnft-sub");
    f.content_encoding = S("br");
    f.delegate = std::vector<uint8_t>(36, 0x7e); // txid(32)+LE index(4)
    const std::vector<uint8_t> body = S("the quick brown fox");

    const CScript spk = BuildDnftEnvelope(f, Sp(body));
    BOOST_CHECK(IsDnftEnvelope(spk));

    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK_MESSAGE(p.valid, "parse error: " + p.error);
    BOOST_CHECK(p.content_type && *p.content_type == S("image/png"));
    BOOST_REQUIRE_EQUAL(p.parents.size(), 2U);
    BOOST_CHECK(p.parents[0] == MakeParent(0x11));
    BOOST_CHECK(p.parents[1] == MakeParent(0x22));
    BOOST_CHECK(p.metadata && *p.metadata == V({0xa1, 0x01, 0x02}));
    BOOST_CHECK(p.metaprotocol && *p.metaprotocol == S("dnft-sub"));
    BOOST_CHECK(p.content_encoding && *p.content_encoding == S("br"));
    BOOST_CHECK(p.delegate && p.delegate->size() == 36U);
    BOOST_CHECK(p.has_body && p.body == body);
}

BOOST_AUTO_TEST_CASE(body_presence_variants) {
    EnvelopeFields f;
    f.content_type = S("text/plain");

    // include_body=true, empty body -> has_body true, body empty
    {
        const ParsedEnvelope p = ParseDnftEnvelope(BuildDnftEnvelope(f, Span<const uint8_t>{}, true));
        BOOST_CHECK(p.valid && p.has_body && p.body.empty());
    }
    // include_body=false -> no separator, has_body false
    {
        const ParsedEnvelope p = ParseDnftEnvelope(BuildDnftEnvelope(f, Span<const uint8_t>{}, false));
        BOOST_CHECK(p.valid && !p.has_body);
    }
    // delegate-only, body-less
    {
        EnvelopeFields d;
        d.delegate = std::vector<uint8_t>(36, 0x01);
        const ParsedEnvelope p = ParseDnftEnvelope(BuildDnftEnvelope(d, Span<const uint8_t>{}, false));
        BOOST_CHECK(p.valid && !p.has_body && p.delegate);
    }
    // Empty envelope (magic only): valid, no fields, no body.
    {
        CScript spk;
        spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
        const ParsedEnvelope p = ParseDnftEnvelope(spk);
        BOOST_CHECK(p.valid && !p.has_body && !p.content_type && p.parents.empty());
    }
}

BOOST_AUTO_TEST_CASE(large_body_single_push) {
    EnvelopeFields f;
    f.content_type = S("application/octet-stream");
    std::vector<uint8_t> body(200000);
    for (size_t i = 0; i < body.size(); ++i) body[i] = uint8_t(i * 7 + 3);
    const CScript spk = BuildDnftEnvelope(f, Sp(body));
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(p.valid);
    BOOST_CHECK(p.has_body && p.body == body);
}

BOOST_AUTO_TEST_CASE(metadata_concatenation) {
    // Two metadata pushes should concatenate (ord "chunkable" semantics).
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_METADATA}) << V({0x01, 0x02});
    spk << V({TAG_METADATA}) << V({0x03, 0x04});
    spk << V({}); // body separator
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(p.valid);
    BOOST_CHECK(p.metadata && *p.metadata == V({0x01, 0x02, 0x03, 0x04}));
}

BOOST_AUTO_TEST_CASE(unknown_odd_tag_ignored) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({13}) << S("future-odd-tag-value"); // tag 13 unknown, odd -> ignored
    spk << V({TAG_CONTENT_TYPE}) << S("text/plain");
    spk << V({}) << S("body");
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK_MESSAGE(p.valid, "parse error: " + p.error);
    BOOST_CHECK(p.content_type && *p.content_type == S("text/plain"));
}

BOOST_AUTO_TEST_CASE(empty_field_value_not_separator) {
    // An empty push at an ODD index is a (valid) empty field value, not the body separator.
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_CONTENT_TYPE}) << V({}); // content_type = empty (odd-index empty push)
    spk << V({TAG_METAPROTOCOL}) << S("mp");
    spk << V({}) << S("body");
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK_MESSAGE(p.valid, "parse error: " + p.error);
    BOOST_CHECK(p.content_type && p.content_type->empty());
    BOOST_CHECK(p.metaprotocol && *p.metaprotocol == S("mp"));
    BOOST_CHECK(p.has_body && p.body == S("body"));
}

BOOST_AUTO_TEST_CASE(invalid_incomplete_field) {
    // A trailing tag with no value (odd field count).
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_CONTENT_TYPE}); // no value, no separator
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-incomplete-field");
}

BOOST_AUTO_TEST_CASE(invalid_duplicate_field) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_CONTENT_TYPE}) << S("a");
    spk << V({TAG_CONTENT_TYPE}) << S("b"); // duplicate content_type
    spk << V({});
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-duplicate-field");
}

// 4I review F3: the other single-shot fields take the identical duplicate-reject path as
// content_type but were previously untested. metaprotocol/content_encoding/delegate all reject;
// metadata is deliberately chunkable and must NOT (it concatenates — covered separately).
BOOST_AUTO_TEST_CASE(invalid_duplicate_field_all_singleshot) {
    for (uint8_t tag : {TAG_METAPROTOCOL, TAG_CONTENT_ENCODING, TAG_DELEGATE}) {
        CScript spk;
        spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
        spk << V({tag}) << S("x");
        spk << V({tag}) << S("y");
        spk << V({});
        const ParsedEnvelope p = ParseDnftEnvelope(spk);
        BOOST_CHECK_MESSAGE(!p.valid, "duplicate tag " << int(tag) << " must be invalid");
        BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-duplicate-field");
    }
    // Metadata repeated is VALID (chunkable): the two chunks concatenate.
    CScript md;
    md << OP_RETURN << V({'D', 'N', 'F', 'T'});
    md << V({TAG_METADATA}) << S("ab");
    md << V({TAG_METADATA}) << S("cd");
    md << V({});
    const ParsedEnvelope p = ParseDnftEnvelope(md);
    BOOST_CHECK(p.valid);
    BOOST_REQUIRE(p.metadata.has_value());
    BOOST_CHECK_EQUAL(HexStr(*p.metadata), HexStr(S("abcd")));
}

// 4I review F2: the PUSHDATA2/PUSHDATA4 length-header bounds checks + oversize guard were
// exercised only via a 1-byte direct push. Feed truncated PUSHDATA1/2/4 headers and an oversize
// PUSHDATA4 claim; all must reject as -malformed and never over-read.
BOOST_AUTO_TEST_CASE(invalid_truncated_pushdata_headers) {
    // PUSHDATA1 with no length byte.
    {
        CScript spk;
        spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
        spk.push_back(OP_PUSHDATA1); // then EOF
        const ParsedEnvelope p = ParseDnftEnvelope(spk);
        BOOST_CHECK(!p.valid);
        BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
    }
    // PUSHDATA2 with a 1-byte (truncated) length header.
    {
        CScript spk;
        spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
        spk.push_back(OP_PUSHDATA2);
        spk.push_back(0x10); // only 1 of 2 length bytes
        const ParsedEnvelope p = ParseDnftEnvelope(spk);
        BOOST_CHECK(!p.valid);
        BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
    }
    // PUSHDATA4 claiming ~2 GB with nothing following — the oversize guard must fire, no over-read.
    {
        CScript spk;
        spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
        spk.push_back(OP_PUSHDATA4);
        spk.push_back(0xff);
        spk.push_back(0xff);
        spk.push_back(0xff);
        spk.push_back(0x7f); // len = 0x7fffffff
        const ParsedEnvelope p = ParseDnftEnvelope(spk);
        BOOST_CHECK(!p.valid);
        BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
    }
    // Well-formed PUSHDATA1 (a real 200-byte body via the builder) still parses — proves the
    // header path isn't over-eager.
    {
        EnvelopeFields f;
        f.content_type = S("text/plain");
        const std::vector<uint8_t> body(200, 0x5a);
        const ParsedEnvelope p = ParseDnftEnvelope(BuildDnftEnvelope(f, Sp(body)));
        BOOST_CHECK(p.valid);
        BOOST_CHECK_EQUAL(p.body.size(), 200u);
    }
}

BOOST_AUTO_TEST_CASE(invalid_unknown_even_tag) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({4}) << S("mandatory-unknown"); // tag 4 unknown, even -> invalid
    spk << V({});
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-unknown-even-tag");
}

BOOST_AUTO_TEST_CASE(invalid_bad_parent_length) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_PARENT}) << std::vector<uint8_t>(64, 0xaa); // 64 != 65
    spk << V({});
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-bad-parent");
}

BOOST_AUTO_TEST_CASE(invalid_non_push_opcode) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << OP_DUP; // a real opcode inside the envelope
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
}

BOOST_AUTO_TEST_CASE(invalid_truncated_push) {
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk.push_back(0x05);       // claims a 5-byte push...
    spk.push_back(0xaa);       // ...but only 2 bytes follow
    spk.push_back(0xbb);
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
}

BOOST_AUTO_TEST_CASE(pushnum_opcode_rejected) {
    // OP_1 (a pushnum) is not a canonical data push in the envelope grammar.
    CScript spk;
    spk << OP_RETURN << V({'D', 'N', 'F', 'T'});
    spk << V({TAG_CONTENT_TYPE});
    spk << OP_1; // where a value push was expected
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "bad-txns-dnft-envelope-malformed");
}

BOOST_AUTO_TEST_CASE(not_an_envelope_parse) {
    CScript spk;
    spk << OP_RETURN << S("just an op_return");
    const ParsedEnvelope p = ParseDnftEnvelope(spk);
    BOOST_CHECK(!p.valid);
    BOOST_CHECK_EQUAL(p.error, "not-a-dnft-envelope");
}

BOOST_AUTO_TEST_CASE(validate_fast_path_matches_parse) {
    EnvelopeFields f;
    f.content_type = S("text/plain");
    f.parents.push_back(MakeParent(0xab));
    f.parents.push_back(MakeParent(0xcd));
    const CScript spk = BuildDnftEnvelope(f, Sp(S("body")));

    std::vector<std::array<uint8_t, PARENT_VALUE_LENGTH>> parents;
    std::string err;
    BOOST_CHECK(ValidateDnftEnvelope(spk, &parents, &err));
    BOOST_REQUIRE_EQUAL(parents.size(), 2U);
    BOOST_CHECK(std::vector<uint8_t>(parents[0].begin(), parents[0].end()) == MakeParent(0xab));
    BOOST_CHECK(std::vector<uint8_t>(parents[1].begin(), parents[1].end()) == MakeParent(0xcd));

    // An invalid envelope must fail the fast path with the same reason as the full parse.
    CScript bad;
    bad << OP_RETURN << V({'D', 'N', 'F', 'T'});
    bad << V({4}) << S("x") << V({});
    BOOST_CHECK(!ValidateDnftEnvelope(bad, nullptr, &err));
    BOOST_CHECK_EQUAL(err, "bad-txns-dnft-envelope-unknown-even-tag");
}

BOOST_AUTO_TEST_CASE(commitment_vector_and_properties) {
    EnvelopeFields f;
    f.content_type = S("image/png");
    const std::vector<uint8_t> body = S("pixels");
    const CScript spk = BuildDnftEnvelope(f, Sp(body));

    const COutPoint prevout(TxId(uint256S("abcdef00")), 3);
    const uint32_t voutIdx = 1;
    const auto commitment = ComputeDnftCommitment(spk, prevout, voutIdx);

    // Independent recomputation: 0x01 || SHA256(spk || outpoint(36) || le32(idx)).
    // Cross-check the 36-byte outpoint form against the consensus COutPoint serialization.
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << prevout;
    BOOST_REQUIRE_EQUAL(ss.size(), 36U);

    std::vector<uint8_t> preimage(spk.begin(), spk.end());
    preimage.insert(preimage.end(), ss.begin(), ss.end());
    uint8_t idx[4] = {uint8_t(voutIdx), uint8_t(voutIdx >> 8), uint8_t(voutIdx >> 16), uint8_t(voutIdx >> 24)};
    preimage.insert(preimage.end(), idx, idx + 4);
    uint8_t digest[32];
    CSHA256().Write(preimage.data(), preimage.size()).Finalize(digest);

    BOOST_CHECK_EQUAL(commitment[0], BINDING_VERSION);
    BOOST_CHECK(std::equal(digest, digest + 32, commitment.begin() + 1));
    BOOST_CHECK_EQUAL(commitment.size(), COMMITMENT_LENGTH);

    // Uniqueness properties (Q1 salting): different vout index or different input0 -> different
    // commitment, even for byte-identical envelope content.
    BOOST_CHECK(ComputeDnftCommitment(spk, prevout, 2) != commitment);
    BOOST_CHECK(ComputeDnftCommitment(spk, COutPoint(TxId(uint256S("abcdef00")), 4), voutIdx) != commitment);
    BOOST_CHECK(ComputeDnftCommitment(spk, COutPoint(TxId(uint256S("abcdef01")), 3), voutIdx) != commitment);
    // Same inputs -> deterministic.
    BOOST_CHECK(ComputeDnftCommitment(spk, prevout, voutIdx) == commitment);
}

// Golden vectors shared with the independent Python reference
// (test/functional/dvt_dnft_reference.py). Pinning both implementations to the same bytes makes a
// silent divergence between the C++ and Python codecs impossible to miss.
BOOST_AUTO_TEST_CASE(golden_cross_check_python) {
    EnvelopeFields f;
    f.content_type = S("image/png");
    const CScript spk = BuildDnftEnvelope(f, Sp(S("pixels")));
    BOOST_CHECK_EQUAL(HexStr(spk), "6a04444e4654010109696d6167652f706e670006706978656c73");

    std::vector<uint8_t> txidBytes(32);
    for (uint8_t i = 0; i < 32; ++i) txidBytes[i] = uint8_t(i + 1); // internal bytes 0x01..0x20
    const COutPoint prevout(TxId(uint256(txidBytes)), 3);
    const auto commitment = ComputeDnftCommitment(spk, prevout, 1);
    BOOST_CHECK_EQUAL(HexStr(commitment),
                      "01bb210f24936615ea89fcf620eae571e62598312cd4dc13ef2c332aea523925eb");
}

BOOST_AUTO_TEST_SUITE_END()
