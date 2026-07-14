#!/usr/bin/env python3
"""DeVault M13 — the fungible-token consensus core (DEVAULT_FT_SPEC.md §6), functional test.

Drives a real devaultd on regtest with hand-built raw transactions (the independent Python codec
dvt_ft_reference.py), verifying:

  1. DEPLOY: an open-mint deploy registers; the reject matrix (vin[0] not index-0, start not after
     the deploy block, premine mismatch, supply overflow).
  2. MINT lifecycle to the EXACT cap: with N=5, M=2 the stateless schedule allows 2, 2, 1 mints in
     consecutive blocks and then closes — total supply is exactly N*Q, with no counter anywhere.
  3. The per-block cap is backstopped by ConnectBlock: a hand-built block carrying M+1 mints of the
     same deploy is REJECTED by submitblock. (BlockAssembler packing lands in 5E; until then the
     mempool is managed by hand here.)
  4. The carve-out is exact and does not leak: Q+-1 rejected, an unregistered deploy rejected, an
     unmarked ex-nihilo FT creation still rejected by plain CashTokens conservation.
  5. STATELESS REORG: invalidating blocks that contain mints rolls back NO emission state — the very
     same mints re-apply on the new branch and remain valid.
  6. The deploy registry is REBUILDABLE: deleting its database and restarting reconstructs it from
     block data alone (deploy validity is view-free), and minting still works.
  7. Crash safety: a kill -9 mid-operation is reconciled by a forward replay at startup.
"""
import base64
import hashlib
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dvt_dnft_reference as dref   # ser_tx / ser_output / wrap_token_spk / category_internal
import dvt_ft_reference as fref     # DVFT deploy + mint envelopes
from dvt_m9_dnft_binding import mine_block

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29771
DU1_HEIGHT = 200
FT_HEIGHT = 260

# The deploy under test: Q=100 per mint, M=2 mints/block, N=5 total.
# The schedule therefore allows 2, 2, 1 mints in three consecutive blocks -> EXACTLY 5 mints.
Q, M, N = 100, 2, 5

FAILURES = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


class RpcError(Exception):
    def __init__(self, err):
        super().__init__(err.get("message", ""))
        self.message = err.get("message", "")


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
            resp = json.loads(urllib.request.urlopen(req, timeout=180).read())
        except urllib.error.HTTPError as e:
            resp = json.loads(e.read())
        if resp.get("error"):
            raise RpcError(resp["error"])
        return resp["result"]


def ctor_sort(raw_hexes):
    """DeVault blocks are canonically ordered: txs after the coinbase must be STRICTLY ASCENDING by
    TxId, compared as raw (internal) bytes -- validation.cpp rejects otherwise with `tx-ordering`.
    Sorting here is what lets the block reach the FT rules at all."""
    def key(raw_hex):
        b = bytes.fromhex(raw_hex)
        internal = hashlib.sha256(hashlib.sha256(b).digest()).digest()
        # uint256::Compare is MSB-first over little-endian storage (uint256.h) -- i.e. TxId ordering
        # is lexicographic over the DISPLAY (reversed) bytes, not the internal ones.
        return internal[::-1]
    return sorted(raw_hexes, key=key)


