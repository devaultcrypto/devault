# WINDOWS BUILD NOTES

Notes on how to build **DeVault Node** (`devaultd`, `devault-cli`, `devault-qt`) for Windows.

Windows binaries are produced by **cross-compiling from Linux** with the
[Mingw-w64](https://www.mingw-w64.org/) tool chain — either on a native Linux host (or VM) or inside
[Windows Subsystem for Linux 2 (WSL 2)](https://learn.microsoft.com/windows/wsl/about). 32-bit Windows
is not supported.

## Compiler requirement (important)

DeVault Node is **C++20** and requires a **C++20-capable Mingw-w64 tool chain, i.e. GCC ≥ 11**.

> The older guidance of Ubuntu 18.04/20.04/22.04 no longer applies: their system Mingw-w64 packages are
> GCC 7 / 9 / **10**, none of which provide C++20's `<source_location>` (and they report the draft
> `__cplusplus = 201709L`, which trips the `static_assert(__cplusplus >= 202002L)` in
> `src/compat/assumptions.h`). Use a distribution whose Mingw-w64 is **GCC ≥ 11** —
> **Ubuntu 24.04 LTS ("noble") ships GCC 13** and is the recommended host.

## Reproducible release builds → Guix

This guide produces **developer** builds. Reproducible, multi-signer-verifiable **release** binaries
will be produced with [GNU Guix](https://guix.gnu.org/) (gitian is deprecated and is being retired —
see `DEVAULT_GUIX_BUILD_PLAN.md` and, once implemented, `contrib/guix/`).

## Cross-compilation (Ubuntu 24.04)

The steps below work on Ubuntu 24.04 directly, in a VM, or under WSL 2. The `depends` system also works
on other distributions, but the package-install commands differ.

### 1. General dependencies

```bash
sudo apt update
sudo apt install autoconf automake bsdmainutils build-essential bison cmake curl git \
    libtool ninja-build nsis pkg-config python3
```

A host tool chain (`build-essential`) is required because some dependency packages build host utilities
used during the cross build. See also [dependencies.md](dependencies.md).

### 2. Mingw-w64 cross compiler (POSIX threads)

```bash
sudo apt install g++-mingw-w64-x86-64
# Select the POSIX-threads variant (the win32 variant conflicts with std::mutex):
sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
```

Confirm the version is **GCC ≥ 11**:

```bash
x86_64-w64-mingw32-g++ --version
```

### 3. Get the source

```bash
git clone https://github.com/devaultcrypto/devault.git
cd devault
```

> **WSL note:** the source must live on the native Linux filesystem (e.g. `~/devault`), **not** under
> `/mnt/c/…`. Before building under WSL, strip Windows entries from `PATH`:
> `export PATH=$(echo "$PATH" | sed -e 's|:/mnt.*||g')`

### 4. Build the dependencies for Win64

```bash
make -C depends build-win64
```

This populates `depends/x86_64-w64-mingw32/`. The build below resolves its libraries from there via
`cmake/platforms/Win64.cmake` (which sets `CMAKE_FIND_ROOT_PATH` to that prefix), so the build directory
**must be at the project root** (not inside `depends/`).

### 5. Configure and build

```bash
cmake -GNinja -B build_win64 -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Win64.cmake \
    -DENABLE_MAN=OFF -DBUILD_BITCOIN_SEEDER=OFF        # seeder is not supported on Windows
ninja -C build_win64 bitcoind bitcoin-cli bitcoin-qt    # or: ninja -C build_win64
```

The output binaries (rebranded) are:

- `build_win64/src/devaultd.exe`
- `build_win64/src/devault-cli.exe`
- `build_win64/src/qt/devault-qt.exe`

### 6. Build the installer (optional)

```bash
ninja -C build_win64 package
```

This produces the NSIS `*-win64-setup.exe` installer.

## Building under WSL 2

Install **Ubuntu 24.04** from the Microsoft Store into WSL 2, open its shell, then follow the
cross-compilation steps above (mind the filesystem/`PATH` note in step 3).

## Depends system

For further documentation on the `depends` system see [README.md](../depends/README.md).

## Footnote — POSIX vs win32 threads

The Mingw-w64 packages install two tool-chain variants: POSIX threads and win32 threads. The win32
variant's headers conflict with parts of the C++ standard library (notably `std::mutex`), so DeVault
Node must be built with the **POSIX** variant (selected in step 2).
