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

# CROSS hosts: source the native-helper profile EARLY (before depends) so every NATIVE step —
# depends' native build tools (native_b2 etc.) AND BCHN's NativeExecutable sub-build — finds a
# native gcc + native deps. Kept OUT of the cross build's own profile to avoid header/lib
# contamination: the cross compiler reads CROSS_*_INCLUDE_PATH / CROSS_LIBRARY_PATH (from the
# clean cross-only profile) while the native gcc reads C_INCLUDE_PATH / CPLUS_INCLUDE_PATH /
# LIBRARY_PATH (disjoint), so the cross packages stay clean. native gcc lands on PATH as `gcc`
# (unprefixed); the cross compiler stays `x86_64-w64-mingw32-gcc` — no conflict.
if [ -n "${NATIVE_HELPER_PROFILE:-}" ] && [ -e "${NATIVE_HELPER_PROFILE}/etc/profile" ]; then
    echo "--- sourcing native-helper profile (native tools/sub-build): ${NATIVE_HELPER_PROFILE} ---"
    GUIX_PROFILE="${NATIVE_HELPER_PROFILE}"
    . "${NATIVE_HELPER_PROFILE}/etc/profile"
    # guix profiles don't export CMAKE_PREFIX_PATH; the native sub-build's find_package(OpenSSL/
    # Boost/...) needs it. The main cross build ignores it (toolchain file FIND_ROOT_PATH=ONLY
    # restricts finds to depends/).
    export CMAKE_PREFIX_PATH="${NATIVE_HELPER_PROFILE}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
fi

