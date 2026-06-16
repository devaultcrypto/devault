# Reproducible DeVault V2 release builds with GNU Guix

This directory builds **bit-for-bit reproducible** DeVault V2 release artifacts —
`devaultd` / `devault-cli` / `devault-qt`, the Windows NSIS installer, and the macOS
`.app`/`.dmg` — for every supported host, using [GNU Guix](https://guix.gnu.org/) as a
hermetic, pinnable toolchain. It replaces the deprecated `contrib/gitian-*` setup.

> **Status: scaffolding (2026-06-13).** First-draft `manifest.scm` / `libexec/build.sh` /
> `guix-build`. Validate against a live, pinned Guix before trusting any output. The full
> plan is `DEVAULT_GUIX_IMPLEMENTATION_PLAN.md` at the top of the harness repo.

## Mandatory targets (none optional)

`x86_64-linux-gnu`, `x86_64-w64-mingw32`, `aarch64-linux-gnu`, `x86_64-apple-darwin23`,
`arm64-apple-darwin23`.

## Install Guix (once, on the build host)

Use the **upstream binary install script**, *not* Ubuntu's `apt install guix`:

```bash
cd /tmp
wget https://git.savannah.gnu.org/cgit/guix.git/plain/etc/guix-install.sh
chmod +x guix-install.sh
sudo ./guix-install.sh            # creates guixbuilder users + the guix-daemon service
sudo systemctl enable --now guix-daemon

export PATH="$HOME/.config/guix/current/bin:$PATH"   # add to ~/.bashrc
guix pull                          # first pull is slow
guix describe                      # RECORD this commit -> it is your GUIX_PIN
```

Set the recorded commit as `GUIX_PIN` in `libexec/prelude.bash` (or pass `GUIX_PIN=<commit>`
per build). This commit is the new "Ubuntu image": every independent builder must agree on it.

## Build

```bash
cd devaultV2

# 1. Pre-fetch depends sources — the Guix container has NO network.
make -C depends download

# 2. (macOS only) place the builder-supplied SDK; never committed.
#    tar -C depends/SDKs -xf /path/to/MacOSX14.5.sdk.tar.xz

# 3. Build. Full mandatory set:
./contrib/guix/guix-build
#    ...or a subset while developing:
HOSTS="x86_64-linux-gnu" ./contrib/guix/guix-build
```

Artifacts land in `guix-build/output/<version>/<host>/`.

## Verify reproducibility

Run the build twice into clean trees (or on two machines) and compare:

```bash
sha256sum guix-build/output/*/x86_64-linux-gnu/*
# hashes from two independent builders must match exactly
```

Multi-signer attestation (`guix-attest` / `guix-verify`) lands in phase G5.

## Files

| File | Role |
|---|---|
| `guix-build` | Driver: loops hosts, runs each build in a pinned, network-isolated container |
| `manifest.scm` | The pinned hermetic package set (toolchains + base tools) |
| `libexec/prelude.bash` | Shared env: repo root, version, `SOURCE_DATE_EPOCH`, `GUIX_PIN`, output layout |
| `libexec/build.sh` | Per-host: depends → cmake(toolchain) → ninja → package → strip/normalize |
| `guix-attest` / `guix-verify` | (G5) produce / cross-check `SHA256SUMS(.asc)` |

## How this maps to the gitian you knew

- gitian's Ubuntu image + apt list → a single **pinned Guix commit** + `manifest.scm`.
- gitian's `faketime` wrapper zoo → `SOURCE_DATE_EPOCH` + Guix's deterministic toolchain.
- gitian's VM → `guix shell --container` (namespaced, offline).
- `gitian-signing/*.asc` → `guix-attest` / `guix-verify`.

See `DEVAULT_GUIX_IMPLEMENTATION_PLAN.md` §1 for the full mental-model mapping and the
first-time-Guix-user Q&A.
