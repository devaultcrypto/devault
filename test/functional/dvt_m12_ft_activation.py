#!/usr/bin/env python3
"""DeVault M12 — fungible-token (FT) fork activation + reorg-safe boundary, functional test.

Verifies on regtest, against a real devaultd with -du1activationheight and -ftforkactivationheight:
  1. PRE-fork (below ftForkHeight): a fungible-amount token genesis is rejected by the FT-deferral
     gate (bad-txns-token-ft-deferred).
  2. POST-fork: a fixed-supply FT genesis (full supply, split across multiple outputs) is accepted
     and mined; a plain FT transfer conserves the amount; a partial burn decreases it. (CashTokens
     conservation governs these — CheckFtRules is a 5A no-op.)
  3. Restart across the boundary is clean and the token state persists.
  4. DEEP REORG below ftForkHeight leaves a HEALTHY node: with an FT tx in the mempool,
     invalidating back below the fork drops the now-invalid FT tx and getblocktemplate stays
     healthy — and re-activating the fork lets FT txs flow again. (Note: invalidateblock re-validates
     the whole mempool itself via importMempool, so this end-to-end check is belt-and-suspenders; the
     4I review #2 boundary fix — NextBlockUpgradeBoundary treating ftForkHeight as an upgrade
     boundary for the natural-reorg DisconnectTip/ConnectTip mempool purge — is teeth-verified by the
     dnft_activation_tests/ft_fork_upgrade_boundary unit test, which fails if the fix is removed.)

Standalone (no test_framework dependency): drives devaultd over JSON-RPC.
"""
import base64
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29761
DU1_HEIGHT = 200
FT_HEIGHT = 260  # ftForkHeight: FT rules apply to blocks with height > FT_HEIGHT (first is 261)

FAILURES = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


class RpcError(Exception):
    def __init__(self, err):
        super().__init__(err.get("message", ""))
        self.code = err.get("code")
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
            resp = json.loads(urllib.request.urlopen(req, timeout=120).read())
        except urllib.error.HTTPError as e:
            resp = json.loads(e.read())
        if resp.get("error"):
            raise RpcError(resp["error"])
        return resp["result"]


