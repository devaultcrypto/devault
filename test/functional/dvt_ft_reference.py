#!/usr/bin/env python3
"""Independent Python reference for the DVFT envelope codec (DEVAULT_FT_SPEC.md §8).

A deliberately separate implementation from src/devault/ft_envelope.cpp, pinned to the same
golden vectors (the 4B discipline): a silent divergence between the C++ and Python codecs is
impossible to miss. Used by the Phase-5 functional harnesses (M13+) to hand-build deploy/mint
markers, and runnable directly (`python3 dvt_ft_reference.py`) as a golden self-check.
"""
import struct

OP_RETURN = 0x6A
MAGIC = b"DVFT"

TAG_SYMBOL = 1
TAG_NAME = 3
TAG_DECIMALS = 5
TAG_MODE = 7
TAG_QUANTITY = 9
TAG_PER_BLOCK_LIMIT = 11
TAG_START_HEIGHT = 13
TAG_MAX_MINTS = 15
TAG_END_HEIGHT = 17
TAG_PREMINE = 19
TAG_METADATA = 21
TAG_MINT = 23
TAG_NOP = 255

MODE_FIXED = 0x00
MODE_OPEN = 0x01

MAX_SYMBOL_BYTES = 16
MAX_NAME_BYTES = 64
MAX_DECIMALS = 8
METADATA_POINTER_LENGTH = 36
DEPLOY_TXID_LENGTH = 32


def push(data: bytes) -> bytes:
    n = len(data)
    if n <= 75:
        return bytes([n]) + data
    if n <= 0xFF:
        return b"\x4c" + bytes([n]) + data
    if n <= 0xFFFF:
        return b"\x4d" + struct.pack("<H", n) + data
    return b"\x4e" + struct.pack("<I", n) + data


def _tag(t: int, value: bytes) -> bytes:
    return push(bytes([t])) + push(value)


def build_ft_deploy(symbol: bytes, name: bytes, decimals: int, mode: int,
                    quantity=None, per_block_limit=None, start_height=None,
                    max_mints=None, end_height=None, premine: int = 0,
                    metadata: bytes = None) -> bytes:
    """Build a deploy envelope scriptPubKey, fields in ascending-tag order (the canonical
    encoding the golden vectors pin). Mirrors BuildFtDeployEnvelope exactly."""
    s = bytes([OP_RETURN]) + push(MAGIC)
    s += _tag(TAG_SYMBOL, symbol)
    s += _tag(TAG_NAME, name)
    s += _tag(TAG_DECIMALS, bytes([decimals]))
    s += _tag(TAG_MODE, bytes([mode]))
    if mode == MODE_OPEN:
        s += _tag(TAG_QUANTITY, struct.pack("<Q", quantity))
        s += _tag(TAG_PER_BLOCK_LIMIT, struct.pack("<Q", per_block_limit))
        s += _tag(TAG_START_HEIGHT, struct.pack("<I", start_height))
        assert (max_mints is None) != (end_height is None), "exactly one of max_mints/end_height"
        if max_mints is not None:
            s += _tag(TAG_MAX_MINTS, struct.pack("<Q", max_mints))
        else:
            s += _tag(TAG_END_HEIGHT, struct.pack("<I", end_height))
        if premine > 0:
            s += _tag(TAG_PREMINE, struct.pack("<Q", premine))
    if metadata is not None:
        assert len(metadata) == METADATA_POINTER_LENGTH
        s += _tag(TAG_METADATA, metadata)
    return s


def build_ft_mint(deploy_txid_internal: bytes) -> bytes:
    """Build a mint marker for the deploy txid (32 bytes, INTERNAL byte order)."""
    assert len(deploy_txid_internal) == DEPLOY_TXID_LENGTH
    return bytes([OP_RETURN]) + push(MAGIC) + _tag(TAG_MINT, deploy_txid_internal)


def build_ft_mint_display(deploy_txid_display_hex: str) -> bytes:
    """Same, from the display (RPC) hex form of the deploy txid."""
    return build_ft_mint(bytes.fromhex(deploy_txid_display_hex)[::-1])


# ---------------------------------------------------------------- parse (mirror of the C++)

def _elements(script: bytes):
    """Yield (is_push, data) mirroring GetScriptOp bounds; raise ValueError when truncated."""
    i = 0
    while i < len(script):
        op = script[i]; i += 1
        if op <= 0x4E:  # OP_PUSHDATA4
            if op < 0x4C:
                n = op
            elif op == 0x4C:
                if len(script) - i < 1: raise ValueError("truncated")
                n = script[i]; i += 1
            elif op == 0x4D:
                if len(script) - i < 2: raise ValueError("truncated")
                n = struct.unpack("<H", script[i:i+2])[0]; i += 2
            else:
                if len(script) - i < 4: raise ValueError("truncated")
                n = struct.unpack("<I", script[i:i+4])[0]; i += 4
            if len(script) - i < n: raise ValueError("truncated")
            yield True, script[i:i+n]; i += n
        else:
            yield False, b""


