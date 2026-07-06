#!/usr/bin/env python3
"""Independent Python reference for the DeVault DNFT envelope codec (DEVAULT_NFT_SPEC.md §5, §6.4).

Deliberately a SEPARATE implementation from the C++ (src/devault/dnft_envelope.cpp) so the two can
cross-check each other in the 4C/4I functional harnesses — a bug in one is unlikely to be mirrored
in the other. Provides: build an envelope scriptPubKey, parse one, and compute the salted binding
commitment. No external dependencies.
"""
import hashlib
import struct

MAGIC = b"DNFT"
BINDING_VERSION = 0x01
COMMITMENT_LENGTH = 33
PARENT_VALUE_LENGTH = 65

OP_RETURN = 0x6A
OP_PUSHDATA1 = 0x4C
OP_PUSHDATA2 = 0x4D
OP_PUSHDATA4 = 0x4E

TAG_CONTENT_TYPE = 1
TAG_PARENT = 3
TAG_METADATA = 5
TAG_METAPROTOCOL = 7
TAG_CONTENT_ENCODING = 9
TAG_DELEGATE = 11
TAG_NOP = 255


# ---- script push encoding ----
def push(data: bytes) -> bytes:
    n = len(data)
    if n < OP_PUSHDATA1:
        return bytes([n]) + data
    if n <= 0xFF:
        return bytes([OP_PUSHDATA1, n]) + data
    if n <= 0xFFFF:
        return bytes([OP_PUSHDATA2]) + struct.pack("<H", n) + data
    return bytes([OP_PUSHDATA4]) + struct.pack("<I", n) + data


def build_envelope(content_type=None, parents=(), metadata=None, metaprotocol=None,
                   content_encoding=None, delegate=None, body=None, include_body=None):
    """Return the envelope scriptPubKey bytes. include_body defaults to (body is not None)."""
    s = bytes([OP_RETURN]) + push(MAGIC)
    if content_type is not None:
        s += push(bytes([TAG_CONTENT_TYPE])) + push(content_type)
    for p in parents:
        assert len(p) == PARENT_VALUE_LENGTH
        s += push(bytes([TAG_PARENT])) + push(p)
    if metadata is not None:
        s += push(bytes([TAG_METADATA])) + push(metadata)
    if metaprotocol is not None:
        s += push(bytes([TAG_METAPROTOCOL])) + push(metaprotocol)
    if content_encoding is not None:
        s += push(bytes([TAG_CONTENT_ENCODING])) + push(content_encoding)
    if delegate is not None:
        s += push(bytes([TAG_DELEGATE])) + push(delegate)
    inc = (body is not None) if include_body is None else include_body
    if inc:
        s += push(b"")  # separator
        if body:
            s += push(body)
    return s


# ---- script element iteration ----
def _elements(script: bytes):
    """Yield (is_push, data) for each element; raises ValueError on a truncated/oversized push."""
    i, n = 0, len(script)
    while i < n:
        op = script[i]; i += 1
        if op <= OP_PUSHDATA4:
            if op < OP_PUSHDATA1:
                size = op
            elif op == OP_PUSHDATA1:
                if i + 1 > n: raise ValueError("truncated")
                size = script[i]; i += 1
            elif op == OP_PUSHDATA2:
                if i + 2 > n: raise ValueError("truncated")
                size = struct.unpack_from("<H", script, i)[0]; i += 2
            else:
                if i + 4 > n: raise ValueError("truncated")
                size = struct.unpack_from("<I", script, i)[0]; i += 4
            if i + size > n: raise ValueError("truncated")
            yield (True, script[i:i + size]); i += size
        else:
            yield (False, bytes([op]))


class ParseResult:
    def __init__(self):
        self.valid = False
        self.error = ""
        self.content_type = None
        self.parents = []
        self.metadata = None
        self.metaprotocol = None
        self.content_encoding = None
        self.delegate = None
        self.has_body = False
        self.body = b""


def is_dnft_envelope(script: bytes) -> bool:
    if not script or script[0] != OP_RETURN:
        return False
    try:
        it = _elements(script[1:])
        is_push, data = next(it)
    except (StopIteration, ValueError):
        return False
    return is_push and data == MAGIC


def parse_envelope(script: bytes) -> ParseResult:
    r = ParseResult()
    if not script or script[0] != OP_RETURN:
        r.error = "not-a-dnft-envelope"; return r
    try:
        elems = list(_elements(script[1:]))
    except ValueError:
        # truncated somewhere after OP_RETURN; if magic didn't even parse it's "not ours"
        r.error = "bad-txns-dnft-envelope-malformed"
        # but distinguish "not ours": re-check the magic leniently
        try:
            first = next(_elements(script[1:]))
        except (StopIteration, ValueError):
            r.error = "not-a-dnft-envelope"
        else:
            if not (first[0] and first[1] == MAGIC):
                r.error = "not-a-dnft-envelope"
        return r
    if not elems or not elems[0][0] or elems[0][1] != MAGIC:
        r.error = "not-a-dnft-envelope"; return r

    pushes = []
    for is_push, data in elems[1:]:
        if not is_push:
            r.error = "bad-txns-dnft-envelope-malformed"; return r
        pushes.append(data)

    sep = len(pushes)
    for i in range(0, len(pushes), 2):
        if len(pushes[i]) == 0:
            sep = i; break
    n_fields = len(pushes) if sep == len(pushes) else sep
    if sep != len(pushes):
        r.has_body = True
        r.body = b"".join(pushes[sep + 1:])
    if n_fields % 2 != 0:
        r.error = "bad-txns-dnft-envelope-incomplete-field"; return r

    metadata_chunks = []
    for k in range(0, n_fields, 2):
        tag, val = pushes[k], pushes[k + 1]
        if len(tag) == 1:
            t = tag[0]
            if t == TAG_CONTENT_TYPE:
                if r.content_type is not None: r.error = "bad-txns-dnft-envelope-duplicate-field"; return r
                r.content_type = val; continue
            if t == TAG_PARENT:
                if len(val) != PARENT_VALUE_LENGTH: r.error = "bad-txns-dnft-envelope-bad-parent"; return r
                r.parents.append(val); continue
            if t == TAG_METADATA:
                metadata_chunks.append(val); continue
            if t == TAG_METAPROTOCOL:
                if r.metaprotocol is not None: r.error = "bad-txns-dnft-envelope-duplicate-field"; return r
                r.metaprotocol = val; continue
            if t == TAG_CONTENT_ENCODING:
                if r.content_encoding is not None: r.error = "bad-txns-dnft-envelope-duplicate-field"; return r
                r.content_encoding = val; continue
            if t == TAG_DELEGATE:
                if r.delegate is not None: r.error = "bad-txns-dnft-envelope-duplicate-field"; return r
                r.delegate = val; continue
            if t == TAG_NOP:
                continue
        if (tag[0] & 1) == 0:
            r.error = "bad-txns-dnft-envelope-unknown-even-tag"; return r
        # unknown odd tag -> ignore
    if metadata_chunks:
        r.metadata = b"".join(metadata_chunks)
    r.valid = True
    return r


