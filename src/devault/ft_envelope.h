// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_FT_ENVELOPE_H
#define DEVAULT_DEVAULT_FT_ENVELOPE_H

#include <script/script.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * DVFT envelope codec (DEVAULT_FT_SPEC.md §8) — a pure, self-contained library: parse, validate
 * and construct the OP_RETURN markers of the DeVault fungible-token system. No consensus wiring
 * lives here (that is CheckFtRules, spec §6, Phase 5C); the module has no chain-state
 * dependencies and is unit-tested and fuzzed in isolation, exactly like the DNFT codec (4B).
 *
 * Wire format of one DVFT marker scriptPubKey:
 *
 *     OP_RETURN
 *     PUSH "DVFT"                    ; 4-byte protocol magic (distinct from "DNFT", Q10)
 *     PUSH <tag> PUSH <value> ...    ; even-length field section (the ord tag model)
 *
 * Differences from the DNFT envelope, by design (spec §8):
 *   - NO body: a body separator (an empty push at an even element index) is invalid.
 *   - Every known field is single-valued: any duplicate known tag is invalid.
 *   - Integers are strict fixed-width little-endian (u64 = 8 bytes, u32 = 4 bytes, u8 = 1).
 *   - Exactly one role per envelope: a DEPLOY (tag 7 `mode` present, with the deploy field set)
 *     or a MINT (tag 23 `mint` present, and no deploy fields). Anything else is invalid.
 * Shared with DNFT: unknown EVEN tags are binding (invalid — old nodes must reject what they
 * cannot validate); unknown ODD tags are ignorable (forward-compatible extension space).
 */
namespace dnft {

//! 4-byte protocol magic ("DVFT").
inline constexpr std::array<uint8_t, 4> FT_MAGIC = {{'D', 'V', 'F', 'T'}};

//! Tag numbers (spec §8; provisional numbering finalized in 5B).
enum : uint8_t {
    FT_TAG_SYMBOL = 1,            // UTF-8 ticker, 1..16 bytes
    FT_TAG_NAME = 3,              // UTF-8 name, 1..64 bytes
    FT_TAG_DECIMALS = 5,          // u8, 0..8 (Q6)
    FT_TAG_MODE = 7,              // u8: 0x00 fixed, 0x01 open
    FT_TAG_QUANTITY = 9,          // u64 LE, >=1 (open only): tokens created per mint
    FT_TAG_PER_BLOCK_LIMIT = 11,  // u64 LE, >=1 (open only; REQUIRED, O1)
    FT_TAG_START_HEIGHT = 13,     // u32 LE (open only)
    FT_TAG_MAX_MINTS = 15,        // u64 LE, >=1 (open; mutually exclusive with tag 17)
    FT_TAG_END_HEIGHT = 17,       // u32 LE (open; mutually exclusive with tag 15; >= start)
    FT_TAG_PREMINE = 19,          // u64 LE (open only; absent == 0)
    FT_TAG_METADATA = 21,         // DNFT item id bytes: txid(32, internal) || le32(index)
    FT_TAG_MINT = 23,             // deploy transaction id D (32 bytes, internal order)
    FT_TAG_NOP = 255,             // ignored
};

inline constexpr uint8_t FT_MODE_FIXED = 0x00;
inline constexpr uint8_t FT_MODE_OPEN = 0x01;

inline constexpr size_t FT_MAX_SYMBOL_BYTES = 16; // Q8
inline constexpr size_t FT_MAX_NAME_BYTES = 64;   // Q8
inline constexpr uint8_t FT_MAX_DECIMALS = 8;     // Q6
inline constexpr size_t FT_METADATA_POINTER_LENGTH = 36;
inline constexpr size_t FT_DEPLOY_TXID_LENGTH = 32;

//! Result of parsing a DVFT envelope. Byte fields are owned copies. On the reject path nothing
//! beyond the reason string is populated.
struct ParsedFtEnvelope {
    bool valid = false;
    //! Empty-magic distinction: "not-a-dvft-envelope" when the script is not ours at all (use
    //! IsFtEnvelope to pre-classify); otherwise a "bad-txns-ft-envelope-*" reject reason.
    std::string error;

    bool is_deploy = false; // exactly one of these is set when valid
    bool is_mint = false;

    // ---- deploy fields ----
    std::vector<uint8_t> symbol; // 1..16 bytes
    std::vector<uint8_t> name;   // 1..64 bytes
    uint8_t decimals = 0;
    uint8_t mode = 0; // FT_MODE_FIXED / FT_MODE_OPEN
    // open-mode schedule; exactly one of max_mints / end_height was present (spec §5.2 — the
    // other is derived contextually in 5C, which also enforces start > deploy height etc.)
    uint64_t quantity_per_mint = 0;
    uint64_t per_block_limit = 0;
    uint32_t start_height = 0;
    std::optional<uint64_t> max_mints;
    std::optional<uint32_t> end_height;
    uint64_t premine = 0; // 0 == absent
    std::optional<std::array<uint8_t, FT_METADATA_POINTER_LENGTH>> metadata;

    // ---- mint field ----
    std::array<uint8_t, FT_DEPLOY_TXID_LENGTH> mint_deploy_txid{}; // internal byte order
};

//! Cheap detector: OP_RETURN whose first push is exactly the DVFT magic (the same predicate the
//! consensus layer uses to classify DVFT markers — mirrors IsDnftEnvelope).
bool IsFtEnvelope(const CScript &spk);

//! Parse and validate a DVFT envelope scriptPubKey. Single forward pass over the pushes;
//! allocates nothing but the reason string on reject.
ParsedFtEnvelope ParseFtEnvelope(const CScript &spk);

//! Deploy-builder inputs. `open` fields are used only when mode == FT_MODE_OPEN; exactly one of
//! max_mints / end_height must be set then (the builder asserts).
struct FtDeployParams {
    std::vector<uint8_t> symbol;
    std::vector<uint8_t> name;
    uint8_t decimals = 0;
    uint8_t mode = FT_MODE_FIXED;
    uint64_t quantity_per_mint = 0;
    uint64_t per_block_limit = 0;
    uint32_t start_height = 0;
    std::optional<uint64_t> max_mints;
    std::optional<uint32_t> end_height;
    uint64_t premine = 0; // emitted only when > 0
    std::optional<std::array<uint8_t, FT_METADATA_POINTER_LENGTH>> metadata;
};

//! Build a deploy envelope. Fields are emitted in ascending-tag order (deterministic encoding —
//! the golden vectors pin it). The caller is responsible for passing bound-valid params; the
//! result always satisfies ParseFtEnvelope (asserted in tests).
CScript BuildFtDeployEnvelope(const FtDeployParams &params);

//! Build a mint marker for the deploy transaction id `D` (internal byte order).
CScript BuildFtMintEnvelope(const std::array<uint8_t, FT_DEPLOY_TXID_LENGTH> &deployTxid);

} // namespace dnft

#endif // DEVAULT_DEVAULT_FT_ENVELOPE_H