class FtParseResult:
    def __init__(self):
        self.valid = False
        self.error = ""
        self.is_deploy = False
        self.is_mint = False
        self.symbol = None
        self.name = None
        self.decimals = None
        self.mode = None
        self.quantity = None
        self.per_block_limit = None
        self.start_height = None
        self.max_mints = None
        self.end_height = None
        self.premine = 0
        self.metadata = None
        self.mint_deploy_txid = None  # internal byte order


def parse_ft_envelope(script: bytes) -> FtParseResult:
    r = FtParseResult()

    def reject(reason):
        r.error = reason
        return r

    if not script or script[0] != OP_RETURN:
        return reject("not-a-dvft-envelope")
    try:
        elems = list(_elements(script[1:]))
    except ValueError:
        try:
            first = next(_elements(script[1:]))
        except (StopIteration, ValueError):
            return reject("not-a-dvft-envelope")
        if not (first[0] and first[1] == MAGIC):
            return reject("not-a-dvft-envelope")
        return reject("bad-txns-ft-envelope-malformed")
    if not elems or not elems[0][0] or elems[0][1] != MAGIC:
        return reject("not-a-dvft-envelope")
    pushes = []
    for is_push, data in elems[1:]:
        if not is_push:
            return reject("bad-txns-ft-envelope-malformed")
        pushes.append(data)

    for i in range(0, len(pushes), 2):
        if len(pushes[i]) == 0:
            return reject("bad-txns-ft-envelope-has-body")
    if len(pushes) % 2 != 0:
        return reject("bad-txns-ft-envelope-incomplete-field")

    seen = set()
    for k in range(0, len(pushes), 2):
        tag, val = pushes[k], pushes[k + 1]
        if len(tag) == 1 and tag[0] in (TAG_SYMBOL, TAG_NAME, TAG_DECIMALS, TAG_MODE, TAG_QUANTITY,
                                        TAG_PER_BLOCK_LIMIT, TAG_START_HEIGHT, TAG_MAX_MINTS,
                                        TAG_END_HEIGHT, TAG_PREMINE, TAG_METADATA, TAG_MINT):
            t = tag[0]
            if t in seen:
                return reject("bad-txns-ft-envelope-duplicate-field")
            seen.add(t)
            if t == TAG_SYMBOL:
                if not 1 <= len(val) <= MAX_SYMBOL_BYTES: return reject("bad-txns-ft-envelope-bad-symbol")
                r.symbol = val
            elif t == TAG_NAME:
                if not 1 <= len(val) <= MAX_NAME_BYTES: return reject("bad-txns-ft-envelope-bad-name")
                r.name = val
            elif t == TAG_DECIMALS:
                if len(val) != 1 or val[0] > MAX_DECIMALS: return reject("bad-txns-ft-envelope-bad-decimals")
                r.decimals = val[0]
            elif t == TAG_MODE:
                if len(val) != 1 or val[0] not in (MODE_FIXED, MODE_OPEN): return reject("bad-txns-ft-envelope-bad-mode")
                r.mode = val[0]
            elif t == TAG_QUANTITY:
                if len(val) != 8 or val == b"\x00" * 8: return reject("bad-txns-ft-envelope-bad-quantity")
                r.quantity = struct.unpack("<Q", val)[0]
            elif t == TAG_PER_BLOCK_LIMIT:
                if len(val) != 8 or val == b"\x00" * 8: return reject("bad-txns-ft-envelope-bad-per-block-limit")
                r.per_block_limit = struct.unpack("<Q", val)[0]
            elif t == TAG_START_HEIGHT:
                if len(val) != 4: return reject("bad-txns-ft-envelope-bad-start-height")
                r.start_height = struct.unpack("<I", val)[0]
            elif t == TAG_MAX_MINTS:
                if len(val) != 8 or val == b"\x00" * 8: return reject("bad-txns-ft-envelope-bad-max-mints")
                r.max_mints = struct.unpack("<Q", val)[0]
            elif t == TAG_END_HEIGHT:
                if len(val) != 4: return reject("bad-txns-ft-envelope-bad-end-height")
                r.end_height = struct.unpack("<I", val)[0]
            elif t == TAG_PREMINE:
                if len(val) != 8: return reject("bad-txns-ft-envelope-bad-premine")
                r.premine = struct.unpack("<Q", val)[0]
            elif t == TAG_METADATA:
                if len(val) != METADATA_POINTER_LENGTH: return reject("bad-txns-ft-envelope-bad-metadata")
                r.metadata = val
            elif t == TAG_MINT:
                if len(val) != DEPLOY_TXID_LENGTH: return reject("bad-txns-ft-envelope-bad-mint")
                r.mint_deploy_txid = val
            continue
        if len(tag) == 1 and tag[0] == TAG_NOP:
            continue
        if tag[0] % 2 == 0:
            return reject("bad-txns-ft-envelope-unknown-even-tag")
        # unknown odd tag: ignored

    saw_mode = r.mode is not None
    saw_mint = r.mint_deploy_txid is not None
    if saw_mode == saw_mint:
        return reject("bad-txns-ft-envelope-role")
    deploy_fields = any(x is not None for x in
                        (r.symbol, r.name, r.decimals, r.quantity, r.per_block_limit,
                         r.start_height, r.max_mints, r.end_height, r.metadata)) or TAG_PREMINE in seen
    if saw_mint:
        if deploy_fields:
            return reject("bad-txns-ft-envelope-role")
        r.is_mint = True
        r.valid = True
        return r

    if r.symbol is None or r.name is None or r.decimals is None:
        return reject("bad-txns-ft-envelope-missing-field")
    open_fields = any(x is not None for x in (r.quantity, r.per_block_limit, r.start_height,
                                              r.max_mints, r.end_height)) or TAG_PREMINE in seen
    if r.mode == MODE_OPEN:
        if r.quantity is None or r.per_block_limit is None or r.start_height is None:
            return reject("bad-txns-ft-envelope-missing-field")
        if (r.max_mints is None) == (r.end_height is None):
            return reject("bad-txns-ft-envelope-schedule")
        if r.end_height is not None and r.end_height < r.start_height:
            return reject("bad-txns-ft-envelope-schedule")
    else:
        if open_fields:
            return reject("bad-txns-ft-envelope-schedule")
    r.is_deploy = True
    r.valid = True
    return r