def start_node(datadir, rpc, extra=()):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", f"-ftforkactivationheight={FT_HEIGHT}",
            "-acceptnonstdtxn=0", "-allowunconnectedmining=1", *extra]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(240):
        try:
            rpc("getblockchaininfo"); return node
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not start")


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-m13-")
    cookie = os.path.join(datadir, "regtest", ".cookie")
    rpc = Rpc(RPCPORT, cookie)
    node = start_node(datadir, rpc)

    cb_next = [10]

    def next_coinbase():
        """A mature coinbase's vout 0 (value in sats). Its txid is a valid genesis category."""
        h = cb_next[0]; cb_next[0] += 1
        cb = rpc("getblock", rpc("getblockhash", h), 2)["tx"][0]
        return cb["txid"], int(round(cb["vout"][0]["value"] * 1e8))

    def addr_spk(a=None):
        a = a or rpc("getnewaddress")
        return bytes.fromhex(rpc("validateaddress", a)["scriptPubKey"])

    def sign_send(raw_hex, expect_ok=True):
        signed = rpc("signrawtransactionwithwallet", raw_hex)
        if not signed.get("complete"):
            raise RpcError({"message": f"sign incomplete: {signed}"})
        if not expect_ok:
            return signed["hex"]
        return rpc("sendrawtransaction", signed["hex"])

    def sign_only(raw_hex):
        signed = rpc("signrawtransactionwithwallet", raw_hex)
        assert signed.get("complete"), signed
        return signed["hex"]

    def build_deploy(cb_txid, cb_val, start, q=Q, m=M, n=N, premine=0, end_height=None,
                     vin0_index=0, extra_genesis_amount=None):
        """A deploy tx: vin[0] spends (cb_txid, vin0_index); the category is cb_txid."""
        cat = dref.category_internal_from_txid(cb_txid)
        env = fref.build_ft_deploy(b"GOLD", b"Gold Token", 2, fref.MODE_OPEN, quantity=q,
                                   per_block_limit=m, start_height=start,
                                   max_mints=(n if end_height is None else None),
                                   end_height=end_height, premine=premine)
        outs = []
        amt = premine if extra_genesis_amount is None else extra_genesis_amount
        if amt:
            outs.append(dref.ser_output(10**8, dref.wrap_token_spk(cat, dref.CAP_NONE, None, amt,
                                                                   addr_spk())))
        outs.append(dref.ser_output(0, env))
        outs.append(dref.ser_output(cb_val - (10**8 if amt else 0) - 10**8, addr_spk()))  # change
        return dref.ser_tx(2, [(cb_txid, vin0_index, b"", 0xFFFFFFFF)], outs).hex(), cat

    def build_mint(deploy_txid, cat, out_amount, fund_txid, fund_val, token_in=None):
        """A mint tx: marker names the DEPLOY txid; creates `out_amount` of `cat` ex nihilo."""
        env = fref.build_ft_mint_display(deploy_txid)
        vins = [(fund_txid, 0, b"", 0xFFFFFFFF)]
        if token_in:
            vins.append((token_in[0], token_in[1], b"", 0xFFFFFFFF))
        outs = []
        if out_amount:
            outs.append(dref.ser_output(10**8, dref.wrap_token_spk(cat, dref.CAP_NONE, None,
                                                                   out_amount, addr_spk())))
        outs.append(dref.ser_output(0, env))
        outs.append(dref.ser_output(fund_val - (10**8 if out_amount else 0) - 10**8, addr_spk()))
        return dref.ser_tx(2, vins, outs).hex()

    try:
        print(f"== setup: du1={DU1_HEIGHT} ftfork={FT_HEIGHT}; mine past the FT fork", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", 300, miner)
        # Lock the coinbases we hand-spend so wallet coin selection cannot race us for them.
        reserved = [next_coinbase() for _ in range(24)]
        rpc("lockunspent", False, [{"txid": t, "vout": 0} for t, _ in reserved])
        res = iter(reserved)

        # ---- 1. DEPLOY ----
        print("== 1. open-mint deploy + reject matrix", flush=True)
        cb, val = next(res)
        h = rpc("getblockcount")
        raw, cat = build_deploy(cb, val, start=h + 2)  # deploy lands in block h+1 => start = h+2
        deploy_txid = sign_send(raw)
        rpc("generatetoaddress", 1, miner)
        deploy_height = rpc("getrawtransaction", deploy_txid, True)
        check(deploy_height.get("blockhash") is not None, "open-mint deploy accepted and mined")
        dh = rpc("getblock", deploy_height["blockhash"])["height"]
        start = dh + 1
        check(start == h + 2, f"start height is the block after the deploy ({start})")

        # reject: vin[0] does not spend an index-0 prevout
        cb2, val2 = next(res)
        try:
            raw2, _ = build_deploy(cb2, val2, start=rpc("getblockcount") + 2, vin0_index=1)
            sign_send(raw2)
            check(False, "deploy with a non-index-0 vin[0] should be rejected")
        except RpcError as e:
            check("bad-txns-ft-deploy-no-genesis-input" in e.message
                  or "Input not found" in e.message,
                  f"deploy without an index-0 vin[0] rejected: {e.message[:70]}")

        # reject: start height not strictly after the deploy block
        cb3, val3 = next(res)
        try:
            raw3, _ = build_deploy(cb3, val3, start=rpc("getblockcount"))  # in the past
            sign_send(raw3)
            check(False, "deploy with start <= deploy height should be rejected")
        except RpcError as e:
            check("bad-txns-ft-deploy-start-not-after-deploy" in e.message,
                  f"deploy with start <= deploy height rejected: {e.message[:70]}")

        # reject: premine mismatch (declares 0, creates tokens)
        cb4, val4 = next(res)
        try:
            raw4, _ = build_deploy(cb4, val4, start=rpc("getblockcount") + 2, premine=0,
                                   extra_genesis_amount=500)
            sign_send(raw4)
            check(False, "deploy with a premine mismatch should be rejected")
        except RpcError as e:
            check("bad-txns-ft-deploy-premine-mismatch" in e.message,
                  f"premine mismatch rejected: {e.message[:70]}")

        # reject: supply overflow (premine + N*Q beyond the token ceiling)
        cb5, val5 = next(res)
        try:
            raw5, _ = build_deploy(cb5, val5, start=rpc("getblockcount") + 2,
                                   q=2**40, m=1, n=2**40)
            sign_send(raw5)
            check(False, "deploy overflowing the supply ceiling should be rejected")
        except RpcError as e:
            check("bad-txns-ft-deploy-supply-overflow" in e.message,
                  f"supply overflow rejected: {e.message[:70]}")

        # ---- 2. mint reject matrix (at a height inside the window) ----
        print("== 2. mint reject matrix", flush=True)
        while rpc("getblockcount") + 1 < start:
            rpc("generatetoaddress", 1, miner)
        check(rpc("getblockcount") + 1 == start, f"next block is the window's first ({start})")

        f1, fv1 = next(res)
        for bad_amt, label in ((Q + 1, "Q+1"), (Q - 1, "Q-1")):
            try:
                sign_send(build_mint(deploy_txid, cat, bad_amt, f1, fv1))
                check(False, f"mint creating {label} should be rejected")
            except RpcError as e:
                check("bad-txns-ft-mint-quantity" in e.message,
                      f"mint of {label} rejected (exact-Q): {e.message[:60]}")

        # a marker naming an unregistered deploy
        try:
            sign_send(build_mint("11" * 32, cat, Q, f1, fv1))
            check(False, "mint naming an unregistered deploy should be rejected")
        except RpcError as e:
            check("bad-txns-ft-mint-no-deploy" in e.message,
                  f"mint naming an unregistered deploy rejected: {e.message[:60]}")

        # an UNMARKED tx creating FT ex nihilo: still rejected by plain CashTokens conservation
        try:
            outs = [dref.ser_output(10**8, dref.wrap_token_spk(cat, dref.CAP_NONE, None, Q, addr_spk())),
                    dref.ser_output(fv1 - 2 * 10**8, addr_spk())]
            sign_send(dref.ser_tx(2, [(f1, 0, b"", 0xFFFFFFFF)], outs).hex())
            check(False, "an unmarked ex-nihilo FT creation should be rejected")
        except RpcError as e:
            check("bad-txns-token-invalid-category" in e.message,
                  f"unmarked ex-nihilo FT creation still rejected: {e.message[:60]}")

        # ---- 3. mint lifecycle to the EXACT cap (2, 2, 1) ----
        print(f"== 3. lifecycle: Q={Q} M={M} N={N} -> the schedule allows 2,2,1 then closes", flush=True)
        minted = 0
        per_block = []
        for blk in range(3):
            allowed = min(M, N - minted)
            txids = []
            for _ in range(allowed):
                ftx, fval = next(res)
                txids.append(sign_send(build_mint(deploy_txid, cat, Q, ftx, fval)))
            minted += allowed
            per_block.append(allowed)
            # One more mint would exceed this block's allowance. ATMP is loose (spec §6.6), so it may
            # be ACCEPTED to the mempool; it must not be mined into this block. Keep the mempool
            # clean (BlockAssembler packing arrives in 5E) and prove the cap at ConnectBlock below.
            rpc("generatetoaddress", 1, miner)
            for t in txids:
                check(rpc("getrawtransaction", t, True).get("blockhash") is not None,
                      f"block {blk}: mint {t[:10]} confirmed")
        check(per_block == [2, 2, 1], f"the schedule delivered 2,2,1 mints per block ({per_block})")
        check(minted == N, f"exactly N={N} mints occurred (total supply {minted * Q} = N*Q)")

        # the window is now CLOSED: any further mint is rejected (cap exhausted, no counter needed)
        fx, fxv = next(res)
        try:
            sign_send(build_mint(deploy_txid, cat, Q, fx, fxv))
            check(False, "a mint past the cap should be rejected")
        except RpcError as e:
            check("bad-txns-ft-mint-schedule" in e.message,
                  f"mint past the cap rejected by the stateless schedule: {e.message[:60]}")

        # ---- 4. the ConnectBlock backstop: M+1 mints of one deploy in a single block ----
        print("== 4. ConnectBlock backstop: M+1 mints of a deploy in one block is invalid", flush=True)
        cb6, val6 = next(res)
        h2 = rpc("getblockcount")
        raw6, cat6 = build_deploy(cb6, val6, start=h2 + 2, q=Q, m=M, n=100)
        deploy2 = sign_send(raw6)
        rpc("generatetoaddress", 1, miner)  # deploy lands; start = h2+2 = next block
        check(rpc("getrawtransaction", deploy2, True).get("blockhash") is not None,
              "second deploy (large N) mined")

        # Hand-build a block containing M+1 = 3 mints of deploy2 -> must be rejected at ConnectBlock.
        raws = []
        for _ in range(M + 1):
            ftx, fval = next(res)
            raws.append(sign_only(build_mint(deploy2, cat6, Q, ftx, fval)))
        gbt = rpc("getblocktemplate")
        blk = mine_block(gbt, addr_spk(miner), ctor_sort(raws))
        rej = rpc("submitblock", blk)
        check(rej is not None and "ft-mint-schedule" in str(rej),
              f"a block with M+1 mints of one deploy is rejected: {rej}")

        # ...and exactly M of them IS accepted.
        gbt = rpc("getblocktemplate")
        blk_ok = mine_block(gbt, addr_spk(miner), ctor_sort(raws[:M]))
        check(rpc("submitblock", blk_ok) is None, f"a block with exactly M={M} mints is accepted")

        # ---- 5. STATELESS reorg: emission has nothing to roll back ----
        print("== 5. stateless reorg: the same mints re-apply on the new branch", flush=True)
        tip_hash = rpc("getbestblockhash")
        mint_ids = [t["txid"] for t in rpc("getblock", tip_hash, 2)["tx"][1:]]
        check(len(mint_ids) == M, f"the tip block holds the {M} mints")
        rpc("invalidateblock", tip_hash)
        check(all(t in rpc("getrawmempool") for t in mint_ids),
              "invalidated mints returned to the mempool")
        rpc("generatetoaddress", 1, miner)  # re-mine them at the same height
        for t in mint_ids:
            check(rpc("getrawtransaction", t, True).get("blockhash") is not None,
                  f"mint {t[:10]} re-applied after the reorg (no emission state to roll back)")

        # A deploy that is reorged OUT must become unmintable (the registry entry is erased).
        cb7, val7 = next(res)
        h3 = rpc("getblockcount")
        raw7, cat7 = build_deploy(cb7, val7, start=h3 + 2, q=Q, m=M, n=100)
        deploy3 = sign_send(raw7)
        rpc("generatetoaddress", 1, miner)
        d3_block = rpc("getrawtransaction", deploy3, True)["blockhash"]
        check(g_lookup_ok(rpc, deploy3, cat7, next(res), build_mint, sign_only),
              "deploy3 is registered (a mint against it validates)")
        rpc("invalidateblock", d3_block)  # deploy3 is now off-chain
        f3, f3v = next(res)
        try:
            sign_send(build_mint(deploy3, cat7, Q, f3, f3v))
            check(False, "a mint against a reorged-out deploy should be rejected")
        except RpcError as e:
            check("bad-txns-ft-mint-no-deploy" in e.message,
                  f"the reorged-out deploy is no longer registered: {e.message[:60]}")
        rpc("reconsiderblock", d3_block)
        check(rpc("getrawtransaction", deploy3, True).get("blockhash") is not None,
              "deploy3 restored after reconsiderblock")

        # ---- 6. the registry is rebuildable from block data alone ----
        print("== 6. registry rebuild: delete the DB, restart, reconstruct from blocks", flush=True)
        tip = rpc("getbestblockhash")
        rpc("stop"); node.wait(timeout=90)
        shutil.rmtree(os.path.join(datadir, "regtest", "ftregistry"))
        node = start_node(datadir, rpc)
        check(rpc("getbestblockhash") == tip, "chain intact after deleting the registry DB")
        f4, f4v = next(res)
        mint_after_rebuild = sign_send(build_mint(deploy2, cat6, Q, f4, f4v))
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", mint_after_rebuild, True).get("blockhash") is not None,
              "the registry was rebuilt from block data — minting works again")

        # ---- 7. crash safety: kill -9, then forward-replay reconciliation at startup ----
        print("== 7. crash safety: kill -9 then reconcile", flush=True)
        rpc("generatetoaddress", 2, miner)
        d2_height = rpc("getblock", rpc("getrawtransaction", deploy2, True)["blockhash"])["height"]
        pre_crash_height = rpc("getblockcount")
        node.send_signal(signal.SIGKILL)
        node.wait(timeout=60)
        node = start_node(datadir, rpc)
        # A hard kill may legitimately lose an un-fsynced tip block, so the tip is not required to be
        # byte-identical. What 5C MUST guarantee is that the node recovers and the registry is
        # reconciled to whatever tip it lands on -- never AHEAD of the chainstate (the registry is
        # flushed after it), so a forward replay always suffices.
        post = rpc("getblockcount")
        check(post <= pre_crash_height and post >= d2_height,
              f"node recovered after kill -9 (height {post}; deploy still on-chain at {d2_height})")
        f5, f5v = next(res)
        mint_after_crash = sign_send(build_mint(deploy2, cat6, Q, f5, f5v))
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", mint_after_crash, True).get("blockhash") is not None,
              "the registry reconciled after the crash — minting works")

    finally:
        if node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=60)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"M13 RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("M13 RESULT: ALL PASS")


def g_lookup_ok(rpc, deploy, cat, funding, build_mint, sign_only):
    """testmempoolaccept a mint against `deploy` without broadcasting it."""
    raw = sign_only(build_mint(deploy, cat, Q, funding[0], funding[1]))
    res = rpc("testmempoolaccept", [raw])
    return bool(res) and res[0].get("allowed") is True


if __name__ == "__main__":
    main()