# ---- transaction / CashTokens wrapping serialization (for the M9 raw-tx harness) ----
TOKEN_PREFIX = 0xEF
STRUCT_HAS_AMOUNT = 0x10
STRUCT_HAS_NFT = 0x20
STRUCT_HAS_COMMITMENT_LENGTH = 0x40
CAP_NONE, CAP_MUTABLE, CAP_MINTING = 0x00, 0x01, 0x02


def ser_compactsize(n: int) -> bytes:
    if n < 0xFD:
        return bytes([n])
    if n <= 0xFFFF:
        return b"\xfd" + n.to_bytes(2, "little")
    if n <= 0xFFFFFFFF:
        return b"\xfe" + n.to_bytes(4, "little")
    return b"\xff" + n.to_bytes(8, "little")


def wrap_token_spk(category_internal: bytes, capability: int, commitment, amount, real_spk: bytes) -> bytes:
    """0xef || tokendata || real_spk, matching token.h OutputData serialization.
    category_internal is the 32-byte category in internal (serialized) order."""
    assert len(category_internal) == 32
    has_nft = commitment is not None or capability != CAP_NONE
    bitfield = 0
    if has_nft:
        bitfield |= STRUCT_HAS_NFT | (capability & 0x0F)
        if commitment is not None:
            bitfield |= STRUCT_HAS_COMMITMENT_LENGTH
    if amount is not None:
        bitfield |= STRUCT_HAS_AMOUNT
    td = category_internal + bytes([bitfield])
    if commitment is not None:
        td += ser_compactsize(len(commitment)) + commitment
    if amount is not None:
        td += ser_compactsize(amount)
    return bytes([TOKEN_PREFIX]) + td + real_spk


def ser_output(value_sats: int, spk: bytes) -> bytes:
    return value_sats.to_bytes(8, "little") + ser_compactsize(len(spk)) + spk


def ser_tx(version: int, vins, vouts, locktime: int = 0) -> bytes:
    """vins: list of (txid_display_hex, vout, scriptSig_bytes, sequence). vouts: list of raw output bytes."""
    s = version.to_bytes(4, "little") + ser_compactsize(len(vins))
    for txid_hex, vout, script_sig, seq in vins:
        s += bytes.fromhex(txid_hex)[::-1] + vout.to_bytes(4, "little")
        s += ser_compactsize(len(script_sig)) + script_sig + seq.to_bytes(4, "little")
    s += ser_compactsize(len(vouts))
    for o in vouts:
        s += o
    s += locktime.to_bytes(4, "little")
    return s


def category_internal_from_txid(txid_display_hex: str) -> bytes:
    """A CashTokens category equals a txid; on the wire both are the internal (reversed) byte order."""
    return bytes.fromhex(txid_display_hex)[::-1]


def compute_commitment(envelope_spk: bytes, input0_txid_internal: bytes, input0_n: int,
                       token_vout_index: int) -> bytes:
    """0x01 || SHA256( envelope_spk || outpoint0(36) || le32(vout_index) ).
    input0_txid_internal is the 32-byte txid in internal (serialized) byte order."""
    assert len(input0_txid_internal) == 32
    preimage = envelope_spk + input0_txid_internal + struct.pack("<I", input0_n) + \
        struct.pack("<I", token_vout_index)
    return bytes([BINDING_VERSION]) + hashlib.sha256(preimage).digest()


if __name__ == "__main__":
    # Self-check against known vectors.
    spk = build_envelope(content_type=b"image/png", body=b"pixels")
    p = parse_envelope(spk)
    assert p.valid and p.content_type == b"image/png" and p.body == b"pixels", p.error
    assert is_dnft_envelope(spk)
    parent = bytes([0x11]) * 32 + bytes([BINDING_VERSION]) + bytes([0x22]) * 32
    spk2 = build_envelope(parents=[parent], delegate=b"\x7e" * 36, include_body=False)
    p2 = parse_envelope(spk2)
    assert p2.valid and p2.parents == [parent] and not p2.has_body and p2.delegate == b"\x7e" * 36
    c = compute_commitment(spk, bytes.fromhex("ab") + b"\x00" * 31, 3, 1)
    assert len(c) == COMMITMENT_LENGTH and c[0] == BINDING_VERSION
    print("dnft reference self-check OK")
