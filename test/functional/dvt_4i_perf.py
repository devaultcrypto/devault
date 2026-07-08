#!/usr/bin/env python3
"""4I.3 — recorded performance numbers for the DNFT stack.

Builds a worst-case corpus (a ~32 MB block of 32 max-size mints on a 32 MB-configured regtest),
then records:
  P1  assemble+validate the ~32 MB block (generatetoaddress wall time)
  P2  full -reindex (IBD-equivalent revalidation) over the corpus, WITHOUT -nftindex
  P3  the same WITH -nftindex  ->  index overhead %
  P4  dnft-explorer ingestion over the same chain (blocks/s, MB/s)

Run on an otherwise idle machine; numbers go into the 4I report.
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
EXPLORER = os.environ.get("DNFT_EXPLORER_BIN",
                          "/home/pro/Git/devault-dev/dnft-explorer/target/release/dnft-explorer")
RPCPORT = 29901
DU1 = 200
BIG = ["-excessiveblocksize=32000000", "-blockmaxsize=32000000"]

RESULTS = []


def perf(name, value):
    RESULTS.append((name, value))
    print(f"  [PERF] {name}: {value}", flush=True)


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
            raise RuntimeError(resp["error"].get("message", ""))
        return resp["result"]


def start(datadir, extra=()):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1}", "-allowunconnectedmining=1", *BIG, *extra]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))
    return node, rpc


def wait_rpc(rpc, node, timeout=600):
    end = time.time() + timeout
    while time.time() < end:
        if node.poll() is not None:
            raise RuntimeError("node exited")
        try:
            rpc("getblockcount")
            return
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not come up")


def wait_height(rpc, node, height, timeout=1800):
    end = time.time() + timeout
    while time.time() < end:
        if node.poll() is not None:
            raise RuntimeError("node exited")
        try:
            if rpc("getblockcount") >= height:
                return
        except Exception:
            pass
        time.sleep(0.5)
    raise RuntimeError("timeout waiting for height")


def stop(rpc, node):
    try:
        rpc("stop")
        node.wait(timeout=120)
    except Exception:
        node.kill()


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-4i-perf-")
    node, rpc = start(datadir)
    try:
        print("== corpus: 32 x 990KB mints in one block", flush=True)
        wait_rpc(rpc, node)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", DU1 + 20, miner)
        t0 = time.monotonic()
        for i in range(32):
            rpc("mintnft", os.urandom(990_000).hex(), "application/octet-stream")
        perf("32 x mintnft 990KB (build+sign+ATMP each)", f"{time.monotonic() - t0:.2f}s total")

        t0 = time.monotonic()
        bh = rpc("generatetoaddress", 1, miner)[0]
        dt = time.monotonic() - t0
        size = rpc("getblock", bh, 1)["size"]
        perf(f"P1 assemble+validate {size:,}-byte block", f"{dt:.3f}s")
        height = rpc("getblockcount")
        stop(rpc, node)

        # P2: full -reindex without the index. Time from process launch — the reindex runs during
        # startup, so a post-RPC timer would miss it on a small chain.
        t0 = time.monotonic()
        node, rpc = start(datadir, ["-reindex"])
        wait_rpc(rpc, node)
        wait_height(rpc, node, height)
        p2 = time.monotonic() - t0
        perf("P2 -reindex incl. startup (no -nftindex)", f"{p2:.2f}s")
        stop(rpc, node)

        # P3: full -reindex with the index (same span).
        t0 = time.monotonic()
        node, rpc = start(datadir, ["-reindex", "-nftindex"])
        wait_rpc(rpc, node)
        wait_height(rpc, node, height)
        p3 = time.monotonic() - t0
        perf("P3 -reindex incl. startup (with -nftindex)", f"{p3:.2f}s")
        overhead = (p3 - p2) / p2 * 100 if p2 > 0 else 0
        perf("P3/P2 -nftindex overhead", f"{overhead:+.1f}%")

        # P4: explorer ingestion over the same live node
        indexdir = tempfile.mkdtemp(prefix="dvt-4i-explorer-")
        t0 = time.monotonic()
        proc = subprocess.run(
            [EXPLORER, "--chain", "regtest", "--rpc-url", f"http://127.0.0.1:{RPCPORT}",
             "--cookie-file", os.path.join(datadir, "regtest", ".cookie"),
             "--data-dir", indexdir, "--index-only"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=1800)
        p4 = time.monotonic() - t0
        blocks = height + 1
        mb = (size + blocks * 200) / 1e6
        perf("P4 explorer ingestion (fresh index)",
             f"{p4:.2f}s = {blocks / p4:.0f} blocks/s ≈ {mb / p4:.1f} MB/s (rc={proc.returncode})")
        shutil.rmtree(indexdir, ignore_errors=True)
        stop(rpc, node)
        node = None
    finally:
        if node is not None and node.poll() is None:
            stop(rpc, node)
        shutil.rmtree(datadir, ignore_errors=True)

    print("\n4I.3 PERF SUMMARY")
    for name, value in RESULTS:
        print(f"  {name}: {value}")


if __name__ == "__main__":
    main()