# ---------------------------------------------------------------- golden self-check

GOLDEN_DEPLOY_HEX = (
    "6a0444564654010104474f4c4401030a476f6c6420546f6b656e0105010201070101010908640000000000000001"
    "0b080500000000000000010d042c010000010f08e803000000000000011308f4010000000000000115"
    "24abababababababababababababababababababababababababababababababababababab")
GOLDEN_MINT_HEX = (
    "6a04445646540117200102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20")
GOLDEN_FIXED_HEX = "6a0444564654010103555344010309555320446f6c6c61720105010801070100"


def self_check():
    d = build_ft_deploy(b"GOLD", b"Gold Token", 2, MODE_OPEN, quantity=100, per_block_limit=5,
                        start_height=300, max_mints=1000, premine=500, metadata=b"\xab" * 36)
    assert d.hex() == GOLDEN_DEPLOY_HEX, d.hex()
    p = parse_ft_envelope(d)
    assert p.valid and p.is_deploy and p.symbol == b"GOLD" and p.quantity == 100 \
        and p.per_block_limit == 5 and p.start_height == 300 and p.max_mints == 1000 \
        and p.premine == 500 and p.end_height is None, p.error

    m = build_ft_mint(bytes(range(1, 33)))
    assert m.hex() == GOLDEN_MINT_HEX, m.hex()
    q = parse_ft_envelope(m)
    assert q.valid and q.is_mint and q.mint_deploy_txid == bytes(range(1, 33)), q.error

    f = build_ft_deploy(b"USD", b"US Dollar", 8, MODE_FIXED)
    assert f.hex() == GOLDEN_FIXED_HEX, f.hex()
    assert parse_ft_envelope(f).valid

    # A few reject spot-checks mirroring the C++ matrix.
    hdr = bytes([OP_RETURN]) + push(MAGIC)
    assert parse_ft_envelope(hdr + b"\x51").error == "bad-txns-ft-envelope-malformed"
    assert parse_ft_envelope(hdr + push(b"\x01")).error == "bad-txns-ft-envelope-incomplete-field"
    assert parse_ft_envelope(hdr + push(b"")).error == "bad-txns-ft-envelope-has-body"
    assert parse_ft_envelope(hdr).error == "bad-txns-ft-envelope-role"
    assert parse_ft_envelope(hdr + _tag(2, b"\x41")).error == "bad-txns-ft-envelope-unknown-even-tag"
    print("dvt_ft_reference: all golden self-checks pass")


if __name__ == "__main__":
    self_check()
