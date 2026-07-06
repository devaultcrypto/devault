#!/usr/bin/env python3
"""DeVault 4D — wallet + core DNFT RPCs, functional lifecycle test.

Drives the new wallet RPCs (mintnft / listnfts / getnftinfo / sendnft / burnnft) and the raw-tx
`dnft` decode section over JSON-RPC, on a regtest node with DU1 active. Verifies the full
mint -> hold -> send -> burn lifecycle end to end, and the "no accidental sends" safety property
(a monetary sendtoaddress never selects an NFT UTXO).
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
RPCPORT = 29863
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
    def __init__(self, port, cookie, wallet=None):
        self.url = f"http://127.0.0.1:{port}" + (f"/wallet/{wallet}" if wallet else "")
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


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-4d-")
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", "-allowunconnectedmining=1"]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    cookie = os.path.join(datadir, "regtest", ".cookie")
    rpc = Rpc(RPCPORT, cookie)

    def wait_up():
        for _ in range(120):
            try:
                rpc("getblockchaininfo"); return
            except Exception:
                time.sleep(0.5)
        raise RuntimeError("node did not start")

    try:
        wait_up()
        print(f"== setup: wallet + mine past DU1 ({DU1_HEIGHT})", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", DU1_HEIGHT + 5, miner)

        # ---- pre-DU1 guard would fire below DU1; here DU1 is active. mintnft ----
        print("== mintnft (new collection)", flush=True)
        content = b"<svg>DNFT</svg>".hex()
        res = rpc("mintnft", content, "image/svg+xml")
        check(all(k in res for k in ("txid", "category", "commitment", "item_id")),
              f"mintnft returned txid/category/commitment/item_id ({list(res)})")
        rpc("generatetoaddress", 1, miner)
        category, commitment, txid = res["category"], res["commitment"], res["txid"]
        check(commitment.startswith("01") and len(commitment) == 66, "commitment is 0x01 + 32-byte hash")
        check(res["item_id"] == txid + "i0", "item_id is <txid>i0")

        conf = rpc("getrawtransaction", txid, True)
        check(conf.get("confirmations", 0) >= 1, "mint tx confirmed")
        # tokenData on the inscribed output + dnft decode section present
        td = conf["vout"][0].get("tokenData", {})
        check(td.get("category") == category and td.get("nft", {}).get("commitment") == commitment,
              "inscribed output carries the expected category+commitment")
        dnft = conf.get("dnft", [])
        check(len(dnft) == 1 and dnft[0]["valid"] and dnft[0]["content_type"] == "image/svg+xml",
              f"dnft decode section present and valid ({dnft})")

        # ---- listnfts / getnftinfo ----
        print("== listnfts / getnftinfo", flush=True)
        nfts = rpc("listnfts")
        check(len(nfts) == 1 and nfts[0]["category"] == category and nfts[0]["commitment"] == commitment,
              "listnfts shows the minted item")
        info = rpc("getnftinfo", category, commitment)
        check(info["category"] == category and info["commitment"] == commitment and "address" in info,
              "getnftinfo returns the item detail")

        # ---- wallet safety: a monetary send must not select the NFT UTXO ----
        print("== wallet safety (monetary send skips NFT UTXO)", flush=True)
        dest = rpc("getnewaddress")
        rpc("sendtoaddress", dest, 100)
        rpc("generatetoaddress", 1, miner)
        check(len(rpc("listnfts")) == 1, "NFT still held after a monetary sendtoaddress (not spent as money)")

        # ---- mintnft broadcast=false returns hex without broadcasting ----
        print("== mintnft broadcast=false", flush=True)
        res2 = rpc("mintnft", b"raw".hex(), "text/plain", {"broadcast": False})
        check("hex" in res2, "broadcast=false returns raw hex")
        # the unbroadcast mint is not in the mempool / chain
        try:
            rpc("getrawtransaction", res2["txid"])
            in_chain = True
        except RpcError:
            in_chain = False
        check(not in_chain, "unbroadcast mint is not on the node")
        # decoderawtransaction shows its dnft section
        dec = rpc("decoderawtransaction", res2["hex"])
        check(dec.get("dnft", [{}])[0].get("content_type") == "text/plain",
              "decoderawtransaction surfaces the dnft envelope")

        # ---- sendnft (transfer) ----
        print("== sendnft (transfer)", flush=True)
        recv = rpc("getnewaddress")
        send_res = rpc("sendnft", category, commitment, recv)
        rpc("generatetoaddress", 1, miner)
        moved = rpc("getrawtransaction", send_res["txid"], True)
        out0 = moved["vout"][0]
        check(out0["tokenData"]["nft"]["commitment"] == commitment
              and out0["scriptPubKey"]["addresses"][0] == recv,
              "sendnft moved the NFT to the destination, commitment preserved")
        # still owned by the (single) wallet, now at the new address; listnfts still shows 1
        check(len(rpc("listnfts")) == 1, "wallet still holds the (moved) NFT")

        # ---- burnnft ----
        print("== burnnft", flush=True)
        burn_res = rpc("burnnft", category, commitment)
        rpc("generatetoaddress", 1, miner)
        burned = rpc("getrawtransaction", burn_res["txid"], True)
        check(all("tokenData" not in v for v in burned["vout"]),
              "burn tx has no token outputs (NFT destroyed)")
        check(len(rpc("listnfts")) == 0, "NFT no longer held after burn")
        # getnftinfo now errors
        try:
            rpc("getnftinfo", category, commitment)
            check(False, "getnftinfo should error for a burned NFT")
        except RpcError as e:
            check("No spendable NFT" in e.message, "getnftinfo errors for the burned NFT")

    finally:
        try:
            rpc("stop"); node.wait(timeout=30)
        except Exception:
            node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"4D RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("4D RESULT: ALL PASS")


if __name__ == "__main__":
    main()
