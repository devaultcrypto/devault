#!/usr/bin/env python3
"""DeVault M8 — DU1 (DeVault Upgrade 1) activation + FT-deferral gate, functional test.

Verifies on regtest, against a real devaultd with -du1activationheight set:
  1. PRE-fork: a token-forming tx is rejected exactly as today (txn-tokens-before-activation).
  2. POST-fork: an NFT (capability none, no amount) genesis + transfer are accepted and mined.
  3. POST-fork: a fungible-amount token genesis is rejected by the FT-deferral gate
     (bad-txns-token-ft-deferred) — the DeVault DNFT layer, DEVAULT_NFT_SPEC.md §10.8.
  4. POST-fork: native introspection is active (a P2SH OP_TXVERSION redeem spends) — proving
     the DU1 VM bundle (Q9) beyond just SCRIPT_ENABLE_TOKENS.
  5. Restart across the boundary is clean and state persists.

Standalone (no test_framework dependency): drives devaultd over JSON-RPC.
"""
import base64
import hashlib
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

BIN = os.environ.get("DVT_V2_BIN", "/home/pro/Git/devault-dev/devaultV2/build/src")
DEVAULTD = os.path.join(BIN, "devaultd")
RPCPORT = 29743
DU1_HEIGHT = 250

FAILURES = []


def check(cond, msg):
    print(f"  [{'PASS' if cond else 'FAIL'}] {msg}", flush=True)
    if not cond:
        FAILURES.append(msg)


def sha256(b):
    return hashlib.sha256(b).digest()


def sha256d(b):
    return sha256(sha256(b))


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


class RpcError(Exception):
    def __init__(self, err):
        super().__init__(err.get("message", ""))
        self.code = err.get("code")
        self.message = err.get("message", "")


def ser_compact(n):
    if n < 0xFD:
        return bytes([n])
    if n <= 0xFFFF:
        return b"\xfd" + n.to_bytes(2, "little")
    return b"\xfe" + n.to_bytes(4, "little")


