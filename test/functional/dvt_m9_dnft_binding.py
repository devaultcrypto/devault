#!/usr/bin/env python3
"""DeVault M9 — the DNFT binding rule end-to-end (DEVAULT_NFT_SPEC.md §6, §7).

The authoritative consensus test for phase 4C. Hand-builds real DNFT mint/transfer/burn
transactions (envelope = multi-push OP_RETURN + CashTokens-wrapped inscribed outputs), signs them
with the wallet, and exercises the binding rule through BOTH consensus paths:
  * mempool  (sendrawtransaction -> AcceptToMemoryPool)
  * block    (generatetoaddress mines accepted txs; a manual block injects rejects into ConnectBlock)

Verifies: single/batch/later-mint/transfer/burn/parent accept; the full reject matrix; reorg
(invalidate/reconsider); reindex-chainstate identical tip; and an independent recomputation of each
on-chain commitment via the separate Python reference (dvt_dnft_reference.py).
"""
import base64
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dvt_dnft_reference as ref

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29841
DU1_HEIGHT = 200

FAILURES = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


def sha256d(b):
    import hashlib
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()


class RpcError(Exception):
    def __init__(self, msg):
        super().__init__(msg)
        self.message = msg


class Rpc:
    def __init__(self, port, cookie):
        self.url = f"http://127.0.0.1:{port}"
        self.cookie = cookie

    def __call__(self, method, *params):
        auth = base64.b64encode(open(self.cookie, "rb").read()).decode()
        body = json.dumps({"id": 1, "method": method, "params": list(params)}).encode()
        req = urllib.request.Request(
            self.url, body, {"Authorization": "Basic " + auth, "Content-Type": "application/json"})
        try:
            resp = json.loads(urllib.request.urlopen(req, timeout=120).read())
        except urllib.error.HTTPError as e:
            resp = json.loads(e.read())
        if resp.get("error"):
            raise RpcError(resp["error"].get("message", ""))
        return resp["result"]


# --- block assembly for the ConnectBlock reject path (minimal, from the pool sim) ---
def ser_compact(n):
    if n < 0xFD:
        return bytes([n])
    if n <= 0xFFFF:
        return b"\xfd" + n.to_bytes(2, "little")
    return b"\xfe" + n.to_bytes(4, "little")


def scriptnum(n):
    out = bytearray()
    v = n
    while v:
        out.append(v & 0xFF)
        v >>= 8
    if out and (out[-1] & 0x80):
        out.append(0)
    return bytes(out)


def build_coinbase(height, value, script, payload, tag=b"/dvt-m9/"):
    outs = [(value, script)] + [(int(p["amount"]), bytes.fromhex(p["script"])) for p in payload]

    def ser(ss):
        s = (2).to_bytes(4, "little") + ser_compact(1) + b"\x00" * 32 + b"\xff\xff\xff\xff"
        s += ser_compact(len(ss)) + ss + b"\xff\xff\xff\xff" + ser_compact(len(outs))
        for v, spk in outs:
            s += int(v).to_bytes(8, "little") + ser_compact(len(spk)) + spk
        return s + (0).to_bytes(4, "little")

    ss = bytes([len(scriptnum(height))]) + scriptnum(height) + bytes([len(tag)]) + tag
    pad = 100 - len(ser(ss))
    if pad > 0:
        ss = bytes([len(scriptnum(height))]) + scriptnum(height) + bytes([len(tag) + pad]) + tag + b"\x00" * pad
    return ser(ss)


def merkle_root(txids):
    layer = [bytes.fromhex(t)[::-1] for t in txids]
    while len(layer) > 1:
        if len(layer) % 2:
            layer.append(layer[-1])
        layer = [sha256d(layer[i] + layer[i + 1]) for i in range(0, len(layer), 2)]
    return layer[0]


