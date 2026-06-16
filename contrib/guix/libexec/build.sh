#!/usr/bin/env bash
# shellcheck shell=bash
#
# DeVault V2 — Guix reproducible build: per-host build step.
# Runs INSIDE the hermetic Guix container (no network). Builds depends for $HOST,
# CMake-configures with the matching cmake/platforms/*.cmake, ninja + package,
# splits debug symbols, and emits normalized artifacts into $OUTDIR.
#
# This is the CMake/Ninja adaptation of Bitcoin Core's autotools build.sh, with the
# determinism steps ported from our own contrib/gitian-descriptors/gitian-win.yml
# (minus all the faketime machinery — the Guix toolchain is already deterministic).
#
# Inputs (exported by guix-build): HOST OUTDIR REPO_ROOT VERSION DISTNAME SOURCE_DATE_EPOCH
#
# DRAFT (2026-06-13): first scaffold. Validate against a live, pinned Guix before relying on it.

set -e -o pipefail

HOST="${1:?usage: build.sh <HOST>}"
: "${OUTDIR:?}"; : "${REPO_ROOT:?}"; : "${VERSION:?}"; : "${DISTNAME:?}"; : "${SOURCE_DATE_EPOCH:?}"

export TZ="UTC" LC_ALL="C"
umask 0022

# Reproducibility helpers for Qt's resource compiler (carried over from gitian-win).
export QT_RCC_TEST=1
export QT_RCC_SOURCE_DATE_OVERRIDE=1
export GZIP="-9n"

# --- Per-host toolchain file + extra flags ---------------------------------
declare -A TOOLCHAIN_FILE=(
    [x86_64-linux-gnu]="cmake/platforms/Linux64.cmake"
    [x86_64-w64-mingw32]="cmake/platforms/Win64.cmake"
    [aarch64-linux-gnu]="cmake/platforms/LinuxAArch64.cmake"
    [x86_64-apple-darwin23]="cmake/platforms/OSX.cmake"
    [arm64-apple-darwin23]="cmake/platforms/OSXArm64.cmake"   # TODO(G4)
)
TF="${TOOLCHAIN_FILE[$HOST]:?unknown HOST '$HOST'}"

# Native host builds without a toolchain file (Linux64.cmake is a near-noop; keep it
# explicit for parity with cross hosts). Cross hosts need the binutils prefix on PATH.
CMAKE_EXTRA=()
case "$HOST" in
    x86_64-linux-gnu|aarch64-linux-gnu)
        # BCHN's own release options (cf. gitian-linux): static C++ runtime + glibc back-compat.
        # NOTE: full off-Guix portability is NOT yet solved — binaries are built against Guix
        # glibc 2.41 and still require GLIBC_2.38 symbols + carry a /gnu/store RUNPATH, so they
        # run on a Guix host but not on older systems. Completing this needs an OLDER-glibc
        # toolchain (Bitcoin Core's approach) + -static-libgcc + patchelf --remove-rpath.
        # Tracked in DEVAULT_GUIX_IMPLEMENTATION_PLAN.md / memory; deferred pending decision.
        CMAKE_EXTRA+=( "-DENABLE_STATIC_LIBSTDCXX=ON" "-DENABLE_GLIBC_BACK_COMPAT=ON" )
        ;;
    x86_64-w64-mingw32)
        CMAKE_EXTRA+=( "-DCPACK_PACKAGE_FILE_NAME=${DISTNAME}-win64-setup-unsigned" )
        ;;
    arm64-apple-darwin23)
        # Until OSX.cmake is parameterized, this passes the prefix/arch through.
        CMAKE_EXTRA+=( "-DCMAKE_TOOLCHAIN_PREFIX=arm64-apple-darwin23" )
        ;;
esac

# Fast bring-up knob: GUIX_NO_QT=1 skips the (heavy) Qt GUI build so we can validate the
# core node/CLI pipeline end-to-end first. Default builds the full set (devault-qt included).
if [ "${GUIX_NO_QT:-0}" = "1" ]; then
    CMAKE_EXTRA+=( "-DBUILD_BITCOIN_QT=OFF" )
fi

echo "--- depends (HOST=$HOST) ---"
# Sources were pre-fetched on the host (container is offline). This builds the
# target libs into depends/$HOST. NO_QT must be passed HERE too (not just to cmake) or
# depends builds Qt regardless — that is the long pole, so the smoke knob skips it here.
DEPENDS_OPTS=()
[ "${GUIX_NO_QT:-0}" = "1" ] && DEPENDS_OPTS+=( "NO_QT=1" )
# Persistent build cache: set via ENV (BASE_CACHE is `?=` in depends/Makefile, and env vars
# survive the Makefile's MAKEOVERRIDES command-line filter, unlike `make BASE_CACHE=...`).
if [ -n "${DEPENDS_BASE_CACHE:-}" ]; then
    export BASE_CACHE="${DEPENDS_BASE_CACHE}"
    mkdir -p "${BASE_CACHE}"
