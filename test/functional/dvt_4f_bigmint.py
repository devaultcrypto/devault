#!/usr/bin/env python3
"""DeVault 4F — policy/relay/mining with big DNFT payloads, functional E2E.

Two connected regtest nodes, BOTH with -acceptnonstdtxn=0 (the STANDARD relay path — the whole
point of 4F). Verifies:
  * plain-OP_RETURN policy unchanged: >223 B non-envelope OP_RETURN still rejects (Q15 keeps the cap)
  * an envelope with no mint is consensus-rejected (the policy exemption opens no spam path)
  * a full 990 KB mint: mintnft -> relays node0 -> node1 -> mined -> node1's copy hash-matches the
    source bytes exactly; decoded vout type is "dnftenvelope"; 4E index serves it (A4 + Q15 E2E)
  * sustained 10-block multi-mint run (3 x ~300 KB per block)
  * one ~8 MB block of 8 max-size mints (mining + validation at scale)
  * mempool.dat persistence of a big mint across restart
  * node1 full -reindex over the token corpus; perf numbers recorded throughout
"""
import base64
import hashlib
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

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPC0, RPC1, P2P1 = 29877, 29879, 29880
DU1_HEIGHT = 200

FAILURES = []
PERF = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


def perf(name, seconds):
    PERF.append((name, seconds))
    print(f"  [PERF] {name}: {seconds:.3f}s", flush=True)


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


COMMON_ARGS = ["-regtest", "-server=1", f"-du1activationheight={DU1_HEIGHT}",
               "-acceptnonstdtxn=0",        # force the STANDARD relay path (the 4F subject)
               "-excessiveblocksize=32000000", "-blockmaxsize=32000000",
               "-allowunconnectedmining=1"]


def start_node(datadir, rpcport, extra=()):
    args = [DEVAULTD, f"-datadir={datadir}", f"-rpcport={rpcport}", *COMMON_ARGS, *extra]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(rpcport, os.path.join(datadir, "regtest", ".cookie"))
    for _ in range(240):
        try:
            rpc("getblockchaininfo"); return node, rpc
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not start")


def poll(fn, timeout=60, step=0.05):
    end = time.time() + timeout
    last = None
    while time.time() < end:
        try:
            last = fn()
            if last:
                return last
        except RpcError as e:
            last = e
        time.sleep(step)
    return last


def wait_sync(rpc0, rpc1, timeout=60):
    """Wait until node1's tip equals node0's; return the wait in seconds."""
    tip = rpc0("getbestblockhash")
    t0 = time.monotonic()
    ok = poll(lambda: rpc1("getbestblockhash") == tip, timeout=timeout)
    if not ok:
        raise RuntimeError("nodes failed to sync")
    return time.monotonic() - t0


def find_envelope_vout(txverbose):
    for v in txverbose["vout"]:
        spk = bytes.fromhex(v["scriptPubKey"]["hex"])
        if ref.is_dnft_envelope(spk):
            return v, spk
    return None, None


