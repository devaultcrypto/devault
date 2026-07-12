// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_ENVELOPE_UTIL_H
#define DEVAULT_DEVAULT_ENVELOPE_UTIL_H

#include <crypto/common.h>
#include <script/script.h>
#include <span.h>

#include <cstdint>

/**
 * Shared low-level push iteration for the DeVault OP_RETURN envelope codecs (DNFT "DNFT" and
 * DFT "DVFT"). ONLY the bounds-critical element walk lives here — one reviewed implementation —
 * while each protocol keeps its own tag-model interpretation (they differ deliberately: DNFT has
 * a body separator, repeatable/chunkable fields and 65-byte parent values; DVFT is body-less with
 * single-valued, fixed-width fields). Extracted verbatim from dnft_envelope.cpp in Phase 5B; the
 * DNFT golden vectors and fuzz corpus pin its behavior across the move.
 */
namespace dnft {

// Read the next script element starting at `pc` (< `end`).
//   returns false  -> end reached, or a truncated/oversized push (malformed script)
//   returns true, isPush=true,  data=<span of pushed bytes>  -> a canonical data push (OP_0..OP_PUSHDATA4)
//   returns true, isPush=false                               -> some other opcode (not a data push)
// Bounds logic mirrors GetScriptOp() exactly but yields a zero-copy Span instead of copying.
inline bool NextElement(const uint8_t *&pc, const uint8_t *end, bool &isPush, Span<const uint8_t> &data) {
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

} // namespace dnft

#endif // DEVAULT_DEVAULT_ENVELOPE_UTIL_H
