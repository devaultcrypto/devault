// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/dnft_envelope.h>

#include <crypto/common.h>
#include <crypto/sha256.h>

#include <cstring>

namespace dnft {

namespace {

// Read the next script element starting at `pc` (< `end`).
//   returns false  -> end reached, or a truncated/oversized push (malformed script)
//   returns true, isPush=true,  data=<span of pushed bytes>  -> a canonical data push (OP_0..OP_PUSHDATA4)
//   returns true, isPush=false                               -> some other opcode (not a data push)
// Bounds logic mirrors GetScriptOp() exactly but yields a zero-copy Span instead of copying.
bool NextElement(const uint8_t *&pc, const uint8_t *end, bool &isPush, Span<const uint8_t> &data) {
    if (pc >= end) {
        return false;
    }
    const uint32_t opcode = *pc++;
    if (opcode <= OP_PUSHDATA4) {
        uint32_t nSize = 0;
        if (opcode < OP_PUSHDATA1) {
            nSize = opcode;
        } else if (opcode == OP_PUSHDATA1) {
            if (end - pc < 1) return false;
            nSize = *pc++;
        } else if (opcode == OP_PUSHDATA2) {
            if (end - pc < 2) return false;
            nSize = ReadLE16(pc);
            pc += 2;
        } else { // OP_PUSHDATA4
            if (end - pc < 4) return false;
            nSize = ReadLE32(pc);
            pc += 4;
        }
        if (uint32_t(end - pc) < nSize) return false;
        data = Span<const uint8_t>(pc, size_t(nSize));
        pc += nSize;
        isPush = true;
        return true;
    }
    isPush = false;
    return true;
}

// Internal span-based parse result (zero-copy views into the input script).
struct SpanEnvelope {
    bool is_envelope = false; // OP_RETURN + DNFT magic present
    bool valid = false;
    std::string error;

    std::optional<Span<const uint8_t>> content_type;
    std::vector<Span<const uint8_t>> parents;
    std::vector<Span<const uint8_t>> metadata_chunks;
    std::optional<Span<const uint8_t>> metaprotocol;
    std::optional<Span<const uint8_t>> content_encoding;
    std::optional<Span<const uint8_t>> delegate;