def main():
    dir0 = tempfile.mkdtemp(prefix="dvt-4f-n0-")
    dir1 = tempfile.mkdtemp(prefix="dvt-4f-n1-")
    node1, rpc1 = start_node(dir1, RPC1, ["-listen=1", "-bind=127.0.0.1", f"-port={P2P1}", "-nftindex"])
    node0, rpc0 = start_node(dir0, RPC0, ["-listen=0", f"-connect=127.0.0.1:{P2P1}"])

    try:
        print("== setup: connect, wallet, mine past DU1", flush=True)
        check(poll(lambda: rpc0("getconnectioncount") >= 1) and poll(lambda: rpc1("getconnectioncount") >= 1),
              "nodes are connected")
        rpc0("createwallet", "")
        miner = rpc0("getnewaddress")
        rpc0("generatetoaddress", DU1_HEIGHT + 20, miner)
        wait_sync(rpc0, rpc1)

        def coinbase_at(h):
            cb = rpc0("getblock", rpc0("getblockhash", h), 2)["tx"][0]
            return cb["txid"], int(round(cb["vout"][0]["value"] * 1e8))

        def addr_spk(a):
            return bytes.fromhex(rpc0("validateaddress", a)["scriptPubKey"])

        # ---- policy unchanged for non-envelope data + no unpaired-envelope path ----
        print("== adversarial: plain-OP_RETURN cap intact; unpaired envelope rejected", flush=True)
        cb_txid, cb_val = coinbase_at(10)
        big_opreturn = b"\x6a" + ref.push(b"\x42" * 300)  # >223 B, no DNFT magic
        raw = ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                         [ref.ser_output(0, big_opreturn),
                          ref.ser_output(cb_val - 10**8, addr_spk(miner))]).hex()
        signed = rpc0("signrawtransactionwithwallet", raw)["hex"]
        try:
            rpc0("sendrawtransaction", signed)
            check(False, "300B plain OP_RETURN must NOT relay post-DU1")
        except RpcError as e:
            check("oversize-op-return" in e.message,
                  f"300B plain OP_RETURN rejected: {e.message}")

        cb_txid, cb_val = coinbase_at(11)
        orphan_env = ref.build_envelope(content_type=b"text/plain", body=b"orphan")
        raw = ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)],
                         [ref.ser_output(0, orphan_env),
                          ref.ser_output(cb_val - 10**8, addr_spk(miner))]).hex()
        signed = rpc0("signrawtransactionwithwallet", raw)["hex"]
        try:
            rpc0("sendrawtransaction", signed)
            check(False, "an envelope without a mint must NOT be accepted")
        except RpcError as e:
            check("envelope-unclaimed" in e.message,
                  f"unpaired envelope rejected by consensus: {e.message}")

        # ---- the headline case: a full 990 KB mint, E2E ----
        print("== 990 KB mint: mintnft -> relay -> mine -> byte-exact on node1", flush=True)
        content = os.urandom(990_000)
        t0 = time.monotonic()
        m = rpc0("mintnft", content.hex(), "application/octet-stream")
        t_mint = time.monotonic() - t0
        perf("mintnft 990KB (build+sign+ATMP+broadcast)", t_mint)
        size0 = rpc0("getmempoolentry", m["txid"])["size"]
        check(size0 > 900_000, f"mint tx is big ({size0} bytes) — impossible pre-4F (cap was 100KB)")

        t0 = time.monotonic()
        relayed = poll(lambda: m["txid"] in rpc1("getrawmempool"), timeout=60)
        perf("relay 990KB mint node0->node1", time.monotonic() - t0)
        check(bool(relayed), "990KB mint relayed to node1 under -acceptnonstdtxn=0")

        t0 = time.monotonic()
        rpc0("generatetoaddress", 1, miner)
        dt = wait_sync(rpc0, rpc1)
        perf("single-mint block: node1 accept+connect", dt)

        txv = rpc1("getrawtransaction", m["txid"], True)
        check(txv.get("blockhash") is not None, "mint mined and visible on node1")
        env_vout, env_spk = find_envelope_vout(txv)
        parsed = ref.parse_envelope(env_spk) if env_spk else None
        check(parsed is not None and parsed.valid and
              hashlib.sha256(parsed.body).digest() == hashlib.sha256(content).digest() and
              len(parsed.body) == len(content),
              "node1's envelope body is byte-exact (SHA256 match, 990,000 bytes)")
        check(env_vout is not None and env_vout["scriptPubKey"]["type"] == "dnftenvelope",
              f"decoded vout type is 'dnftenvelope' ({env_vout['scriptPubKey']['type']})")
        item = poll(lambda: rpc1("getnftitem", m["category"], m["commitment"]))
        check(isinstance(item, dict) and item["content_length"] == 990_000
              and item["still_at_mint_outpoint"] is True,
              f"4E index on node1 serves the item (content_length={item.get('content_length')})")

        # ---- sustained multi-mint run: 10 blocks x 3 mints of ~300 KB ----
        print("== sustained: 10 blocks x 3 x ~300KB mints", flush=True)
        sync_times = []
        minted = []
        for b in range(10):
            for _ in range(3):
                minted.append(rpc0("mintnft", os.urandom(300_000).hex(), "application/octet-stream"))
            rpc0("generatetoaddress", 1, miner)
            sync_times.append(wait_sync(rpc0, rpc1))
        perf("10-block multi-mint run: avg node1 accept", sum(sync_times) / len(sync_times))
        perf("10-block multi-mint run: max node1 accept", max(sync_times))
        confirmed = sum(1 for x in minted if rpc1("getrawtransaction", x["txid"], True).get("blockhash"))
        check(confirmed == 30, f"all 30 sustained-run mints confirmed on node1 ({confirmed}/30)")
        ok_idx = sum(1 for x in minted if isinstance(
            poll(lambda x=x: rpc1("getnftitem", x["category"], x["commitment"])), dict))
        check(ok_idx == 30, f"all 30 items served by node1's index ({ok_idx}/30)")

        # ---- one ~8 MB block: 8 max-size mints ----
        print("== ~8MB block: 8 x 990KB mints in one block", flush=True)
        big8 = [rpc0("mintnft", os.urandom(990_000).hex(), "application/octet-stream")
                for _ in range(8)]
        check(all(x["txid"] in rpc0("getrawmempool") for x in big8), "8 big mints in node0 mempool")
        t0 = time.monotonic()
        bh = rpc0("generatetoaddress", 1, miner)[0]
        t_mine = time.monotonic() - t0
        blk = rpc0("getblock", bh, 1)
        check(blk["size"] > 7_500_000 and all(x["txid"] in blk["tx"] for x in big8),
              f"all 8 mints in one {blk['size']:,}-byte block")
        perf("assemble+validate ~8MB block (node0)", t_mine)
        perf("~8MB block: node1 accept+connect", wait_sync(rpc0, rpc1, timeout=120))

        # ---- mempool.dat persistence of a big mint across restart ----
        print("== mempool persistence across restart (big mint)", flush=True)
        keep = rpc0("mintnft", os.urandom(500_000).hex(), "application/octet-stream")
        check(keep["txid"] in rpc0("getrawmempool"), "500KB mint in mempool pre-restart")
        rpc0("stop"); node0.wait(timeout=120)
        node0, rpc0 = start_node(dir0, RPC0, ["-listen=0", f"-connect=127.0.0.1:{P2P1}"])
        back = poll(lambda: keep["txid"] in rpc0("getrawmempool"), timeout=120, step=0.5)
        check(bool(back), "big mint reloaded from mempool.dat (ATMP re-run at load)")
        rpc0("generatetoaddress", 1, miner)
        wait_sync(rpc0, rpc1)
        check(rpc1("getrawtransaction", keep["txid"], True).get("blockhash") is not None,
              "persisted mint mined + synced")

        total_items = len(rpc0("listnfts"))
        check(total_items == 40, f"wallet holds all 40 minted items ({total_items})")

        # ---- node1 full -reindex over the token corpus ----
        print("== node1 -reindex over the corpus", flush=True)
        tip = rpc1("getbestblockhash")
        height = rpc1("getblockchaininfo")["blocks"]
        rpc1("stop"); node1.wait(timeout=120)
        t0 = time.monotonic()  # spans startup: the reindex mostly completes during node warmup
        node1, rpc1 = start_node(dir1, RPC1, ["-listen=1", "-bind=127.0.0.1", f"-port={P2P1}",
                                              "-nftindex", "-reindex"])
        ok = poll(lambda: rpc1("getbestblockhash") == tip, timeout=600, step=0.5)
        perf(f"node1 -reindex to tip incl. startup (height {height}, ~29MB corpus)", time.monotonic() - t0)
        check(bool(ok), "node1 reindexed the token corpus to the same tip")
        item = poll(lambda: rpc1("getnftitem", m["category"], m["commitment"]), timeout=60)
        check(isinstance(item, dict) and item["content_length"] == 990_000,
              "990KB item still served after reindex")

    finally:
        for n, r in ((node0, Rpc(RPC0, os.path.join(dir0, "regtest", ".cookie"))),
                     (node1, Rpc(RPC1, os.path.join(dir1, "regtest", ".cookie")))):
            if n is not None and n.poll() is None:
                try:
                    r("stop"); n.wait(timeout=60)
                except Exception:
                    n.kill()
        shutil.rmtree(dir0, ignore_errors=True)
        shutil.rmtree(dir1, ignore_errors=True)

    print("\nPERF SUMMARY", flush=True)
    for name, secs in PERF:
        print(f"  {name}: {secs:.3f}s")
    print()
    if FAILURES:
        print(f"4F RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("4F RESULT: ALL PASS")


if __name__ == "__main__":
    main()
