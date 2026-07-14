#!/usr/bin/env python3
"""DeVault 5D — the fungible-token wallet RPCs, functional test.

Exercises deployft / mintft / sendft / getftinfo / listfttokens / getftbalance end to end, plus the
two hazards the RPC layer must protect the user from:

  * The `start_height` foot-gun. Consensus requires start > deploy_height (spec O4), but the
    deployer cannot know the height their deploy will be mined at. deployft therefore defaults to a
    safety margin and REJECTS a start that could never be valid.
  * Funding must never pull token-bearing coins in as fee inputs — that would silently change a
    mint's net creation away from exactly Q and make it consensus-invalid. (A default CCoinControl
    has m_allow_tokens=false; this test proves it holds even when the wallet is full of tokens.)

Also checks the DVFT decode section in getrawtransaction/decoderawtransaction.
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
RPCPORT = 29781
DU1_HEIGHT = 200
FT_HEIGHT = 260

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


def start_node(datadir, rpc):
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}", "-listen=0",
            "-server=1", f"-du1activationheight={DU1_HEIGHT}", f"-ftforkactivationheight={FT_HEIGHT}",
            "-acceptnonstdtxn=0", "-allowunconnectedmining=1", "-txindex"]
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(240):
        try:
            rpc("getblockchaininfo"); return node
        except Exception:
            time.sleep(0.5)
    raise RuntimeError("node did not start")


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-5d-")
    rpc = Rpc(RPCPORT, os.path.join(datadir, "regtest", ".cookie"))
    node = start_node(datadir, rpc)

    try:
        print("== setup: wallet, mine past the FT fork", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", 300, miner)

        # ---- pre-flight: the start_height guard ----
        print("== 1. deployft: the start_height foot-gun is guarded", flush=True)
        tip = rpc("getblockcount")
        for bad in (tip, tip + 1):
            try:
                rpc("deployft", "BAD", "Bad", 2, "open",
                    {"quantity": 100, "per_block_limit": 2, "max_mints": 10, "start_height": bad})
                check(False, f"deployft with start_height={bad} should be rejected")
            except RpcError as e:
                check("start_height must be at least" in e.message,
                      f"start_height={bad} (<= tip+1) rejected up front: {e.message[:60]}")

        # ---- 2. open-mint deploy (default start margin) ----
        print("== 2. deployft open + getftinfo", flush=True)
        dep = rpc("deployft", "GOLD", "Gold Token", 2, "open",
                  {"quantity": 100, "per_block_limit": 2, "max_mints": 5, "premine": 500})
        check(dep["mode"] == "open" and dep["quantity"] == 100 and dep["max_mints"] == 5
              and dep["premine"] == 500,
              f"deployft returned the open params (start_height={dep['start_height']})")
        deploy_txid, category = dep["deploy_txid"], dep["category"]
        check(dep["start_height"] >= tip + 2, "the default start height leaves a safety margin")
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", deploy_txid, True).get("blockhash") is not None,
              "deploy confirmed")

        # the premine landed in this wallet
        check(rpc("getftbalance", category) == 500, "getftbalance shows the 500-token premine")
        lst = rpc("listfttokens")
        check(any(e["category"] == category and e["amount"] == 500 for e in lst),
              f"listfttokens reports the premine ({lst})")

        info = rpc("getftinfo", deploy_txid)
        check(info["mode"] == "open" and info["symbol"] == "GOLD" and info["name"] == "Gold Token"
              and info["decimals"] == 2 and info["max_supply"] == 500 + 5 * 100,
              f"getftinfo: registry params + symbol/name from the deploy tx (max_supply="
              f"{info['max_supply']})")
        check(info["window_open"] is False,
              "the window is not open yet (start height is still ahead)")

        # ---- 3. mintft before the window opens is refused with a clear error ----
        try:
            rpc("mintft", deploy_txid)
            check(False, "mintft before the window opens should be refused")
        except RpcError as e:
            check("not open" in e.message,
                  f"mintft refuses before the window opens: {e.message[:60]}")

        # ---- 4. mint through the window; the wallet is FULL of tokens the whole time ----
        print("== 3. mintft: exactly Q each; funding never touches token coins", flush=True)
        while rpc("getblockcount") + 1 < info["start_height"]:
            rpc("generatetoaddress", 1, miner)
        st = rpc("getftinfo", deploy_txid)
        check(st["window_open"] is True and st["allowance_next_block"] == 2,
              f"the window is open; the next block allows {st['allowance_next_block']} mints")

        minted_total = 0
        for expected in (2, 2, 1):  # N=5, M=2 -> the stateless schedule allows 2, 2, 1
            got = rpc("mintft", deploy_txid, expected)
            check(len(got["txids"]) == expected and got["quantity_each"] == 100,
                  f"mintft built {expected} mint tx(s) of 100 tokens each")
            rpc("generatetoaddress", 1, miner)
            for t in got["txids"]:
                check(rpc("getrawtransaction", t, True).get("blockhash") is not None,
                      f"mint {t[:10]} confirmed")
            minted_total += expected * 100

        # This is the hazard check: every mint above was funded while the wallet already held
        # hundreds of GOLD tokens. If coin selection had grabbed a GOLD coin as a fee input, the
        # net creation would not have been exactly Q and consensus would have rejected the mint.
        check(minted_total == 500, "all 5 scheduled mints confirmed (funding never took a token coin)")
        check(rpc("getftbalance", category) == 500 + 500,
              "balance = premine + minted (500 + 500)")

        # ---- 5. the cap is exhausted; mintft says so clearly ----
        st = rpc("getftinfo", deploy_txid)
        check(st["window_open"] is False, "after N=5 mints the window is closed")
        try:
            rpc("mintft", deploy_txid)
            check(False, "mintft past the cap should be refused")
        except RpcError as e:
            check("not open" in e.message, f"mintft refuses past the cap: {e.message[:60]}")

        # ---- 6. sendft: transfer + token change ----
        print("== 4. sendft (transfer + token change)", flush=True)
        dest = rpc("getnewaddress")
        before = rpc("getftbalance", category)
        snd = rpc("sendft", category, 250, dest)
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", snd["txid"], True).get("blockhash") is not None,
              "sendft confirmed")
        # the destination is our own wallet, so the balance is unchanged but the UTXOs were reshaped
        check(rpc("getftbalance", category) == before,
              f"conservation: total balance unchanged after an internal send ({before})")

        # sending more than we hold is refused
        try:
            rpc("sendft", category, before + 1, dest)
            check(False, "sendft beyond the balance should be refused")
        except RpcError as e:
            check("Insufficient token balance" in e.message,
                  f"sendft refuses to overspend: {e.message[:60]}")

        # ---- 7. fixed-supply deploy ----
        print("== 5. deployft fixed", flush=True)
        fx = rpc("deployft", "USD", "US Dollar", 8, "fixed", {"supply": 21000000})
        rpc("generatetoaddress", 1, miner)
        check(rpc("getrawtransaction", fx["deploy_txid"], True).get("blockhash") is not None,
              "fixed-supply deploy confirmed")
        check(rpc("getftbalance", fx["category"]) == 21000000,
              "the whole fixed supply is in the wallet")
        fi = rpc("getftinfo", fx["deploy_txid"])
        check(fi["mode"] == "fixed" and fi["symbol"] == "USD",
              "getftinfo reports a fixed deploy (not registered — it can never be minted)")
        # ...and a fixed token can never be minted
        try:
            rpc("mintft", fx["deploy_txid"])
            check(False, "minting a fixed-supply token should be refused")
        except RpcError as e:
            check("No open-mint deploy is registered" in e.message,
                  f"mintft refuses a fixed token: {e.message[:60]}")

        # ---- 8. the DVFT decode section ----
        print("== 6. decode section", flush=True)
        d = rpc("getrawtransaction", deploy_txid, True)
        dft = d.get("dft")
        check(isinstance(dft, list) and dft[0]["type"] == "deploy" and dft[0]["symbol"] == "GOLD"
              and dft[0]["mode"] == "open" and dft[0]["valid"] is True,
              f"getrawtransaction decodes the DVFT deploy envelope ({dft})")
        m = rpc("getrawtransaction", got["txids"][0], True)
        mdft = m.get("dft")
        check(isinstance(mdft, list) and mdft[0]["type"] == "mint"
              and mdft[0]["deploy_txid"] == deploy_txid,
              f"the mint marker decodes and names the deploy txid ({mdft})")

        # ---- 8b. the raw path: broadcast=false hands back a signed, self-consistent tx ----
        raw = rpc("deployft", "RAW", "Raw Path", 0, "fixed",
                  {"supply": 7, "broadcast": False})
        check("hex" in raw and raw.get("txid") not in rpc("getrawmempool"),
              "deployft broadcast=false returns the hex WITHOUT broadcasting")
        dec = rpc("decoderawtransaction", raw["hex"])
        check(dec["txid"] == raw["txid"] and dec["dft"][0]["symbol"] == "RAW",
              "the unbroadcast deploy decodes as a DVFT deploy (decoderawtransaction)")
        # The category is derived from vin[0].prevout, so it must be predictable BEFORE broadcast.
        check(dec["vin"][0]["txid"] == raw["category"] and dec["vin"][0]["vout"] == 0,
              "the returned category == the (txid, 0) genesis prevout of vin[0]")
        # ...and it is a valid transaction the network will take.
        sent = rpc("sendrawtransaction", raw["hex"])
        rpc("generatetoaddress", 1, miner)
        check(sent == raw["txid"] and rpc("getftbalance", raw["category"]) == 7,
              "the hex relays and confirms unchanged (supply lands in the wallet)")

        # ---- 9. wallet safety: an ordinary DVT send must not spend token coins ----
        print("== 7. wallet safety: a plain DVT send skips token UTXOs", flush=True)
        utxos_before = {(u["txid"], u["vout"]) for u in rpc("listunspent")
                        if u.get("tokenData")}
        plain = rpc("sendtoaddress", rpc("getnewaddress"), 10)
        spent = {(vin["txid"], vin["vout"]) for vin in rpc("getrawtransaction", plain, True)["vin"]}
        check(not (spent & utxos_before),
              "an ordinary sendtoaddress did not spend any token-bearing UTXO")

    finally:
        if node.poll() is None:
            try:
                rpc("stop"); node.wait(timeout=60)
            except Exception:
                node.kill()
        shutil.rmtree(datadir, ignore_errors=True)

    print()
    if FAILURES:
        print(f"5D RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("5D RESULT: ALL PASS")


if __name__ == "__main__":
    main()
