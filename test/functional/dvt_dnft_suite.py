#!/usr/bin/env python3
"""DeVault DNFT functional suite — the CI entrypoint (4I.4).

Runs every DNFT functional harness (standalone scripts, each spinning its own regtest node(s)
against the built binaries) sequentially and aggregates results. Nonzero exit on any failure.

    python3 test/functional/dvt_dnft_suite.py [--fast] [--bin <build/src>]

--fast skips the large-payload tests (dvt_4f_*: ~8 MB blocks, ~1 GB of traffic) for quick
pre-commit runs; CI should run the full set. The explorer's own suite (M10) lives in the
dnft-explorer repository.
"""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))

# (script, fast) — fast=True runs even with --fast.
TESTS = [
    ("dvt_m8_du1_activation.py", True),   # DU1 activation boundary + script-flag bundle
    ("dvt_m12_ft_activation.py", True),   # FT-fork activation + reorg-safe boundary (5A)
    ("dvt_m9_dnft_binding.py", True),     # the consensus binding rule (raw-tx matrix)
    ("dvt_4d_wallet_dnft.py", True),      # wallet RPC lifecycle
    ("dvt_4e_nftindex.py", True),         # -nftindex + collection RPCs, reorg, rebuild
    ("dvt_4i_index_reorg.py", True),      # multi-block-same-category reorg (4I review F2)
    ("dvt_4f_bigmint.py", False),         # 990KB mints, relay, ~8MB blocks, persistence
    ("dvt_4f_mempool_pressure.py", False),# mempool limiting under jumbo mints
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fast", action="store_true", help="skip the large-payload tests")
    parser.add_argument("--bin", default=None, help="path to build/src (sets DVT_V2_BIN)")
    args = parser.parse_args()

    env = dict(os.environ)
    if args.bin:
        env["DVT_V2_BIN"] = os.path.abspath(args.bin)

    results = []
    for script, in_fast in TESTS:
        if args.fast and not in_fast:
            results.append((script, "SKIPPED", 0.0))
            continue
        path = os.path.join(HERE, script)
        print(f"\n=== {script} ===", flush=True)
        t0 = time.monotonic()
        proc = subprocess.run([sys.executable, path], env=env)
        dt = time.monotonic() - t0
        results.append((script, "PASS" if proc.returncode == 0 else "FAIL", dt))
        if proc.returncode != 0:
            print(f"*** {script} FAILED (rc={proc.returncode})", flush=True)

    print("\n" + "=" * 60)
    print("DNFT SUITE SUMMARY")
    failed = 0
    for script, status, dt in results:
        print(f"  {status:8s} {script:32s} {dt:7.1f}s")
        failed += status == "FAIL"
    print("=" * 60)
    if failed:
        print(f"RESULT: FAIL ({failed} test(s))")
        sys.exit(1)
    print("RESULT: ALL PASS")


if __name__ == "__main__":
    main()