def mine_block(gbt, miner_script, extra_raw_txs):
    cb = build_coinbase(gbt["height"], gbt["coinbasevalue"], miner_script, gbt.get("coinbase_payload", []))
    cb_txid = sha256d(cb)[::-1].hex()
    extra_txids = [sha256d(bytes.fromhex(t))[::-1].hex() for t in extra_raw_txs]
    root = merkle_root([cb_txid] + extra_txids)
    base = (int(gbt["version"]).to_bytes(4, "little") + bytes.fromhex(gbt["previousblockhash"])[::-1] +
            root + int(gbt["curtime"]).to_bytes(4, "little") + bytes.fromhex(gbt["bits"])[::-1])
    target = int(gbt["target"], 16)
    for nonce in range(1 << 32):
        h = sha256d(base + nonce.to_bytes(4, "little"))
        if int.from_bytes(h, "little") <= target:
            break
    block = base + nonce.to_bytes(4, "little") + ser_compact(1 + len(extra_raw_txs)) + cb + b"".join(
        bytes.fromhex(t) for t in extra_raw_txs)
    return block.hex()


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-m9-")
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", "-allowunconnectedmining=1"]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))

    def wait_up():
        for _ in range(120):
            try:
                rpc("getblockchaininfo"); return
            except Exception:
                time.sleep(0.5)
        raise RuntimeError("node did not start")

    def coinbase_at(h):
        cb = rpc("getblock", rpc("getblockhash", h), 2)["tx"][0]
        return cb["txid"], int(round(cb["vout"][0]["value"] * 1e8))

    def addr_spk(a):
        return bytes.fromhex(rpc("validateaddress", a)["scriptPubKey"])

    def sign_send(unsigned_hex, mine=True):
        signed = rpc("signrawtransactionwithwallet", unsigned_hex)
        if not signed.get("complete"):
            raise RpcError("sign incomplete: " + json.dumps(signed.get("errors", [])))
        txid = rpc("sendrawtransaction", signed["hex"])
        if mine:
            rpc("generatetoaddress", 1, miner)
        return txid, signed["hex"]

    try:
        wait_up()
        print(f"== setup: mine past DU1 ({DU1_HEIGHT}) with mature coinbases", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        miner_script = addr_spk(miner)
        rpc("generatetoaddress", DU1_HEIGHT + 20, miner)
        cb_next = [10]  # first spendable mature coinbase height (well below tip-100)

        def next_coinbase():
            h = cb_next[0]; cb_next[0] += 1
            return coinbase_at(h)

        def envelope(ct="text/plain", body="hi", parents=()):
            return ref.build_envelope(content_type=ct.encode(), body=body.encode(),
                                      parents=list(parents))

        # ---- ACCEPT: single genesis mint ----
        print("== single mint (accept, mempool + mined)", flush=True)
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope("image/png", "PIXELS")
        nft_spk = addr_spk(rpc("getnewaddress"))
        # layout: vout0 = inscribed NFT, vout1 = change, vout2 = envelope
        commit = ref.compute_commitment(env, cat, 0, 0)  # nft at index 0, input0=(cb,0)
        nft_out = ref.ser_output(1 * 10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, None, nft_spk))
        change = ref.ser_output(cb_val - 2 * 10**8, miner_script)
        env_out = ref.ser_output(0, env)
        vins = [(cb_txid, 0, b"", 0xFFFFFFFF)]
        unsigned = ref.ser_tx(2, vins, [nft_out, change, env_out]).hex()
        # validate wrapping via the node before signing
        dec = rpc("decoderawtransaction", unsigned)
        td = dec["vout"][0].get("tokenData", {})
        check(td.get("category") == cb_txid and td.get("nft", {}).get("commitment") == commit.hex(),
              "hand-built token output decodes with the expected category+commitment")
        txid, _ = sign_send(unsigned)
        conf = rpc("getrawtransaction", txid, True)
        check(conf.get("confirmations", 0) >= 1, "single mint mined into a block (ConnectBlock accept)")
        # independent commitment cross-check from the reference
        onchain_commit = conf["vout"][0]["tokenData"]["nft"]["commitment"]
        check(onchain_commit == ref.compute_commitment(env, cat, 0, 0).hex(),
              "on-chain commitment matches the independent reference recomputation")
        mint1_txid, mint1_commit = txid, commit

        # ---- ACCEPT: batch mint (2 items, same collection genesis) ----
        print("== batch mint of 2 (accept)", flush=True)
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        e0, e1 = envelope("t/0", "a"), envelope("t/1", "b")
        # layout: vout0 nft0, vout1 nft1, vout2 change, vout3 env0, vout4 env1
        c0 = ref.compute_commitment(e0, cat, 0, 0)
        c1 = ref.compute_commitment(e1, cat, 0, 1)
        outs = [
            ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, c0, None, addr_spk(rpc("getnewaddress")))),
            ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, c1, None, addr_spk(rpc("getnewaddress")))),
            ref.ser_output(cb_val - 3 * 10**8, miner_script),
            ref.ser_output(0, e0),
            ref.ser_output(0, e1),
        ]
        txid, _ = sign_send(ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)], outs).hex())
        check(rpc("getrawtransaction", txid, True).get("confirmations", 0) >= 1, "batch mint accepted+mined")

        # ---- ACCEPT: transfer (move) the single-mint NFT, no envelope ----
        print("== transfer/move (accept, no envelope)", flush=True)
        move_to = addr_spk(rpc("getnewaddress"))
        fee_cb_txid, fee_cb_val = next_coinbase()  # fund the fee so the moved NFT keeps full value
        move_out = ref.ser_output(10**8,  # 1 DVT, above the ~0.6 DVT dust floor
                                  ref.wrap_token_spk(cat_of(rpc, mint1_txid, 0), ref.CAP_NONE,
                                                     commit_of(rpc, mint1_txid, 0), None, move_to))
        move_change = ref.ser_output(fee_cb_val - 10**8, miner_script)  # 1 DVT fee
        mv_txid, _ = sign_send(ref.ser_tx(2, [(mint1_txid, 0, b"", 0xFFFFFFFF),
                                              (fee_cb_txid, 0, b"", 0xFFFFFFFF)],
                                          [move_out, move_change]).hex())
        mv = rpc("getrawtransaction", mv_txid, True)
        check(mv.get("confirmations", 0) >= 1 and mv["vout"][0]["tokenData"]["nft"]["commitment"] == mint1_commit.hex(),
              "inscribed NFT moved (commitment preserved), no envelope required")

        # ---- ACCEPT: later-mint via a minting token ----
        print("== later-mint via minting token (accept)", flush=True)
        cb_txid, cb_val = next_coinbase()
        mcat = ref.category_internal_from_txid(cb_txid)
        mint_authority_spk = addr_spk(rpc("getnewaddress"))
        # tx A: genesis a minting token (no envelope; empty commitment -> not inscribed)
        mtok = ref.ser_output(10**8, ref.wrap_token_spk(mcat, ref.CAP_MINTING, None, None, mint_authority_spk))
        changeA = ref.ser_output(cb_val - 2 * 10**8, miner_script)
        txA, _ = sign_send(ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)], [mtok, changeA]).hex())
        check(rpc("getrawtransaction", txA, True).get("confirmations", 0) >= 1, "minting token genesis accepted")
        # tx B: spend the minting token (vin0), mint an inscribed item of the same collection, and
        # re-emit the authority. Fund from a coinbase (vin1). The binding salt is tx B's own input0
        # = (txA, 0), NOT the collection category.
        envB = envelope("t/later", "later")
        fundB_txid, fundB_val = next_coinbase()
        saltB = ref.category_internal_from_txid(txA)  # (txA, 0) is tx B's input0
        cB = ref.compute_commitment(envB, saltB, 0, 0)  # inscribed item at vout 0
        # layout: vout0 inscribed item, vout1 re-emitted minting token, vout2 envelope, vout3 change
        outsB = [
            ref.ser_output(10**8, ref.wrap_token_spk(mcat, ref.CAP_NONE, cB, None, addr_spk(rpc("getnewaddress")))),
            ref.ser_output(10**8, ref.wrap_token_spk(mcat, ref.CAP_MINTING, None, None, mint_authority_spk)),
            ref.ser_output(0, envB),
            ref.ser_output(fundB_val - 2 * 10**8, miner_script),
        ]
        vinsB = [(txA, 0, b"", 0xFFFFFFFF), (fundB_txid, 0, b"", 0xFFFFFFFF)]
        txB, _ = sign_send(ref.ser_tx(2, vinsB, outsB).hex())
        check(rpc("getrawtransaction", txB, True).get("confirmations", 0) >= 1, "later-mint via minting token accepted")

        # ---- ACCEPT: parent/child provenance ----
        print("== parent/child (accept)", flush=True)
        # mint a parent first
        cb_txid, cb_val = next_coinbase()
        pcat = ref.category_internal_from_txid(cb_txid)
        pe = envelope("t/parent", "P")
        p_spk = addr_spk(rpc("getnewaddress"))
        pc = ref.compute_commitment(pe, pcat, 0, 0)
        p_outs = [ref.ser_output(10**8, ref.wrap_token_spk(pcat, ref.CAP_NONE, pc, None, p_spk)),
                  ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, pe)]
        ptxid, _ = sign_send(ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)], p_outs).hex())
        # child: spend a fresh coinbase (child category + salt) AND the parent NFT; claim the parent
        cb2_txid, cb2_val = next_coinbase()
        ccat = ref.category_internal_from_txid(cb2_txid)
        claim = pcat + pc  # 65-byte parent claim = parent category || commitment
        ce = envelope("t/child", "C", parents=[claim])
        cc = ref.compute_commitment(ce, ccat, 0, 0)  # input0 = (cb2, 0)
        c_outs = [ref.ser_output(10**8, ref.wrap_token_spk(ccat, ref.CAP_NONE, cc, None, addr_spk(rpc("getnewaddress")))),
                  ref.ser_output(cb2_val - 10**8, miner_script), ref.ser_output(0, ce)]
        # vin0 = fresh coinbase (n==0, provides child category + salt), vin1 = parent NFT (spend-to-prove)
        cvins = [(cb2_txid, 0, b"", 0xFFFFFFFF), (ptxid, 0, b"", 0xFFFFFFFF)]
        ctxid, _ = sign_send(ref.ser_tx(2, cvins, c_outs).hex())
        check(rpc("getrawtransaction", ctxid, True).get("confirmations", 0) >= 1,
              "child mint claiming a spent parent accepted")

        # ---- REJECT matrix (mempool / ATMP path) ----
        print("== reject matrix (mempool)", flush=True)

        def expect_reject(label, unsigned_hex, reason):
            try:
                signed = rpc("signrawtransactionwithwallet", unsigned_hex)
                rpc("sendrawtransaction", signed["hex"])
                check(False, f"{label}: expected reject '{reason}' but accepted")
            except RpcError as e:
                check(reason in e.message, f"{label}: {e.message}")

        # binding-missing: mint with no envelope
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope()
        commit = ref.compute_commitment(env, cat, 0, 0)
        expect_reject("mint without envelope",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, None, addr_spk(rpc("getnewaddress")))),
                                  ref.ser_output(cb_val - 2 * 10**8, miner_script)]).hex(),
                      "bad-txns-dnft-binding-missing")

        # unclaimed: envelope with no mint
        cb_txid, cb_val = next_coinbase()
        expect_reject("envelope without mint",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(cb_val - 10**8, miner_script), ref.ser_output(0, envelope())]).hex(),
                      "bad-txns-dnft-envelope-unclaimed")

        # binding-mismatch: tampered commitment
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope()
        bad = bytearray(ref.compute_commitment(env, cat, 0, 0)); bad[5] ^= 0xFF
        expect_reject("tampered commitment",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, bytes(bad), None, addr_spk(rpc("getnewaddress")))),
                                  ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, env)]).hex(),
                      "bad-txns-dnft-binding-mismatch")

        # inscribed-has-amount (Q10)
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope()
        commit = ref.compute_commitment(env, cat, 0, 0)
        expect_reject("inscribed with fungible amount",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, 5, addr_spk(rpc("getnewaddress")))),
                                  ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, env)]).hex(),
                      "bad-txns-dnft-inscribed-has-amount")

        # invalid envelope (unknown even tag)
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        bad_env = bytes([ref.OP_RETURN]) + ref.push(ref.MAGIC) + ref.push(bytes([4])) + ref.push(b"x") + ref.push(b"")
        commit = ref.compute_commitment(bad_env, cat, 0, 0)
        expect_reject("invalid envelope",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, None, addr_spk(rpc("getnewaddress")))),
                                  ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, bad_env)]).hex(),
                      "bad-txns-dnft-envelope-unknown-even-tag")

        # parent-missing: claim a parent not spent
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        fake_claim = bytes(32) + bytes([ref.BINDING_VERSION]) + bytes(32)
        env = envelope(parents=[fake_claim])
        commit = ref.compute_commitment(env, cat, 0, 0)
        expect_reject("parent not spent",
                      ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                                 [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, None, addr_spk(rpc("getnewaddress")))),
                                  ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, env)]).hex(),
                      "bad-txns-dnft-parent-missing")

        # ---- REJECT via ConnectBlock (submitblock): unbound mint AND tampered commitment ----
        print("== reject via ConnectBlock (submitblock)", flush=True)

        def block_reject(label, out_list, reason):
            unsigned = ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)], out_list).hex()
            signed = rpc("signrawtransactionwithwallet", unsigned)["hex"]
            gbt = rpc("getblocktemplate")
            cnt = rpc("getblockcount")
            res = rpc("submitblock", mine_block(gbt, miner_script, [signed]))
            check(res is not None and reason in str(res), f"{label}: submitblock -> {res}")
            check(rpc("getblockcount") == cnt, f"{label}: rejected block did not extend the chain")

        # unbound mint in a block
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope()
        commit = ref.compute_commitment(env, cat, 0, 0)
        block_reject("unbound mint in block",
                     [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, commit, None, addr_spk(rpc("getnewaddress")))),
                      ref.ser_output(cb_val - 2 * 10**8, miner_script)],
                     "binding-missing")

        # tampered commitment in a block
        cb_txid, cb_val = next_coinbase()
        cat = ref.category_internal_from_txid(cb_txid)
        env = envelope()
        bad = bytearray(ref.compute_commitment(env, cat, 0, 0)); bad[7] ^= 0xFF
        block_reject("tampered commitment in block",
                     [ref.ser_output(10**8, ref.wrap_token_spk(cat, ref.CAP_NONE, bytes(bad), None, addr_spk(rpc("getnewaddress")))),
                      ref.ser_output(cb_val - 2 * 10**8, miner_script), ref.ser_output(0, env)],
                     "binding-mismatch")

        # ---- reorg: invalidate/reconsider a mint block ----
        print("== reorg (invalidate/reconsider a mint block)", flush=True)
        mint_block = rpc("getrawtransaction", mint1_txid, True)["blockhash"]
        tip_before = rpc("getbestblockhash")
        rpc("invalidateblock", mint_block)
        check(rpc("getrawtransaction", mint1_txid, True).get("confirmations", 0) in (0, None) or
              "confirmations" not in rpc("getrawtransaction", mint1_txid, True),
              "mint tx unconfirmed after invalidateblock")
        rpc("reconsiderblock", mint_block)
        check(rpc("getbestblockhash") == tip_before, "chain restored after reconsiderblock")
        check(rpc("getrawtransaction", mint1_txid, True).get("confirmations", 0) >= 1, "mint re-confirmed")

        # ---- reindex-chainstate: identical tip ----
        print("== reindex-chainstate", flush=True)
        tip = rpc("getbestblockhash")
        count = rpc("getblockcount")
        rpc("stop"); node.wait(timeout=60)
        node2 = subprocess.Popen(args + ["-reindex-chainstate"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            wait_up()
            # wait for reindex to finish
            for _ in range(240):
                if rpc("getblockcount") == count:
                    break
                time.sleep(0.5)
            check(rpc("getbestblockhash") == tip and rpc("getblockcount") == count,
                  "reindex-chainstate reproduced the identical tip (DNFT rules revalidate deterministically)")
            rpc("stop"); node2.wait(timeout=60)
        finally:
            if node2.poll() is None:
                node2.kill()

    finally:
        if node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=30)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"M9 RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("M9 RESULT: ALL PASS")


# helpers that read token data back off-chain
def cat_of(rpc, txid, vout):
    return bytes.fromhex(rpc("getrawtransaction", txid, True)["vout"][vout]["tokenData"]["category"])[::-1]


def commit_of(rpc, txid, vout):
    return bytes.fromhex(rpc("getrawtransaction", txid, True)["vout"][vout]["tokenData"]["nft"]["commitment"])


if __name__ == "__main__":
    main()
