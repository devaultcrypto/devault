;;; DeVault V2 — Guix reproducible build manifest.
;;;
;;; The hermetic package set for `guix shell --container -m manifest.scm`. Combined with
;;; `guix time-machine --commit=<GUIX_PIN>` (set in libexec/prelude.bash), this pins the
;;; ENTIRE toolchain bit-for-bit. Modelled on Bitcoin Core's contrib/guix/manifest.scm,
;;; trimmed to what our depends + CMake/Ninja flow needs.
;;;
;;; HOST-AWARE: the driver (guix-build) builds ONE host per container and exports HOST.
;;; This manifest reads (getenv "HOST") and includes ONLY that host's toolchain on top of
;;; the shared base tools. So a native Linux build (G1) never instantiates the mingw or
;;; aarch64 cross toolchains — they're constructed lazily, only for their own host. This is
;;; what lets us bring the phases up incrementally (G1 green before G2/G3 toolchains exist).
;;;
;;; GUIX_PIN currently 230aa373f315f247852ee07dff34146e9b480aec.
;;; DRAFT (2026-06-15): native (G1) base set validated via --dry-run; the cross-toolchain
;;; branches (G2/G3/G4) still need validation against the pin as each phase is brought up.

(use-modules (gnu packages)
             (gnu packages cross-base)        ; cross-gcc / cross-binutils / cross-libc
             (gnu packages gcc)               ; gcc-12 (base compiler for the linux cross toolchain)
             (guix profiles))

;;; ---------------------------------------------------------------------------
;;; Base BUILD TOOLS shared by every host. These RUN on the build host but do NOT provide
;;; C/C++ system headers or libstdc++, so they don't collide with a cross toolchain's headers
;;; in the profile union (unlike a native gcc-toolchain — see native-compiler below).
;;; Validated to resolve at GUIX_PIN 230aa373 via `guix shell -m manifest.scm --dry-run`.
;;; ---------------------------------------------------------------------------
(define base-build-tools
  (map specification->package
       (list
        ;; build drivers
        "make"                      ; depends/ Makefile + most package builds (NOT in gcc-toolchain)
        "cmake-minimal"
        "ninja"
        "pkg-config"
        ;; autotools chain (several depends regenerate configure / need these)
        "m4"                        ; gmp et al.
        "autoconf" "automake" "libtool"
        "gettext-minimal"           ; msgfmt for some configure steps
        "bison"                     ; required by some depends
        "gperf"                     ; fontconfig
        "file"                      ; many configure scripts probe with `file`
        ;; scripting
        "python"                    ; build/codegen scripts (xcb-proto, qt)
        "perl"                      ; openssl/qt build steps in depends
        ;; archive / text / determinism utilities
        "coreutils"                 ; touch --date, sha256sum, sort, ...
        "findutils" "diffutils"
        "sed" "grep" "gawk"
        "tar" "gzip" "xz" "bzip2" "zip"   ; bzip2 essential: several sources are .tar.bz2
        "patch"
        "util-linux"                ; getopt etc.
        "which"
        "git-minimal"
        "bash-minimal")))

;;; The NATIVE compiler. CRITICAL: for a CROSS host this must NOT share the build profile with
;;; the cross toolchain — its glibc/libstdc++ headers+libs collide with the cross toolchain's at
;;; the profile's `include/`+`lib/`, the native files win, and the cross compile/link then grabs
;;; glibc <semaphore.h> / native libstdc++ (→ sem_timedwait / __gxx_personality_seh0 failures).
;;; So for cross hosts it goes into a SEPARATE native-helper profile (role "native-helper" below),
;;; sourced by build.sh only for BCHN's native sub-build. For the native host (x86_64-linux-gnu,
;;; build==host, no cross toolchain) there is no collision, so it stays in the single profile.
(define native-compiler
  (list (specification->package "gcc-toolchain@12")))   ; native C++20 compiler + binutils

