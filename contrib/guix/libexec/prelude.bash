#!/usr/bin/env bash
# shellcheck shell=bash
#
# DeVault V2 — Guix reproducible build: shared prelude.
# Sourced by guix-build / guix-attest / build.sh. Sets up the repo root, version,
# deterministic environment, and output layout. Adapted from Bitcoin Core's
# contrib/guix/libexec/prelude.bash (autotools) for our CMake/Ninja flow.
#
# DRAFT (2026-06-13): first scaffold. Validate against a live, pinned Guix before relying on it.

set -e -o pipefail

# --- Repo root -------------------------------------------------------------
# This file lives at <repo>/contrib/guix/libexec/prelude.bash
PRELUDE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export GUIX_ROOT="$(cd "${PRELUDE_DIR}/.." && pwd)"          # <repo>/contrib/guix
export REPO_ROOT="$(cd "${GUIX_ROOT}/../.." && pwd)"          # <repo>

# Must be a clean git checkout for a release build; warn but don't hard-fail in dev.
if ! git -C "${REPO_ROOT}" rev-parse --git-dir >/dev/null 2>&1; then
    echo "WARNING: ${REPO_ROOT} is not a git checkout." >&2
fi

# --- Version ---------------------------------------------------------------
# Source of truth: CMakeLists.txt  ->  project(... VERSION X.Y.Z)
VERSION="$(grep -m1 -A3 '^project(' "${REPO_ROOT}/CMakeLists.txt" \
            | grep -oE 'VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' \
            | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || true)"
: "${VERSION:?could not parse VERSION from CMakeLists.txt}"
export VERSION
export DISTNAME="devault-${VERSION}"

# --- Pinned Guix commit(s) -------------------------------------------------
# The new "Ubuntu image": every independent builder MUST agree on these commits.
# Override per release with GUIX_PIN / GUIX_PIN_LINUX=<commit>.
# Set 2026-06-15 from `guix describe` on the build host (branch master, git.guix.gnu.org).
export GUIX_PIN="${GUIX_PIN:-230aa373f315f247852ee07dff34146e9b480aec}"

# Linux x86-64 release binaries need a LOW glibc symbol floor to run off a Guix host (portable
# on Ubuntu 22.04 / Debian 12+). GUIX_PIN's default glibc is 2.41 (too new), and cross-libc
# can't build an older glibc at that pin (a Guix-internal builder bug — see plan/memory). So
# the x86_64-linux-gnu target pins an OLDER Guix commit whose DEFAULT toolchain is gcc-12 +
# glibc 2.35 — a standard, well-tested toolchain, NO cross-libc gymnastics. Windows/macOS keep
# GUIX_PIN (their modern C++20 cross-compilers are unaffected). 842a11f1 = 2023-11-29.
export GUIX_PIN_LINUX="${GUIX_PIN_LINUX:-842a11f1caa1bb929c427722ad9d7b7c1ff65727}"

# Map a HOST to the Guix commit it builds under.
guix_pin_for_host() {
    case "$1" in
        x86_64-linux-gnu) echo "${GUIX_PIN_LINUX}" ;;
        *)                echo "${GUIX_PIN}" ;;
    esac
}

# --- Reproducibility env (faketime-free; Guix toolchain is deterministic) ---
# SOURCE_DATE_EPOCH: default to the HEAD commit time; override for a tagged release.
if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH="$(git -C "${REPO_ROOT}" log -1 --format=%ct 2>/dev/null || echo 1)"
fi
export SOURCE_DATE_EPOCH
export TZ="UTC"
export LC_ALL="C"
umask 0022

# --- Output layout ---------------------------------------------------------
# Artifacts land here, one subdir per host. Keep OUTSIDE the source tree so the
# container can --share it read-write while the source is exposed read-only.
export OUTDIR_BASE="${OUTDIR_BASE:-${REPO_ROOT}/guix-build/output/${VERSION}}"

# --- Host -> CMake toolchain file map --------------------------------------
# (Apple-Silicon entry depends on the OSX.cmake fix tracked in the impl plan, G4.)
declare -gA CMAKE_TOOLCHAIN_FILE=(
    [x86_64-linux-gnu]="cmake/platforms/Linux64.cmake"
    [x86_64-w64-mingw32]="cmake/platforms/Win64.cmake"
    [aarch64-linux-gnu]="cmake/platforms/LinuxAArch64.cmake"
    [x86_64-apple-darwin23]="cmake/platforms/OSX.cmake"
    [arm64-apple-darwin23]="cmake/platforms/OSXArm64.cmake"   # TODO(G4): create/parameterize
)

# Full mandatory release set (none optional — cb 2026-06-13).
export HOSTS_DEFAULT="x86_64-linux-gnu x86_64-w64-mingw32 aarch64-linux-gnu x86_64-apple-darwin23 arm64-apple-darwin23"
