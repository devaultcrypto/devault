#!/usr/bin/env python3
"""DeVault 4E — the -nftindex + getnftcollection/getnftitem RPCs, functional test.

Verifies the node-side DNFT index: item + collection records, minted counts, open/closed derived
from the live UTXO set (minting-token spend), reorg rollback (invalidate + fork), and a full
-reindex rebuild reproducing the same answers.
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
from dvt_m9_dnft_binding import mine_block  # coinbase-block builder (submitblock path)

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29877
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
            resp = json.loads(urllib.request.urlopen(req, timeout=120).read())
        except urllib.error.HTTPError as e:
            resp = json.loads(e.read())
        if resp.get("error"):
            raise RpcError(resp["error"].get("message", ""))
        return resp["result"]


def start_node(datadir, extra=()):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", "-allowunconnectedmining=1",
            "-nftindex", *extra]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))
    for _ in range(120):
        try:
            rpc("getblockchaininfo"); return node, rpc
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not start")


def poll(fn, timeout=20):
    """Retry fn() until it returns truthy (index callbacks are async)."""
    end = time.time() + timeout
    last = None
    while time.time() < end:
        try:
            last = fn()
            if last:
                return last
        except RpcError as e:
            last = e
        time.sleep(0.3)
    return last


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-4e-")
    node, rpc = start_node(datadir)

    def coinbase_at(h):
        cb = rpc("getblock", rpc("getblockhash", h), 2)["tx"][0]
        return cb["txid"], int(round(cb["vout"][0]["value"] * 1e8))

    def addr_spk(a):
        return bytes.fromhex(rpc("validateaddress", a)["scriptPubKey"])

    try:
        print(f"== setup: wallet + mine past DU1 ({DU1_HEIGHT})", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", DU1_HEIGHT + 20, miner)
        cb_next = [10]

        def next_coinbase():
            h = cb_next[0]; cb_next[0] += 1
            return coinbase_at(h)

        # ---- a minting-token collection (raw tx): 1 item + an open minting authority ----
        # (done first, at an earlier height, so it survives the later reorg of the 1/1 mint)
        print("== minting-token collection (open -> closed)", flush=True)
        cb_txid, cb_val = next_coinbase()
        mcat_disp = cb_txid
        mcat = ref.category_internal_from_txid(cb_txid)
        env = ref.build_envelope(content_type=b"text/plain", body=b"collection item")
        item_commit = ref.compute_commitment(env, mcat, 0, 0)  # item at vout 0, input0=(cb,0)
        auth_addr = rpc("getnewaddress")
        # vout0 inscribed item, vout1 minting token (authority), vout2 envelope, vout3 change
        outs = [
            ref.ser_output(10**8, ref.wrap_token_spk(mcat, ref.CAP_NONE, item_commit, None, addr_spk(rpc("getnewaddress")))),
            ref.ser_output(10**8, ref.wrap_token_spk(mcat, ref.CAP_MINTING, None, None, addr_spk(auth_addr))),
            ref.ser_output(0, env),
            ref.ser_output(cb_val - 3 * 10**8, addr_spk(miner)),
        ]
        unsigned = ref.ser_tx(2, [(cb_txid, 0, b"", 0xFFFFFFFF)], outs).hex()
        signed = rpc("signrawtransactionwithwallet", unsigned)
        gtxid = rpc("sendrawtransaction", signed["hex"])
        rpc("generatetoaddress", 1, miner)

        coll = poll(lambda: (lambda c: c if c and c["minted"] == 1 else None)(rpc("getnftcollection", mcat_disp)))
        check(isinstance(coll, dict) and coll["minted"] == 1 and coll["open"] is True
              and len(coll["minting_outpoints"]) == 1
              and coll["minting_outpoints"][0] == {"txid": gtxid, "vout": 1},
              f"minting collection: minted=1, OPEN, authority utxo listed ({coll})")

        # spend (burn) the minting token -> collection closes. Fund the fee from a coinbase so the
        # rescued output clears the ~0.6 DVT dust floor.
        fee_txid, fee_val = next_coinbase()
        burn = ref.ser_tx(2, [(gtxid, 1, b"", 0xFFFFFFFF), (fee_txid, 0, b"", 0xFFFFFFFF)],
                          [ref.ser_output(10**8, addr_spk(rpc("getnewaddress"))),
                           ref.ser_output(fee_val - 10**8, addr_spk(miner))]).hex()
        burn_signed = rpc("signrawtransactionwithwallet", burn)
        rpc("sendrawtransaction", burn_signed["hex"])
        rpc("generatetoaddress", 1, miner)
        coll2 = poll(lambda: (lambda c: c if c and c["open"] is False else None)(rpc("getnftcollection", mcat_disp)))
        check(isinstance(coll2, dict) and coll2["open"] is False and coll2["minted"] == 1,
              f"minting token spent -> collection CLOSED, minted still 1 ({coll2})")

        # ---- simple 1/1 mint via mintnft (the LAST DNFT event, so reorging it leaves the
        #      earlier minting collection intact) ----
        print("== 1/1 mint -> getnftcollection / getnftitem", flush=True)
        m = rpc("mintnft", b"<svg/>".hex(), "image/svg+xml")
        rpc("generatetoaddress", 1, miner)  # this block (the tip) is the 1/1 mint block
        cat, commit = m["category"], m["commitment"]
        coll = poll(lambda: rpc("getnftcollection", cat))
        check(isinstance(coll, dict) and coll["minted"] == 1 and coll["open"] is False
              and coll["genesis_txid"] == m["txid"],
              f"getnftcollection: minted=1, closed (1/1), genesis={m['txid'][:12]} ({coll})")
        item = rpc("getnftitem", cat, commit)
        check(item["content_type"] == "image/svg+xml" and item["mint_txid"] == m["txid"]
              and item["still_at_mint_outpoint"] is True and item["item_id"] == m["txid"] + "i0",
              f"getnftitem: content_type + mint tx + live ({item})")

        # ---- reorg: invalidate the (tip) 1/1 mint block, then build a LONGER fork of coinbase-only
        #      blocks (via submitblock) that EXCLUDE the returned-to-mempool mint tx, so the item is
        #      truly reorged out. The earlier minting collection survives. ----
        print("== reorg rollback (invalidate the tip mint + coinbase-only fork)", flush=True)
        miner_script = addr_spk(miner)
        mint_block = rpc("getrawtransaction", m["txid"], True)["blockhash"]
        rpc("invalidateblock", mint_block)  # tip -> parent; mint tx returns to the mempool
        for _ in range(2):                  # fork longer than the invalidated branch, mint excluded
            gbt = rpc("getblocktemplate")
            # coinbasevalue assumes the mempool txs are included; our coinbase-only block excludes
            # them, so pay just the subsidy (coinbasevalue minus the pending mempool fees).
            mempool = rpc("getrawmempool", True)
            fees = sum(int(round(e["fees"]["base"] * 1e8)) for e in mempool.values())
            gbt["coinbasevalue"] = gbt["coinbasevalue"] - fees
            res = rpc("submitblock", mine_block(gbt, miner_script, []))
            if res is not None:
                raise RpcError(f"fork submitblock rejected: {res}")
        gone = poll(lambda: "gone" if _collection_absent(rpc, cat) else None)
        check(gone == "gone", "after invalidate+fork, the reorged-out 1/1 item's collection is gone from the index")
        check(rpc("getnftcollection", mcat_disp)["minted"] == 1,
              "the earlier minting collection survives the reorg")

        # snapshot the index state for the reindex comparison (the surviving minting collection)
        snapshot = rpc("getnftcollection", mcat_disp)
        snap_item = rpc("getnftitem", mcat_disp, item_commit.hex())

        # ---- rebuild: -reindex wipes and rebuilds the index; answers must match ----
        print("== -reindex rebuild reproduces the index", flush=True)
        tip = rpc("getbestblockhash")
        rpc("stop"); node.wait(timeout=60)
        node2, rpc2 = start_node_reindex(datadir)
        try:
            for _ in range(240):
                try:
                    if rpc2("getbestblockhash") == tip and rpc2("getblockchaininfo").get("blocks"):
                        break
                except RpcError:
                    pass
                time.sleep(0.5)
            rebuilt = poll(lambda: (lambda c: c if c and c["minted"] == snapshot["minted"] else None)(
                rpc2("getnftcollection", mcat_disp)), timeout=60)
            check(isinstance(rebuilt, dict) and rebuilt == snapshot,
                  "reindexed getnftcollection identical to the pre-reindex snapshot")
            rebuilt_item = rpc2("getnftitem", mcat_disp, item_commit.hex())
            check(rebuilt_item == snap_item, "reindexed getnftitem identical to the snapshot")
            # the reorged-out 1/1 item is absent after reindex too
            check(_collection_absent(rpc2, cat), "reorged-out item absent in the rebuilt index")
            rpc2("stop"); node2.wait(timeout=60)
        finally:
            if node2.poll() is None:
                node2.kill()
        node = None

    finally:
        if node is not None and node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=30)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"4E RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("4E RESULT: ALL PASS")


def _collection_absent(rpc, category):
    try:
        rpc("getnftcollection", category)
        return False
    except RpcError as e:
        return "No DNFT collection" in e.message


def start_node_reindex(datadir):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", "-allowunconnectedmining=1",
            "-nftindex", "-reindex"]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))
    for _ in range(240):
        try:
            rpc("getblockchaininfo"); return node, rpc
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("reindex node did not start")


if __name__ == "__main__":
    main()