fi
make -C "${REPO_ROOT}/depends" HOST="${HOST}" "${DEPENDS_OPTS[@]}" -j"${JOBS:-$(nproc)}"
DEPENDS_PREFIX="${REPO_ROOT}/depends/${HOST}"

echo "--- cmake configure (HOST=$HOST) ---"
BUILD_DIR="${REPO_ROOT}/guix-build/build/${HOST}"
INSTALL_DIR="${BUILD_DIR}/install/${DISTNAME}"
rm -rf "${BUILD_DIR}"
mkdir -p "${INSTALL_DIR}"

# NB: source dir MUST be the repo root — the cmake/platforms/*.cmake files resolve
# depends via ${CMAKE_CURRENT_SOURCE_DIR}/depends/<triple>.
cmake -GNinja -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/${TF}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCLIENT_VERSION_IS_RELEASE=ON \
    -DENABLE_REDUCE_EXPORTS=ON \
    -DENABLE_CLANG_TIDY=OFF \
    -DENABLE_MAN=OFF \
    -DBUILD_BITCOIN_SEEDER=OFF \
    -DCPACK_STRIP_FILES=ON \
    -DCCACHE=OFF \
    "${CMAKE_EXTRA[@]}"

echo "--- ninja build ---"
ninja -C "${BUILD_DIR}" -j"${JOBS:-$(nproc)}"

# security-check exists in the BCHN cmake tree (used by gitian-win). Best-effort:
# skip gracefully if the target is absent for a given host.
if ninja -C "${BUILD_DIR}" -t targets all 2>/dev/null | grep -q '^security-check:'; then
    echo "--- ninja security-check ---"
    ninja -C "${BUILD_DIR}" security-check
fi

echo "--- install ---"
# Install into INSTALL_DIR (= CMAKE_INSTALL_PREFIX). Host-specific packaging (NSIS/dmg/tar)
# happens per-host below; we do NOT run a generic `ninja package` for Linux (we build our
# own deterministic tarball from the install tree instead).
DESTDIR="" cmake --install "${BUILD_DIR}" || ninja -C "${BUILD_DIR}" install

# --- Per-host packaging + debug split + normalization ----------------------
# Common: split debug symbols into *.dbg, strip, deterministic archive.
strip_and_split() {
    local objcopy="$1" strip="$2" dir="$3"
    find "${dir}" -type f -executable ! -name "*.dbg" -print0 |
        while IFS= read -r -d '' f; do
            "${objcopy}" --only-keep-debug "${f}" "${f}.dbg"
            "${strip}" -s "${f}"
            "${objcopy}" --add-gnu-debuglink="${f}.dbg" "${f}"
        done
}

mkdir -p "${OUTDIR}"
case "$HOST" in
    x86_64-linux-gnu|aarch64-linux-gnu)
        strip_and_split objcopy strip "${INSTALL_DIR}"
        # Deterministic tarball: sorted entries, fixed owner/mtime.
        ( cd "${INSTALL_DIR}/.." && \
          find "${DISTNAME}" ! -name "*.dbg" | sort | \
            tar --no-recursion --owner=0 --group=0 --mtime="@${SOURCE_DATE_EPOCH}" \
                -c -T - | gzip -9n > "${OUTDIR}/${DISTNAME}-${HOST}.tar.gz" )
        ;;
    x86_64-w64-mingw32)
        strip_and_split "${HOST}-objcopy" "${HOST}-strip" "${INSTALL_DIR}"
        # NSIS installer (driven by cmake/modules/NSIS.template.in).
        ninja -C "${BUILD_DIR}" package
        find "${BUILD_DIR}" -name "*-setup-unsigned.exe" -exec cp {} "${OUTDIR}/" \;
        ( cd "${INSTALL_DIR}/.." && \
          find "${DISTNAME}" ! -name "*.dbg" -print0 | \
            xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}" && \
          find "${DISTNAME}" ! -name "*.dbg" -type f | sort | \
            zip -X@ "${OUTDIR}/${DISTNAME}-win64.zip" )
        ;;
    *-apple-darwin23)
        # macOS: .app via macdeployqtplus, then .dmg. TODO(G4): wire macdeploy + codesign-ready.
        echo "TODO(G4): darwin packaging (.app/.dmg) not yet implemented for ${HOST}" >&2
        ;;
esac

# Normalize every artifact mtime in OUTDIR.
find "${OUTDIR}" -print0 | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"

echo "--- ${HOST} artifacts ---"
( cd "${OUTDIR}" && sha256sum ./* 2>/dev/null || true )
