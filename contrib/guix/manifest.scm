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
             (guix profiles))

;;; ---------------------------------------------------------------------------
;;; Base tools shared by every host (native side of every build).
;;; Validated to resolve at GUIX_PIN 230aa373 via `guix shell -m manifest.scm --dry-run`.
;;; ---------------------------------------------------------------------------
(define base-tools
  (map specification->package
       (list
        ;; build system
        "gcc-toolchain@12"          ; native C++20 compiler (>= C++20 floor; matches/exceeds Core)
        "cmake-minimal"
        "ninja"
        "pkg-config"
        "bison"                     ; required by some depends
        "python"                    ; build/codegen scripts
        "perl"                      ; openssl/qt build steps in depends
        ;; archive / determinism utilities
        "coreutils"                 ; touch --date, sha256sum, sort, ...
        "findutils"
        "sed" "grep" "gawk"
        "tar" "gzip" "xz" "zip"
        "patch"
        "which"
        "git-minimal"
        "bash-minimal")))

;;; ---------------------------------------------------------------------------
;;; Per-host toolchain, constructed LAZILY (only for the requested HOST).
;;; ---------------------------------------------------------------------------
(define (host->toolchain host)
  (cond
    ;; G1 — native Linux x86-64: nothing extra beyond base-tools.
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

    ;; G2 — Windows x64 (mingw-w64, POSIX threads — win32 threads break std::mutex).
    ;; TODO(G2): the exact mingw cross-gcc/winpthreads + nsis package names need validating
    ;; at the pin; this is the first cross target we bring up after G1.
    ((string=? host "x86_64-w64-mingw32")
     (list (specification->package "nsis")
           ;; (cross-gcc "x86_64-w64-mingw32" #:xbinutils (cross-binutils "x86_64-w64-mingw32"))
           ;; (specification->package "mingw-w64-x86_64-winpthreads")
           ))

    ;; G4 — macOS (x86_64 + arm64 apple-darwin23): clang + cctools/ld64/libtapi, plus the
    ;; builder-supplied MacOSX14.5.sdk under depends/SDKs/ (NOT in this manifest).
    ;; TODO(G4): port Core's clang/cctools/ld64 set; also fix OSX.cmake's hardcoded x86_64.
    ((or (string=? host "x86_64-apple-darwin23")
         (string=? host "arm64-apple-darwin23"))
     '())

    (else (error "manifest.scm: unknown HOST" host))))

;;; ---------------------------------------------------------------------------
;;; Profile: base tools + the single requested host's toolchain.
;;; HOST defaults to native so `guix shell -m manifest.scm` works without the driver.
;;; ---------------------------------------------------------------------------
(define host (or (getenv "HOST") "x86_64-linux-gnu"))

(packages->manifest
 (append base-tools (host->toolchain host)))
