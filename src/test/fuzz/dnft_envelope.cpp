// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/dnft_envelope.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <span.h>
#include <uint256.h>

#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <vector>

// The DNFT envelope parser is attacker-facing consensus input (it runs in ConnectBlock/ATMP on
// every DNFT mint). This target asserts it never crashes/OOBs on arbitrary bytes and that its
// invariants hold: the fast-path validator agrees with the full parse, and a valid parse
// round-trips through the builder without loss.

static void ExerciseScript(const CScript &script) {
    const dnft::ParsedEnvelope p = dnft::ParseDnftEnvelope(script);

    // Fast-path validator must agree with the full parse on validity, and on parents when valid.
    std::vector<std::array<uint8_t, dnft::PARENT_VALUE_LENGTH>> parents;
    std::string err;
    const bool fastValid = dnft::ValidateDnftEnvelope(script, &parents, &err);
    assert(fastValid == p.valid);
    if (p.valid) {
        assert(parents.size() == p.parents.size());
        for (size_t i = 0; i < parents.size(); ++i) {
            assert(std::equal(parents[i].begin(), parents[i].end(), p.parents[i].begin()));
        }
    }

    // IsDnftEnvelope is a strict prefix of a successful parse: any valid parse is an envelope.
    if (p.valid) {
        assert(dnft::IsDnftEnvelope(script));

        // Round-trip: rebuild from the parsed fields+body and re-parse -> identical logical result.
        dnft::EnvelopeFields f;
        f.content_type = p.content_type;
        f.parents = p.parents;
        f.metadata = p.metadata;
        f.metaprotocol = p.metaprotocol;
        f.content_encoding = p.content_encoding;
        f.delegate = p.delegate;
        const CScript rebuilt =
            dnft::BuildDnftEnvelope(f, Span<const uint8_t>(p.body.data(), p.body.size()), p.has_body);
        const dnft::ParsedEnvelope p2 = dnft::ParseDnftEnvelope(rebuilt);
        assert(p2.valid);
        assert(p2.content_type == p.content_type);
        assert(p2.parents == p.parents);
        assert(p2.metadata == p.metadata);
        assert(p2.metaprotocol == p.metaprotocol);
        assert(p2.content_encoding == p.content_encoding);
        assert(p2.delegate == p.delegate);
        assert(p2.has_body == p.has_body);
        assert(p2.body == p.body);

        // The commitment hash must be total (no throw/crash) for any envelope scriptPubKey.
        (void)dnft::ComputeDnftCommitment(script, COutPoint(TxId(uint256S("01")), 0), 0);
    }
}

void test_one_input(Span<const uint8_t> buffer) {
    // (a) the raw bytes as a scriptPubKey
    ExerciseScript(CScript(buffer.begin(), buffer.end()));

    // (b) a well-formed OP_RETURN+magic prefix followed by the fuzz bytes, to drive the field
    //     parser deeper (past the magic gate) more often.
    CScript env;
    env << OP_RETURN << std::vector<uint8_t>(dnft::MAGIC.begin(), dnft::MAGIC.end());
    env.insert(env.end(), buffer.begin(), buffer.end());
    ExerciseScript(env);
}
