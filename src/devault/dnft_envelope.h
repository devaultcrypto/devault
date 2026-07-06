// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_DNFT_ENVELOPE_H
#define DEVAULT_DEVAULT_DNFT_ENVELOPE_H

#include <primitives/transaction.h> // COutPoint
#include <script/script.h>
#include <span.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * DNFT envelope codec (DEVAULT_NFT_SPEC.md §5, §6.4) — a pure, self-contained library: parse,
 * validate, construct, and hash the OP_RETURN content envelope of a DNFT mint. No consensus wiring
 * lives here (that is CheckDnftRules, §6/§7, phase 4C); this module has no dependencies on chain
 * state and can be unit-tested and fuzzed in isolation.
 *
 * Wire format of one envelope output's scriptPubKey:
 *
 *     OP_RETURN
 *     PUSH "DNFT"                    ; 4-byte protocol magic
 *     PUSH <tag> PUSH <value> ...    ; even-length field section (ord tag model)
 *     PUSH ""                        ; empty push at an even index = end of fields / start of body
 *     PUSH <body chunk> ...          ; body bytes (concatenated across chunks)
 *
 * The tag/body split follows ordinals exactly: fields are the pushes before the first EVEN-indexed
 * empty push (the body separator); everything after it is the body. Odd tags are ignorable; an
 * unknown EVEN tag makes the envelope invalid (forward-compat: old nodes reject envelopes they
 * cannot fully understand — and, since envelope validity is consensus at mint, reject the mint).
 */
namespace dnft {

//! 4-byte protocol magic ("DNFT").
inline constexpr std::array<uint8_t, 4> MAGIC = {{'D', 'N', 'F', 'T'}};

//! Commitment version byte prefixing an inscribed NFT's binding hash (§6). Values 0x02+ reserved
//! (continuation, future schemes). A commitment NOT starting with this byte is not an inscribed
//! DNFT and carries no envelope-binding obligation.
inline constexpr uint8_t BINDING_VERSION = 0x01;

//! Full length of an inscribed commitment: version byte + 32-byte SHA256.
inline constexpr size_t COMMITMENT_LENGTH = 1 + 32;

//! Length of a parent-claim value: category(32) || commitment(33) (amendment A1).
inline constexpr size_t PARENT_VALUE_LENGTH = 32 + COMMITMENT_LENGTH;

//! Tag numbers — identical to ordinals where semantics match (spec §5, Q7).
enum : uint8_t {
    TAG_CONTENT_TYPE = 1,
    TAG_PARENT = 3,       // repeatable; value = category(32) || commitment(33)
    TAG_METADATA = 5,     // CBOR; chunkable (multiple occurrences concatenate)
    TAG_METAPROTOCOL = 7,
    TAG_CONTENT_ENCODING = 9, // e.g. "br"
    TAG_DELEGATE = 11,    // item id: txid(32) || LE index (explorer-resolved)
    TAG_NOP = 255,
};

//! Result of parsing an envelope. All byte fields are owned copies (stable across the input
//! script's lifetime). On the reject path no field bytes are copied.
struct ParsedEnvelope {
    bool valid = false;
    //! Empty when the script is not a DNFT envelope at all (use IsDnftEnvelope to distinguish
    //! "not ours" from "ours but malformed"); otherwise a "bad-txns-dnft-*" reject reason.
    std::string error;

    std::optional<std::vector<uint8_t>> content_type;
    std::vector<std::vector<uint8_t>> parents; // each exactly PARENT_VALUE_LENGTH bytes
    std::optional<std::vector<uint8_t>> metadata;
    std::optional<std::vector<uint8_t>> metaprotocol;
    std::optional<std::vector<uint8_t>> content_encoding;
    std::optional<std::vector<uint8_t>> delegate;

    bool has_body = false; // false = no body separator present; true = body present (possibly empty)
    std::vector<uint8_t> body;
};

//! Cheap check: is this scriptPubKey an OP_RETURN output carrying the DNFT magic as its first push?
bool IsDnftEnvelope(const CScript &spk);

//! Parse and validate an envelope scriptPubKey. Single forward pass; allocates nothing on reject.
ParsedEnvelope ParseDnftEnvelope(const CScript &spk);

//! Consensus fast path (4C): validate structure and extract parent claims WITHOUT materializing
//! the (potentially ~1 MB) body or the other rendering-only fields. Returns true if the envelope
//! is valid; on false, `*err` (if non-null) holds the reject reason. `*parentsOut` (if non-null)
//! receives each 65-byte parent-claim value in order (duplicates preserved; the caller dedups).
bool ValidateDnftEnvelope(const CScript &spk, std::vector<std::array<uint8_t, PARENT_VALUE_LENGTH>> *parentsOut,
                          std::string *err);

//! Fields for the builder. Any subset may be set; order in the produced script is deterministic.
struct EnvelopeFields {
    std::optional<std::vector<uint8_t>> content_type;
    std::vector<std::vector<uint8_t>> parents; // each must be PARENT_VALUE_LENGTH bytes
    std::optional<std::vector<uint8_t>> metadata;
    std::optional<std::vector<uint8_t>> metaprotocol;
    std::optional<std::vector<uint8_t>> content_encoding;
    std::optional<std::vector<uint8_t>> delegate;
};

//! Build an envelope scriptPubKey from fields + body. When include_body is true a body separator is
//! emitted (even a zero-length body is then "present"); when false no separator/body is emitted.
CScript BuildDnftEnvelope(const EnvelopeFields &fields, Span<const uint8_t> body, bool include_body = true);

//! Compute the 33-byte inscribed-NFT commitment binding `envelope_spk` to its mint context (§6.4):
//!   BINDING_VERSION || SHA256( envelope_spk || input0_prevout(36) || LE32(token_vout_index) )
std::array<uint8_t, COMMITMENT_LENGTH> ComputeDnftCommitment(const CScript &envelope_spk,
                                                             const COutPoint &input0_prevout,
                                                             uint32_t token_vout_index);

} // namespace dnft

#endif // DEVAULT_DEVAULT_DNFT_ENVELOPE_H