    bool has_body = false;
    std::vector<Span<const uint8_t>> body_chunks;
};

SpanEnvelope ParseSpans(const CScript &spk) {
    SpanEnvelope r;
    const uint8_t *pc = spk.data();
    const uint8_t *const end = pc + spk.size();

    // OP_RETURN prefix
    if (pc >= end || *pc != OP_RETURN) {
        return r; // not ours (is_envelope=false)
    }
    ++pc;

    // Magic push == "DNFT"
    bool isPush = false;
    Span<const uint8_t> data;
    if (!NextElement(pc, end, isPush, data) || !isPush || data.size() != MAGIC.size() ||
        std::memcmp(data.data(), MAGIC.data(), MAGIC.size()) != 0) {
        return r; // OP_RETURN but not a DNFT envelope
    }
    r.is_envelope = true;

    // Collect the remaining pushes (zero-copy). Any non-push opcode, or a truncated push before
    // end, makes the envelope malformed.
    std::vector<Span<const uint8_t>> pushes;
    while (pc < end) {
        if (!NextElement(pc, end, isPush, data)) {
            r.error = "bad-txns-dnft-envelope-malformed"; // truncated push
            return r;
        }
        if (!isPush) {
            r.error = "bad-txns-dnft-envelope-malformed"; // non-push opcode inside the envelope
            return r;
        }
        pushes.push_back(data);
    }

    // Body separator = first EVEN-indexed empty push (ord model). Fields precede it; body follows.
    size_t sep = pushes.size(); // sentinel = "no separator"
    for (size_t i = 0; i < pushes.size(); i += 2) {
        if (pushes[i].empty()) {
            sep = i;
            break;
        }
    }

    const size_t nFields = (sep == pushes.size()) ? pushes.size() : sep;
    if (sep != pushes.size()) {
        r.has_body = true;
        for (size_t i = sep + 1; i < pushes.size(); ++i) {
            r.body_chunks.push_back(pushes[i]);
        }
    }

    // Fields come in (tag, value) pairs. An odd count means a trailing tag with no value.
    if (nFields % 2 != 0) {
        r.error = "bad-txns-dnft-envelope-incomplete-field";
        return r;
    }

    for (size_t k = 0; k < nFields; k += 2) {
        const Span<const uint8_t> &tag = pushes[k];
        const Span<const uint8_t> &val = pushes[k + 1];
        // tag is guaranteed non-empty here: the first even-indexed empty push is the separator.
        if (tag.size() == 1) {
            switch (tag[0]) {
                case TAG_CONTENT_TYPE:
                    if (r.content_type) { r.error = "bad-txns-dnft-envelope-duplicate-field"; return r; }
                    r.content_type = val;
                    continue;
                case TAG_PARENT:
                    if (val.size() != PARENT_VALUE_LENGTH) { r.error = "bad-txns-dnft-envelope-bad-parent"; return r; }
                    r.parents.push_back(val);
                    continue;
                case TAG_METADATA:
                    r.metadata_chunks.push_back(val); // chunkable: concatenate across occurrences
                    continue;
                case TAG_METAPROTOCOL:
                    if (r.metaprotocol) { r.error = "bad-txns-dnft-envelope-duplicate-field"; return r; }
                    r.metaprotocol = val;
                    continue;
                case TAG_CONTENT_ENCODING:
                    if (r.content_encoding) { r.error = "bad-txns-dnft-envelope-duplicate-field"; return r; }
                    r.content_encoding = val;
                    continue;
                case TAG_DELEGATE:
                    if (r.delegate) { r.error = "bad-txns-dnft-envelope-duplicate-field"; return r; }
                    r.delegate = val;
                    continue;
                case TAG_NOP:
                    continue; // ignored
                default:
                    break; // fall through to unknown-tag handling
            }
        }
        // Unknown tag: odd first byte -> ignorable; even first byte -> the envelope is invalid.
        if ((tag[0] & 1u) == 0u) {
            r.error = "bad-txns-dnft-envelope-unknown-even-tag";
            return r;
        }
        // else: unknown odd tag, ignored (forward-compatible)
    }

    r.valid = true;
    return r;
}

std::vector<uint8_t> Concat(const std::vector<Span<const uint8_t>> &chunks) {
    size_t total = 0;
    for (const auto &c : chunks) total += c.size();
    std::vector<uint8_t> out;
    out.reserve(total);
    for (const auto &c : chunks) out.insert(out.end(), c.begin(), c.end());
    return out;
}

std::vector<uint8_t> ToVec(Span<const uint8_t> s) { return std::vector<uint8_t>(s.begin(), s.end()); }

} // namespace

bool IsDnftEnvelope(const CScript &spk) {
    const uint8_t *pc = spk.data();
    const uint8_t *const end = pc + spk.size();
    if (pc >= end || *pc != OP_RETURN) return false;
    ++pc;
    bool isPush = false;
    Span<const uint8_t> data;
    return NextElement(pc, end, isPush, data) && isPush && data.size() == MAGIC.size() &&
           std::memcmp(data.data(), MAGIC.data(), MAGIC.size()) == 0;
}

ParsedEnvelope ParseDnftEnvelope(const CScript &spk) {
    const SpanEnvelope s = ParseSpans(spk);
    ParsedEnvelope out;
    if (!s.is_envelope) {
        out.error = "not-a-dnft-envelope";
        return out;
    }
    if (!s.valid) {
        out.error = s.error;
        return out;
    }
    // Success: materialize owned copies.
    out.valid = true;
    if (s.content_type) out.content_type = ToVec(*s.content_type);
    for (const auto &p : s.parents) out.parents.push_back(ToVec(p));
    if (!s.metadata_chunks.empty()) out.metadata = Concat(s.metadata_chunks);
    if (s.metaprotocol) out.metaprotocol = ToVec(*s.metaprotocol);
    if (s.content_encoding) out.content_encoding = ToVec(*s.content_encoding);
    if (s.delegate) out.delegate = ToVec(*s.delegate);
    out.has_body = s.has_body;
    if (s.has_body) out.body = Concat(s.body_chunks);
    return out;
}

bool ValidateDnftEnvelope(const CScript &spk, std::vector<std::array<uint8_t, PARENT_VALUE_LENGTH>> *parentsOut,
                          std::string *err) {
    const SpanEnvelope s = ParseSpans(spk);
    if (!s.is_envelope) {
        if (err) *err = "not-a-dnft-envelope";
        return false;
    }
    if (!s.valid) {
        if (err) *err = s.error;
        return false;
    }
    if (parentsOut) {
        parentsOut->clear();
        parentsOut->reserve(s.parents.size());
        for (const auto &p : s.parents) {
            std::array<uint8_t, PARENT_VALUE_LENGTH> a;
            std::memcpy(a.data(), p.data(), PARENT_VALUE_LENGTH);
            parentsOut->push_back(a);
        }
    }
    return true;
}

CScript BuildDnftEnvelope(const EnvelopeFields &fields, Span<const uint8_t> body, bool include_body) {
    CScript s;
    s << OP_RETURN;
    s << std::vector<uint8_t>(MAGIC.begin(), MAGIC.end());

    const auto emit = [&s](uint8_t tag, const std::vector<uint8_t> &value) {
        s << std::vector<uint8_t>{tag};
        s << value;
    };
    // Deterministic field order.
    if (fields.content_type) emit(TAG_CONTENT_TYPE, *fields.content_type);
    for (const auto &p : fields.parents) emit(TAG_PARENT, p);
    if (fields.metadata) emit(TAG_METADATA, *fields.metadata);
    if (fields.metaprotocol) emit(TAG_METAPROTOCOL, *fields.metaprotocol);
    if (fields.content_encoding) emit(TAG_CONTENT_ENCODING, *fields.content_encoding);
    if (fields.delegate) emit(TAG_DELEGATE, *fields.delegate);

    if (include_body) {
        // Body separator: an empty push. The field section above always emits an even number of
        // pushes, so this empty push lands at an even index (the ord body-separator rule).
        s << std::vector<uint8_t>{};
        if (!body.empty()) {
            s << std::vector<uint8_t>(body.begin(), body.end());
        }
    }
    return s;
}

std::array<uint8_t, COMMITMENT_LENGTH> ComputeDnftCommitment(const CScript &envelope_spk,
                                                             const COutPoint &input0_prevout,
                                                             uint32_t token_vout_index) {
    // outpoint0 in the standard 36-byte serialization: txid(32, internal order) || LE32(n).
    uint8_t outpoint[36];
    std::memcpy(outpoint, input0_prevout.GetTxId().data(), 32);
    WriteLE32(outpoint + 32, input0_prevout.GetN());

    uint8_t idx[4];
    WriteLE32(idx, token_vout_index);

    uint8_t digest[CSHA256::OUTPUT_SIZE];
    CSHA256()
        .Write(envelope_spk.data(), envelope_spk.size())
        .Write(outpoint, sizeof(outpoint))
        .Write(idx, sizeof(idx))
        .Finalize(digest);

    std::array<uint8_t, COMMITMENT_LENGTH> out;
    out[0] = BINDING_VERSION;
    std::memcpy(out.data() + 1, digest, CSHA256::OUTPUT_SIZE);
    return out;
}

} // namespace dnft
