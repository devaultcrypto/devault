#!/usr/bin/env python3
"""DeVault 4F — mempool limiting under jumbo-mint pressure.

One node with a deliberately tiny -maxmempool=5 (MB). Submitting 6 x ~990 KB mints must keep the
node healthy: dynamic usage stays under the cap (eviction and/or mempool-full rejection at the
rolling min fee — DeVault spock-scale feerate arithmetic), at least one mint survives, mining
clears the pool, and the node keeps accepting fresh transactions afterwards.
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
RPCPORT = 29891
DU1_HEIGHT = 200

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
        self.url = f"http://127.0.0.1:{port}"
        self.cookie = cookie

    def __call__(self, method, *params):
        auth = base64.b64encode(open(self.cookie, "rb").read()).decode()
        body = json.dumps({"id": 1, "method": method, "params": list(params)}).encode()
        req = urllib.request.Request(
            self.url, body, {"Authorization": "Basic " + auth, "Content-Type": "application/json"})
        try:
            resp = json.loads(urllib.request.urlopen(req, timeout=600).read())
        except urllib.error.HTTPError as e:
            resp = json.loads(e.read())
        if resp.get("error"):
            raise RpcError(resp["error"].get("message", ""))
        return resp["result"]


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-4f-mp-")
    node = subprocess.Popen(
        [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
         "-server=1", f"-du1activationheight={DU1_HEIGHT}", "-acceptnonstdtxn=0",
         "-excessiveblocksize=32000000", "-blockmaxsize=32000000",
         "-allowunconnectedmining=1", "-maxmempool=5"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))
    try:
        for _ in range(240):
            try:
                rpc("getblockchaininfo"); break
            except Exception:
                time.sleep(0.5)
        else:
            raise RuntimeError("node did not start")

        print("== setup: wallet + mine past DU1; -maxmempool=5MB", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", DU1_HEIGHT + 20, miner)
        info = rpc("getmempoolinfo")
        check(info["maxmempool"] == 5_000_000, f"maxmempool is 5MB ({info['maxmempool']})")

        print("== pressure: 6 x ~990KB mints into a 5MB pool", flush=True)
        accepted, rejected = 0, 0
        for i in range(6):
            try:
                rpc("mintnft", os.urandom(990_000).hex(), "application/octet-stream")
                accepted += 1
            except RpcError as e:
                rejected += 1
                print(f"  (mint {i}: rejected by the full mempool: {e.message.splitlines()[0]})",
                      flush=True)
            info = rpc("getmempoolinfo")
            check(info["usage"] <= info["maxmempool"],
                  f"dynamic usage within cap after mint {i} ({info['usage']:,}/{info['maxmempool']:,})")
        check(accepted >= 1, f"at least one mint made it in ({accepted} accepted)")
        check(rejected >= 1, f"pressure actually triggered limiting ({rejected} rejected/evicted-path)")
        surviving = len(rpc("getrawmempool"))
        check(0 < surviving < 6, f"mempool holds a bounded subset ({surviving}/6)")
        minfee = rpc("getmempoolinfo")["mempoolminfee"]
        check(minfee > 0, f"rolling mempool min fee rose ({minfee})")

        print("== recovery: mine, pool drains, normal txs flow again", flush=True)
        rpc("generatetoaddress", 1, miner)
        check(len(rpc("getrawmempool")) == 0, "block cleared the surviving mints")
        txid = rpc("sendtoaddress", rpc("getnewaddress"), 10)
        check(txid in rpc("getrawmempool"), "a fresh ordinary tx is accepted post-pressure")
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", txid, True).get("confirmations", 0) == 1, "and it mines")

    finally:
        if node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=60)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"4F-PRESSURE RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("4F-PRESSURE RESULT: ALL PASS")


if __name__ == "__main__":
    main()
