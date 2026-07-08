#!/usr/bin/env python3
"""4I review F2 — DnftIndex multi-block-same-category reorg.

A category is minted in TWO consecutive blocks (genesis + a later mint via its minting token).
Invalidating the genesis disconnects BOTH; BaseIndex then rewinds them in ONE WriteBlock batch.
Category records are read-modify-write, and writes buffered in the CDBBatch are NOT visible to
m_db->Read — so a per-block re-read corrupts a category touched by more than one block in the
batch. The fix threads one `cats` accumulator through the whole WriteBlock. This test asserts the
collection fully disappears after the reorg (pre-fix it would be left with a phantom minted=1).
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

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dvt_dnft_reference as ref
from dvt_m9_dnft_binding import mine_block

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29907
DU1 = 200
FAILURES = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


class RpcError(Exception):
    def __init__(self, msg):
        super().__init__(msg)
        self.message = msg


class Rpc:
    def __init__(self, port, cookie):
        self.url, self.cookie = f"http://127.0.0.1:{port}", cookie

    def __call__(self, method, *params):
        auth = base64.b64encode(open(self.cookie, "rb").read()).decode()
        body = json.dumps({"id": 1, "method": method, "params": list(params)}).encode()
        req = urllib.request.Request(self.url, body,
            {"Authorization": "Basic " + auth, "Content-Type": "application/json"})
        try:
            r = json.loads(urllib.request.urlopen(req, timeout=120).read())
        except urllib.error.HTTPError as e:
            r = json.loads(e.read())
        if r.get("error"):
            raise RpcError(r["error"].get("message", ""))
        return r["result"]


def poll(fn, timeout=20):
    end = time.time() + timeout
    last = None
    while time.time() < end:
        try:
            last = fn()
            if last:
                return last
        except RpcError as e:
            last = e
        time.sleep(0.2)
    return last


def collection_gone(rpc, cat):
    try:
        rpc("getnftcollection", cat)
        return False
    except RpcError as e:
        return "No DNFT collection" in e.message


def main():
    d = tempfile.mkdtemp(prefix="dvt-4i-reorg-")
    node = subprocess.Popen([DEVAULTD, "-regtest", f"-datadir={d}", f"-rpcport={RPCPORT}",
        "-listen=0", "-server=1", f"-du1activationheight={DU1}", "-acceptnonstdtxn=0",
        "-nftindex", "-allowunconnectedmining=1"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(d, "regtest", ".cookie"))
    try:
        for _ in range(120):
            try:
                rpc("getblockcount"); break
            except Exception:
                time.sleep(0.5)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", DU1 + 20, miner)

        def spk(a):
            return bytes.fromhex(rpc("validateaddress", a)["scriptPubKey"])

        def cb(h):
            c = rpc("getblock", rpc("getblockhash", h), 2)["tx"][0]
            return c["txid"], int(round(c["vout"][0]["value"] * 1e8))

        def send(u):
            s = rpc("signrawtransactionwithwallet", u)
            assert s["complete"], s
            return rpc("sendrawtransaction", s["hex"])

        print("== a collection minted across TWO consecutive blocks", flush=True)
        # block G: genesis — item A + minting baton
        g_in, g_val = cb(10)
        X = ref.category_internal_from_txid(g_in)
        envA = ref.build_envelope(content_type=b"text/plain", body=b"A")
        kA = ref.compute_commitment(envA, X, 0, 0)
        auth = rpc("getnewaddress")
        gtx = send(ref.ser_tx(2, [(g_in, 0, b"", 0xFFFFFFFF)], [
            ref.ser_output(10**8, ref.wrap_token_spk(X, ref.CAP_NONE, kA, None, spk(rpc("getnewaddress")))),
            ref.ser_output(10**8, ref.wrap_token_spk(X, ref.CAP_MINTING, None, None, spk(auth))),
            ref.ser_output(0, envA),
            ref.ser_output(g_val - 3 * 10**8, spk(miner)),
        ]).hex())
        rpc("generatetoaddress", 1, miner)
        genesis_block = rpc("getrawtransaction", gtx, True)["blockhash"]
        cat_disp = g_in

        # block G+1: later mint — item B via the baton (re-creating the baton)
        f_in, f_val = cb(11)
        envB = ref.build_envelope(content_type=b"text/plain", body=b"B")
        salt = ref.category_internal_from_txid(gtx)  # tx's own input0 = the baton outpoint (gtx,1)
        kB = ref.compute_commitment(envB, salt, 1, 0)
        ltx = send(ref.ser_tx(2, [(gtx, 1, b"", 0xFFFFFFFF), (f_in, 0, b"", 0xFFFFFFFF)], [
            ref.ser_output(10**8, ref.wrap_token_spk(X, ref.CAP_NONE, kB, None, spk(rpc("getnewaddress")))),
            ref.ser_output(10**8, ref.wrap_token_spk(X, ref.CAP_MINTING, None, None, spk(auth))),
            ref.ser_output(0, envB),
            ref.ser_output(f_val - 3 * 10**8, spk(miner)),
        ]).hex())
        rpc("generatetoaddress", 1, miner)

        coll = poll(lambda: (lambda c: c if c and c["minted"] == 2 else None)(rpc("getnftcollection", cat_disp)))
        check(isinstance(coll, dict) and coll["minted"] == 2 and coll["open"] is True,
              f"collection minted=2, open across two blocks ({coll})")

        # ---- invalidate the GENESIS -> disconnects BOTH blocks; index rewinds them in one batch ----
        print("== invalidate genesis (2-block rewind) + longer coinbase-only fork", flush=True)
        miner_script = spk(miner)
        rpc("invalidateblock", genesis_block)  # both mint txs return to the mempool
        for _ in range(3):  # a strictly longer fork, both mints excluded
            gbt = rpc("getblocktemplate")
            mp = rpc("getrawmempool", True)
            fees = sum(int(round(e["fees"]["base"] * 1e8)) for e in mp.values())
            gbt["coinbasevalue"] = gbt["coinbasevalue"] - fees
            res = rpc("submitblock", mine_block(gbt, miner_script, []))
            if res is not None:
                raise RpcError(f"fork submitblock rejected: {res}")

        gone = poll(lambda: "gone" if collection_gone(rpc, cat_disp) else None)
        check(gone == "gone",
              "after the 2-block rewind, the collection is FULLY gone (no phantom minted count)")

        # -reindex must reproduce the same (absent) state.
        print("== -reindex reproduces the post-reorg state", flush=True)
        tip = rpc("getbestblockhash")
        rpc("stop"); node.wait(timeout=60)
        node2 = subprocess.Popen([DEVAULTD, "-regtest", f"-datadir={d}", f"-rpcport={RPCPORT}",
            "-listen=0", "-server=1", f"-du1activationheight={DU1}", "-acceptnonstdtxn=0",
            "-nftindex", "-reindex", "-allowunconnectedmining=1"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        globals()["node"] = node2
        rpc2 = Rpc(RPCPORT, os.path.join(d, "regtest", ".cookie"))
        for _ in range(240):
            try:
                if rpc2("getbestblockhash") == tip:
                    break
            except Exception:
                pass
            time.sleep(0.5)
        check(collection_gone(rpc2, cat_disp), "reindexed index also has no phantom collection")
        rpc2("stop"); node2.wait(timeout=60)
        node = None
    finally:
        if node is not None and node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=30)
            except Exception:
                node.kill()
        shutil.rmtree(d, ignore_errors=True)

    print()
    if FAILURES:
        print(f"4I-INDEX-REORG RESULT: FAIL ({len(FAILURES)})")
        sys.exit(1)
    print("4I-INDEX-REORG RESULT: ALL PASS")


if __name__ == "__main__":
    main()