def start_node(datadir, rpc):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", f"-ftforkactivationheight={FT_HEIGHT}",
            "-acceptnonstdtxn=0", "-allowunconnectedmining=1"]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(120):
        try:
            rpc("getblockchaininfo"); return node
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not start")


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-m12-")
    cookie = os.path.join(datadir, "regtest", ".cookie")
    rpc = Rpc(RPCPORT, cookie)
    node = start_node(datadir, rpc)

    cb_next = [10]

    def next_coinbase():
        h = cb_next[0]; cb_next[0] += 1
        cb = rpc("getblock", rpc("getblockhash", h), 2)["tx"][0]
        return cb["txid"], cb["vout"][0]["value"]

    def send_ft_genesis(splits):
        """splits: list of (ft_amount:int, dvt_value:float). category = the input coinbase txid.
        Returns (txid, category)."""
        cbid, cbval = next_coinbase()
        outs = [{rpc("getnewaddress"): {"amount": f"{dvt:.3f}",
                                        "tokenData": {"category": cbid, "amount": str(ft)}}}
                for ft, dvt in splits]
        spent = sum(dvt for _, dvt in splits)
        outs.append({rpc("getnewaddress"): f"{cbval - spent - 1.0:.3f}"})  # 1 DVT fee
        raw = rpc("createrawtransaction", [{"txid": cbid, "vout": 0}], outs)
        signed = rpc("signrawtransactionwithwallet", raw)
        assert signed.get("complete"), signed
        return rpc("sendrawtransaction", signed["hex"]), cbid

    def ft_vout(txid, category, amount):
        """Find the vout of txid carrying `amount` FT of `category`."""
        tx = rpc("getrawtransaction", txid, True)
        for v in tx["vout"]:
            td = v.get("tokenData")
            if td and td.get("category") == category and td.get("amount") == str(amount):
                return v["n"], v["value"]
        raise AssertionError(f"no vout with {amount} FT of {category[:12]} in {txid}")

    def spend_ft(txid, category, in_amount, out_amount):
        """Spend the FT output of `txid` carrying in_amount, producing out_amount (out<=in; a burn
        when out<in). Returns the new txid."""
        n, dvtval = ft_vout(txid, category, in_amount)
        out = {rpc("getnewaddress"): {"amount": f"{dvtval - 1.0:.3f}",
                                      "tokenData": {"category": category, "amount": str(out_amount)}}}
        raw = rpc("createrawtransaction", [{"txid": txid, "vout": n}], [out])
        signed = rpc("signrawtransactionwithwallet", raw)
        assert signed.get("complete"), signed
        return rpc("sendrawtransaction", signed["hex"])

    try:
        print(f"== setup: du1={DU1_HEIGHT}, ftfork={FT_HEIGHT}; mine to 259 (pre-fork)", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", 259, miner)
        check(rpc("getblockcount") == 259, "tip at 259 (pre-fork: next-block height 260 ≤ ftfork)")

        # ---- 1. PRE-fork: FT genesis rejected by the deferral gate ----
        print("== 1. pre-fork FT genesis rejected (deferral gate)", flush=True)
        try:
            send_ft_genesis([(1000, 5.0)])
            check(False, "pre-fork FT genesis should be rejected")
        except RpcError as e:
            check("bad-txns-token-ft-deferred" in e.message,
                  f"pre-fork FT genesis rejected by the gate: {e.message}")

        # ---- cross the fork: one more block → tip 260 (mempool next-height 261 → FT lifted) ----
        rpc("generatetoaddress", 1, miner)
        check(rpc("getblockcount") == 260, "tip at 260 (fork boundary; FT now lifts for the mempool)")

        # ---- 2. POST-fork: fixed-supply genesis (split) + transfer + burn ----
        print("== 2. post-fork FT genesis (600/400 split) + transfer + burn", flush=True)
        gtxid, cat = send_ft_genesis([(600, 5.0), (400, 5.0)])
        rpc("generatetoaddress", 1, miner)  # block 261 = first FT block
        check(rpc("getrawtransaction", gtxid, True).get("blockhash") is not None,
              "post-fork FT genesis accepted + mined")
        v600 = ft_vout(gtxid, cat, 600)
        v400 = ft_vout(gtxid, cat, 400)
        check(v600 and v400, "genesis created 600 + 400 FT across two outputs (full supply 1000)")

        xfer = spend_ft(gtxid, cat, 600, 600)  # conserve
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", xfer, True).get("blockhash") is not None
              and ft_vout(xfer, cat, 600), "FT transfer conserved 600 and mined")

        burn = spend_ft(gtxid, cat, 400, 250)  # burn 150
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", burn, True).get("blockhash") is not None
              and ft_vout(burn, cat, 250), "FT partial burn produced 250 (150 destroyed) and mined")

        # ---- 3. restart across the boundary: token state persists ----
        print("== 3. restart across the boundary", flush=True)
        tip = rpc("getbestblockhash")
        rpc("stop"); node.wait(timeout=60)
        node = start_node(datadir, rpc)
        check(rpc("getbestblockhash") == tip, "chain intact after restart")
        check(bool(ft_vout(burn, cat, 250)), "FT token output still present + correct after restart")

        # ---- 4. REORG across ftForkHeight (the 5A fix) ----
        print("== 4. reorg across ftForkHeight is safe (mempool purge + healthy GBT)", flush=True)
        rpc("generatetoaddress", 3, miner)  # tip ~266, comfortably post-fork
        fresh_txid, _ = send_ft_genesis([(777, 5.0)])
        check(fresh_txid in rpc("getrawmempool"), "a fresh FT genesis sits in the mempool (post-fork)")

        # Invalidate block 260 → tip 259 (below the fork). Disconnecting block 260 crosses the
        # boundary, so the mempool is re-evaluated and every now-invalid FT tx is dropped.
        hash260 = rpc("getblockhash", 260)
        rpc("invalidateblock", hash260)
        check(rpc("getblockcount") == 259, "reorged back to 259 (below ftForkHeight)")
        mempool = rpc("getrawmempool")
        check(fresh_txid not in mempool, "the mempool FT tx was purged by the boundary-crossing reorg")
        ft_left = [t for t in mempool if any(v.get("tokenData", {}).get("amount", "0") not in ("0", None)
                                             for v in rpc("getrawtransaction", t, True)["vout"])]
        check(not ft_left, f"no FT txs linger in the mempool after the reorg ({ft_left})")
        # The 4I #2 concern was a bricked template; assert GBT is healthy at the pre-fork tip.
        gbt = rpc("getblocktemplate")
        check(isinstance(gbt, dict) and gbt.get("height") == 260,
              "getblocktemplate is healthy after the reorg (height 260, no invalid FT tx)")
        # And a fresh FT genesis is now correctly REJECTED at the pre-fork tip.
        try:
            send_ft_genesis([(1, 5.0)])
            check(False, "FT genesis after reorg-below-fork should be rejected")
        except RpcError as e:
            check("bad-txns-token-ft-deferred" in e.message,
                  f"FT genesis rejected again below the fork: {e.message}")

        # Re-activate the fork (the other boundary direction) and confirm FT flows again.
        print("== 4b. re-activate the fork; FT flows again", flush=True)
        rpc("reconsiderblock", hash260)
        check(rpc("getblockcount") >= 261, "chain reconnected above the fork after reconsiderblock")
        again, _ = send_ft_genesis([(42, 5.0)])
        check(again in rpc("getrawmempool"), "FT genesis accepted again once the fork is re-active")

    finally:
        if node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=60)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"M12 RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("M12 RESULT: ALL PASS")


if __name__ == "__main__":
    main()