# CROSS hosts: undo --emulate-fhs's glibc contamination of the cross include path. --emulate-fhs
# (needed so configure scripts find /usr/bin/env) unions a glibc into the container profile,
# whose headers land in CROSS_C_INCLUDE_PATH and break the cross compile (glibc <bits/types/
# time_t.h> vs mingw <corecrt.h>; <cwchar> 'swprintf' etc.). Re-point CROSS_*_INCLUDE_PATH at the
# cross toolchain's OWN clean headers — the gcc-cross-<host>-toolchain union (mingw sysroot +
# libstdc++, no glibc). CROSS_LIBRARY_PATH is left as-is: the profile lib has the matching cross
# libstdc++ (its separate gcc 'lib' output isn't in the union's lib). No-op for cross hosts that
# don't use a -toolchain meta-package (e.g. aarch64 raw cross — handled in its own phase).
# NB: `|| true` — under `set -e -o pipefail` a glob with no match makes the pipe (hence the
# command substitution, hence this plain assignment) non-zero and would abort the script. Hosts
# without a -toolchain meta-package (native x86_64, aarch64 raw cross) legitimately match nothing.
_xtc=$(ls -d /gnu/store/*-gcc-cross-"${HOST}"-toolchain-* 2>/dev/null | grep -vE '\.drv$|-builder$' | head -1 || true)
if [ -n "${_xtc}" ]; then
    echo "--- cross include paths -> clean toolchain (de-contaminate fhs glibc): ${_xtc} ---"
    export CROSS_C_INCLUDE_PATH="${_xtc}/include"
    export CROSS_CPLUS_INCLUDE_PATH="${_xtc}/include/c++:${_xtc}/include/c++/${HOST}:${_xtc}/include"
    # Also expose the toolchain root as a cmake find-root: cmake's find_package() for Windows
    # system libs (SHLWAPI etc.) uses find_path/find_library (NOT the compiler env), and the
    # mingw headers/libs live here. This is Guix's equivalent of gitian's /usr/<triple>. The
    # platform toolchain file appends $ENV{GUIX_CROSS_TOOLCHAIN_ROOT} to CMAKE_FIND_ROOT_PATH.
    export GUIX_CROSS_TOOLCHAIN_ROOT="${_xtc}"
fi

# CROSS Linux hosts (e.g. aarch64): the raw cross-gcc/cross-libc do NOT promote search paths, so
# CROSS_*_INCLUDE_PATH start empty and the cross gcc finds glibc + libstdc++ via its built-in
# sysroot — there is no FHS contamination to undo (unlike the mingw meta-package above). BUT the
# Linux KERNEL HEADERS (linux/types.h, pulled in transitively by glibc's <bits/sched.h>) live in
# the separately-propagated linux-libre-headers-cross package, which is NOT on the cross gcc's
# default search path. Add it so glibc's C headers resolve. gcc reads CROSS_C_INCLUDE_PATH while
# g++ reads CROSS_CPLUS_INCLUDE_PATH (a C++ TU does not consult the C var) and depends builds both
# C and C++ — so set BOTH. No-op on hosts without such a package (native x86_64; mingw).
_kh=$(ls -d /gnu/store/*-linux-libre-headers-cross-"${HOST}"-* 2>/dev/null | grep -vE '\.drv$|-builder$' | head -1 || true)  # || true: see _xtc note above
if [ -n "${_kh}" ]; then
    echo "--- cross kernel headers -> CROSS_*_INCLUDE_PATH: ${_kh} ---"
    export CROSS_C_INCLUDE_PATH="${_kh}/include${CROSS_C_INCLUDE_PATH:+:${CROSS_C_INCLUDE_PATH}}"
    export CROSS_CPLUS_INCLUDE_PATH="${_kh}/include${CROSS_CPLUS_INCLUDE_PATH:+:${CROSS_CPLUS_INCLUDE_PATH}}"
    # Qt's installed .prl files list bare `-lm`/`-lpthread` (glibc), and cmake's _qt5_*_process_prl_file
    # resolves every -l via find_library — which, under FIND_ROOT_PATH_MODE_LIBRARY=ONLY, only searches
    # depends/ → "Library not found: m". Expose the cross glibc prefix as a cmake find-root (the platform
    # file appends $GUIX_CROSS_TOOLCHAIN_ROOT, the same hook Win64.cmake uses). Ask the compiler for
    # libm.so's path and take its prefix; libm.so + the libpthread.a stub both live in <prefix>/lib.
    _libm=$("${HOST}-gcc" -print-file-name=libm.so 2>/dev/null || true)
    if [ -e "${_libm}" ]; then
        export GUIX_CROSS_TOOLCHAIN_ROOT="$(dirname "$(cd "$(dirname "${_libm}")" && pwd)")"
        echo "--- cross glibc find-root -> ${GUIX_CROSS_TOOLCHAIN_ROOT} ---"
    fi
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
# For the old-glibc Linux host, depends/hosts/linux.mk uses a PLAIN `gcc` on x86 build hosts
# (its `ifeq 86,$(build_arch)` shortcut) — which would be the native glibc-2.41 gcc and
# reintroduce the high glibc floor. Expose the cross toolchain under unprefixed names on a
# PATH scoped to JUST the depends build, so depends compiles against the old glibc while the
# later native sub-build (run_native_cmake.sh) still finds the native gcc on the normal PATH.
DEPENDS_PATH="${PATH}"
if [ "${HOST}" = "x86_64-linux-gnu" ] && command -v "${HOST}-gcc" >/dev/null 2>&1; then
    XBIN="${REPO_ROOT}/guix-build/xbin/${HOST}"
    rm -rf "${XBIN}"; mkdir -p "${XBIN}"
    _xdir="$(dirname "$(command -v "${HOST}-gcc")")"
    for f in "${_xdir}/${HOST}-"*; do
        [ -e "$f" ] && ln -sf "$f" "${XBIN}/$(basename "$f" | sed "s/^${HOST}-//")"
    done
    DEPENDS_PATH="${XBIN}:${PATH}"
fi
PATH="${DEPENDS_PATH}" make -C "${REPO_ROOT}/depends" HOST="${HOST}" "${DEPENDS_OPTS[@]}" -j"${JOBS:-$(nproc)}"
DEPENDS_PREFIX="${REPO_ROOT}/depends/${HOST}"

# security-check.py (run via `ninja security-check` below) shells out to $OBJDUMP/$READELF to
# introspect the built binaries. Its defaults (/usr/bin/objdump, /usr/bin/readelf) are the native
# binutils, which cannot parse a Windows PE -> "cannot open" and a spurious failure. Point them at
# the host-prefixed binutils when present (mingw PE needs this; guarded so the native Linux host,
# whose default check already passes, is left untouched if no prefixed tool exists).
if command -v "${HOST}-objdump" >/dev/null 2>&1; then export OBJDUMP="${HOST}-objdump"; fi
if command -v "${HOST}-readelf" >/dev/null 2>&1; then export READELF="${HOST}-readelf"; fi

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
    -DENABLE_TEST=OFF \
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
        # Native binutils for the native host; the cross binutils for a cross host — the native
        # objcopy/strip cannot read a foreign-arch ELF ("Unable to recognise the format" on the
        # aarch64 .so/binaries otherwise).
        if [ "${HOST}" = "x86_64-linux-gnu" ]; then
            strip_and_split objcopy strip "${INSTALL_DIR}"
        else
            strip_and_split "${HOST}-objcopy" "${HOST}-strip" "${INSTALL_DIR}"
        fi
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
        # `zip -@` APPENDS to an existing archive — re-running into the same OUTDIR would keep stale
        # entries (e.g. a pre-rename bitcoin-wallet.exe) and make the zip non-reproducible. Remove it
        # first so it is always built fresh from the current install tree.
        rm -f "${OUTDIR}/${DISTNAME}-win64.zip"
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