;;; ---------------------------------------------------------------------------
;;; Native-build dependencies (every host).
;;; BCHN's NativeExecutable.cmake configures a SECOND, *native* cmake build (build-time
;;; codegen tools) WITHOUT the depends toolchain file, so it does find_package(OpenSSL/
;;; Boost/Event/...) against the system. In the hermetic container nothing is on the system,
;;; so configure fails ("Could NOT find OpenSSL"). gitian solved this by apt-installing
;;; libssl-dev/libboost-*/libevent-dev/etc. purely for the native tools (see the FIXME in
;;; contrib/gitian-descriptors/gitian-win.yml); we provide the Guix equivalents here. These
;;; only satisfy the native helper-tool build — the HOST binaries still link depends/ (the
;;; toolchain file's CMAKE_FIND_ROOT_PATH=ONLY keeps the host build away from these). Version
;;; skew vs depends is harmless (gitian likewise used newer distro libs natively).
;;; ---------------------------------------------------------------------------
(define native-build-deps
  (map specification->package
       (list "openssl" "boost" "libevent" "gmp" "zlib")))

;;; ---------------------------------------------------------------------------
;;; Per-host toolchain, constructed LAZILY (only for the requested HOST).
;;; ---------------------------------------------------------------------------
;; G1 portability — an x86_64-linux-gnu "cross" toolchain whose gcc is REBUILT against an
;; older glibc (2.33, the oldest packaged at this pin), so release binaries have a low glibc
;; symbol floor (run on Ubuntu 22.04 / Debian 12 and newer). Just bundling gcc-12 with an old
;; glibc is NOT enough — gcc's own libstdc++/libgcc would still be glibc-2.41-built — so we
;; rebuild gcc against glibc 2.33 via cross-gcc. Build/realize is a one-time, cached compile.
;; MUST stay byte-identical to contrib/guix/ realization so store paths match (see plan).
(define (linux-x86_64-old-glibc-toolchain)
  (let* ((target "x86_64-linux-gnu")
         (old-glibc (specification->package "glibc@2.33"))
         (xbinutils (cross-binutils target))
         (xlibc (cross-libc target #:libc old-glibc #:xbinutils xbinutils))
         (xgcc (cross-gcc target #:xgcc gcc-12 #:xbinutils xbinutils #:libc xlibc)))
    (list xbinutils xlibc xgcc)))

(define (host->toolchain host)
  (cond
    ;; G1 — Linux x86-64. NOTE: linux-x86_64-old-glibc-toolchain (above) is DEFERRED — at this
    ;; Guix pin, cross-libc can't build glibc 2.33/2.35 (the useful-floor versions): an internal
    ;; gexp/sexp mismatch in their package args makes the derivation builder unparseable; only
    ;; glibc 2.39+ work via cross-libc, which is too new to widen portability. Pending a decision
    ;; on the portability approach, the native gcc-toolchain (base-tools) is used → reproducible
    ;; build that runs on a Guix host (glibc floor 2.38). See plan / memory.
    ((string=? host "x86_64-linux-gnu")
     '())

    ;; G3 — Linux ARM64 cross (GCC from base, cross binutils/gcc/libc from cross-base).
    ;; TODO(G3): validate these resolve/build at the pin; LinuxAArch64.cmake is ready.
    ((string=? host "aarch64-linux-gnu")
     (list (cross-binutils "aarch64-linux-gnu")
           (cross-gcc "aarch64-linux-gnu"
                      #:xbinutils (cross-binutils "aarch64-linux-gnu")
                      #:libc (cross-libc "aarch64-linux-gnu"))
           (cross-libc "aarch64-linux-gnu")))

    ;; G2 — Windows x64: stock mingw-w64 cross toolchain (gcc 14.3.0) + nsis (setup.exe).
    ;; Thread model is win32, which on gcc 14 is fully C++20-capable — std::mutex / thread /
    ;; condition_variable / source_location / counting_semaphore all compile & link (verified).
    ;; The old "win32 breaks std::mutex" was a gcc-10-era limitation, now gone. We deliberately
    ;; do NOT build a custom --enable-threads=posix variant: on gcc 14 + winpthreads it has a
    ;; broken libstdc++ <semaphore> (sem_timedwait undeclared). win32-stock is also substitutable
    ;; (no from-source toolchain build). The native sub-build uses the separate native-helper profile.
    ((string=? host "x86_64-w64-mingw32")
     (list (specification->package "gcc-cross-x86_64-w64-mingw32-toolchain@14")
           (specification->package "nsis-x86_64")))

    ;; G4 — macOS (x86_64 + arm64 apple-darwin23): clang + cctools/ld64/libtapi, plus the
    ;; builder-supplied MacOSX14.5.sdk under depends/SDKs/ (NOT in this manifest).
    ;; TODO(G4): port Core's clang/cctools/ld64 set; also fix OSX.cmake's hardcoded x86_64.
    ((or (string=? host "x86_64-apple-darwin23")
         (string=? host "arm64-apple-darwin23"))
     '())

    (else (error "manifest.scm: unknown HOST" host))))

;;; ---------------------------------------------------------------------------
;;; Profile selection. Two roles (set by the driver via GUIX_MANIFEST_ROLE):
;;;   - "native-helper": ONLY the native compiler + native-build-deps. Built into a separate
;;;     GC-rooted profile and sourced by build.sh for BCHN's NativeExecutable sub-build on CROSS
;;;     hosts — kept out of the cross build's profile to avoid header/lib contamination.
;;;   - (default, the per-host BUILD profile):
;;;       * native host (x86_64-linux-gnu): build tools + native compiler + native-build-deps +
;;;         host toolchain '() — single profile, no cross toolchain so no collision.
;;;       * cross host: build tools + the cross toolchain ONLY (native compiler/deps live in the
;;;         helper profile). This keeps CROSS_*_INCLUDE_PATH / CROSS_LIBRARY_PATH clean.
;;; HOST defaults to native so `guix shell -m manifest.scm` works without the driver.
;;; ---------------------------------------------------------------------------
(define host (or (getenv "HOST") "x86_64-linux-gnu"))
(define (cross-host? h) (not (string=? h "x86_64-linux-gnu")))

(packages->manifest
 (cond
   ((equal? (getenv "GUIX_MANIFEST_ROLE") "native-helper")
    (append native-compiler native-build-deps))
   ((cross-host? host)
    (append base-build-tools (host->toolchain host)))
   (else
    (append base-build-tools native-compiler native-build-deps (host->toolchain host)))))