def main():
    datadir = tempfile.mkdtemp(prefix="dvt-m8-")
    args = [DEVAULTD, "-regtest", f"-datadir={datadir}", f"-rpcport={RPCPORT}",
            "-listen=0", "-server=1", f"-du1activationheight={DU1_HEIGHT}",
            "-acceptnonstdtxn=0"]  # force standardness so the pre-fork policy reject is observable
    node = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    cookie = os.path.join(datadir, "regtest", ".cookie")
    rpc = Rpc(RPCPORT, cookie)

    def wait_up():
        for _ in range(120):
            try:
                rpc("getblockchaininfo")
                return
            except Exception:
                time.sleep(0.5)
        raise RuntimeError("node did not start")

    def coinbase_at(height):
        cb = rpc("getblock", rpc("getblockhash", height), 2)["tx"][0]
        return cb["txid"], cb["vout"][0]["value"]

    def build_token_tx(cb_height, out_addr, out_value, token_data, change_addr):
        cbid, cbval = coinbase_at(cb_height)
        inputs = [{"txid": cbid, "vout": 0}]
        # token category for a genesis == the input's txid (prevout n==0)
        outs = [{out_addr: {"amount": f"{out_value:.3f}", "tokenData": token_data(cbid)}},
                {change_addr: f"{cbval - out_value - 1.0:.3f}"}]  # 1 DVT fee
        raw = rpc("createrawtransaction", inputs, outs)
        signed = rpc("signrawtransactionwithwallet", raw)
        assert signed.get("complete"), signed
        return signed["hex"], cbid

    try:
        wait_up()
        print(f"== setup: du1activationheight={DU1_HEIGHT}, mine to 240 (pre-fork)", flush=True)
        rpc("createwallet", "")
        miner = rpc("getnewaddress")
        rpc("generatetoaddress", 240, miner)
        check(rpc("getblockcount") == 240, "tip at 240 (pre-fork)")

        # ---- 1. PRE-fork: an NFT-forming tx is non-standard (tokens not active) ----
        print("== 1. pre-fork token tx rejected", flush=True)
        nft_td = lambda cat: {"category": cat, "nft": {"capability": "none", "commitment": "abcd"}}
        pre_hex, _ = build_token_tx(10, rpc("getnewaddress"), 5.0, nft_td, rpc("getnewaddress"))
        try:
            rpc("sendrawtransaction", pre_hex)
            check(False, "pre-fork token tx should have been rejected")
        except RpcError as e:
            check("txn-tokens-before-activation" in e.message,
                  f"pre-fork token tx rejected: {e.message}")

        # ---- cross the fork boundary ----
        print(f"== mine to {DU1_HEIGHT} (activate DU1)", flush=True)
        rpc("generatetoaddress", DU1_HEIGHT - rpc("getblockcount"), miner)
        check(rpc("getblockcount") == DU1_HEIGHT, f"tip at {DU1_HEIGHT} (DU1 active for mempool)")

        # ---- 2. POST-fork: NFT genesis accepted + mined ----
        print("== 2. post-fork NFT genesis + transfer", flush=True)
        nft_addr = rpc("getnewaddress")
        nft_hex, nft_cat = build_token_tx(20, nft_addr, 5.0, nft_td, rpc("getnewaddress"))
        nft_txid = rpc("sendrawtransaction", nft_hex)
        check(bool(nft_txid), "post-fork NFT genesis accepted to mempool")
        rpc("generatetoaddress", 1, miner)
        nft_conf = rpc("getrawtransaction", nft_txid, True)
        vout0 = nft_conf["vout"][0]
        td = vout0.get("tokenData", {})
        # Acceptance already proves the output has no HasAmount bit (else the FT-deferral gate
        # would have rejected it); the displayed amount is a cosmetic "0". Assert the shape.
        check(td.get("category") == nft_cat and td.get("nft", {}).get("capability") == "none"
              and td.get("amount", "0") == "0", f"minted NFT is immutable, no FT amount (tokenData={td})")

        # transfer the NFT (immutable: same category+commitment preserved)
        xfer_addr = rpc("getnewaddress")
        xfer_in = [{"txid": nft_txid, "vout": 0}]
        xfer_out = [{xfer_addr: {"amount": "4.000",
                                 "tokenData": {"category": nft_cat,
                                               "nft": {"capability": "none", "commitment": "abcd"}}}}]
        xfer_raw = rpc("createrawtransaction", xfer_in, xfer_out)
        xfer_signed = rpc("signrawtransactionwithwallet", xfer_raw)
        check(xfer_signed.get("complete"), "NFT transfer signed")
        xfer_txid = rpc("sendrawtransaction", xfer_signed["hex"])
        rpc("generatetoaddress", 1, miner)
        xfer_conf = rpc("getrawtransaction", xfer_txid, True)
        check(xfer_conf["vout"][0].get("tokenData", {}).get("category") == nft_cat,
              "NFT transfer preserved the category (ownership moved)")

        # ---- 3. POST-fork: FT genesis rejected by the FT-deferral gate ----
        print("== 3. post-fork FT genesis rejected (FT-deferral gate)", flush=True)
        ft_td = lambda cat: {"category": cat, "amount": "1000"}
        ft_hex, _ = build_token_tx(30, rpc("getnewaddress"), 5.0, ft_td, rpc("getnewaddress"))
        try:
            rpc("sendrawtransaction", ft_hex)
            check(False, "post-fork FT genesis should be rejected")
        except RpcError as e:
            check("bad-txns-token-ft-deferred" in e.message,
                  f"post-fork FT genesis rejected by the gate: {e.message}")

        # ---- 4. POST-fork: native introspection active (P2SH OP_TXVERSION) ----
        print("== 4. post-fork native introspection (P2SH OP_TXVERSION)", flush=True)
        # redeem: OP_TXVERSION OP_2 OP_EQUAL  (true iff spending tx nVersion == 2)
        redeem = bytes([0xC2, 0x52, 0x87])
        dec = rpc("decodescript", redeem.hex())
        p2sh_addr = dec["p2sh"]  # let the node compute the P2SH cashaddr (no local ripemd160)
        p2sh_spk = rpc("validateaddress", p2sh_addr)["scriptPubKey"]
        fund_txid = rpc("sendtoaddress", p2sh_addr, 3.0)
        rpc("generatetoaddress", 1, miner)
        fund = rpc("getrawtransaction", fund_txid, True)
        vout_idx = next(i for i, v in enumerate(fund["vout"])
                        if v["scriptPubKey"]["hex"] == p2sh_spk)
        fund_val = fund["vout"][vout_idx]["value"]
        # hand-build the spend: scriptSig = push(redeem), nVersion = 2
        script_sig = bytes([len(redeem)]) + redeem
        out_addr_info = rpc("validateaddress", rpc("getnewaddress"))
        out_spk = bytes.fromhex(out_addr_info["scriptPubKey"])
        out_val = int(round((fund_val - 1.0) * 1e8))  # 1 DVT fee
        tx = struct.pack("<I", 2)  # version 2
        tx += ser_compact(1)
        tx += bytes.fromhex(fund_txid)[::-1] + struct.pack("<I", vout_idx)
        tx += ser_compact(len(script_sig)) + script_sig
        tx += struct.pack("<I", 0xFFFFFFFF)
        tx += ser_compact(1)
        tx += struct.pack("<q", out_val) + ser_compact(len(out_spk)) + out_spk
        tx += struct.pack("<I", 0)
        try:
            intro_txid = rpc("sendrawtransaction", tx.hex())
            rpc("generatetoaddress", 1, miner)
            mined = rpc("getrawtransaction", intro_txid, True).get("confirmations", 0) >= 1
            check(mined, "P2SH OP_TXVERSION spend accepted -> native introspection is active")
        except RpcError as e:
            check(False, f"introspection spend rejected: {e.message}")

        # ---- 5. restart across the boundary ----
        print("== 5. restart persists post-fork state", flush=True)
        tip = rpc("getbestblockhash")
        rpc("stop")
        node.wait(timeout=60)
        node2 = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            wait_up()
            check(rpc("getbestblockhash") == tip, "tip unchanged after restart")
            # the NFT still resolves and is still a token UTXO
            reopened = rpc("getrawtransaction", xfer_txid, True)
            check(reopened["vout"][0].get("tokenData", {}).get("category") == nft_cat,
                  "NFT token data persists across restart")
            rpc("stop")
            node2.wait(timeout=60)
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
        print(f"M8 RESULT: FAIL ({len(FAILURES)})")
        for f in FAILURES:
            print("  -", f)
        sys.exit(1)
    print("M8 RESULT: ALL PASS")


if __name__ == "__main__":
    main()
