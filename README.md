DeVault Core
============

Website: [devault.cc](https://devault.cc)

What is DeVault?
----------------

DeVault (ticker **DVT**) is a scaled peer-to-peer, next generation cryptocurrency network:
instant, low-fee transactions and onchain actions/records with no central authority.

Specifications
--------------

| Specification    | Value                  |
|------------------|------------------------|
| Ticker           | DVT                    |
| PoW algorithm    | SHA256d                |
| Difficulty       | LWMA                   |
| Block spacing    | 120 seconds            |
| Max block size   | 32 MB                  |
| RPC port         | 3339                   |
| P2P port         | 33039                  |
| Networking       | IPv4, IPv6, Tor        |
| Amount precision | 3 decimals (0.001 DVT) |

### Cold Reward requirements

| Requirement                       | Detail                 |
|-----------------------------------|------------------------|
| Confirmations (UTXO age)          | 21,915 blocks          |
| Minimum balance (blocks 1–109,575)| 1,000+ DVT per input   |
| Minimum balance (blocks 109,576+) | 25,000+ DVT per input  |

Building
--------

DeVault Node builds with CMake + Ninja and a **C++20 compiler (GCC ≥ 11 recommended)**:

```
mkdir build && cd build
cmake -GNinja ..
ninja
```

Output binaries are `build/src/devaultd`, `build/src/devault-cli`, and
`build/src/qt/devault-qt`. See [doc/build-unix.md](doc/build-unix.md) and the
other `doc/build-*.md` guides for platform-specific dependencies and options.

Upgrading from a legacy DeVault node
------------------------------------

1. **Back up** your `~/.devault` directory (always recommended before an upgrade).
2. Stop your existing `devaultd` / `devault-qt`.
3. Run the new `devaultd`. It opens `~/.devault`, performs a one-time reindex of
   the chainstate from your local block files (no re-download), and detects and
   migrates a legacy HD wallet automatically.

License
-------

DeVault Node is released under the terms of the MIT license. See
[COPYING](COPYING) or [https://opensource.org/licenses/MIT](https://opensource.org/licenses/MIT).

Development
-----------

Development takes place at
[https://github.com/devaultcrypto/devault](https://github.com/devaultcrypto/devault).
If you would like to contribute, please read [CONTRIBUTING.md](CONTRIBUTING.md).

See [doc/README.md](doc/README.md) for further documentation on installation,
building, and development.

Disclosure Policy
-----------------

We have a [Disclosure Policy](DISCLOSURE_POLICY.md) for the responsible disclosure
of security issues. Report vulnerabilities to **security@devault.cc**.
