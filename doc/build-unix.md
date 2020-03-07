UNIX BUILD NOTES
====================
Some notes on how to build DeVault in Unix.

(for OpenBSD specific instructions, see [build-openbsd.md](build-openbsd.md))

To Build
---------------------

It is recommended to create a build directory to build out-of-tree.
This will build devault-qt as well if the dependencies are met.

Building with cmake
---------------------
```bash
mkdir build
cd build
cmake ../devault
make
make install # optional
```
This will build devault-qt as well if the dependencies are met.

Dependencies
---------------------

These dependencies are required:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 libsodium   | Randomness       | Random Number Generation use
 libboost    | Utility          | Library for threading, data structures, etc
 libevent    | Networking       | OS independent asynchronous networking

Optional dependencies:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 miniupnpc   | UPnP Support     | Firewall-jumping support
 libdb       | Berkeley DB      | Wallet storage (only needed when wallet enabled)
 qt          | GUI              | GUI toolkit (only needed when GUI enabled)
 libqrencode | QR codes in GUI  | Optional for generating QR codes (only needed when GUI enabled)
 univalue    | Utility          | JSON parsing and encoding (bundled version will be used unless --with-system-univalue passed to configure)
 libzmq3     | ZMQ notification | Optional, allows generating ZMQ notifications (requires ZMQ version >= 4.x)

For the versions used in the release, see [release-process.md](release-process.md) under *Fetch and build inputs*.

Memory Requirements
--------------------

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling DeVault. 

Dependency Build Instructions: Ubuntu & Debian
----------------------------------------------
Build requirements:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libsodium-dev libevent-dev bsdmainutils 

Options when installing required Boost library files:

1. On Ubuntu 18.04+ and Debian 8+ there are generic names for the individual boost development packages, so the following can be used to only install necessary parts of boost:

        sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev

2. If that doesn't work, you can install all boost development packages with:

        sudo apt-get install libboost-all-dev

BerkeleyDB 5.3 or later is required for the wallet. This can be installed with:

        sudo apt-get install libdb-dev libdb++-dev

-----------------------------------------
You will also need to install a C++17 compatible compiler to build devault.

For Ubuntu Xenial (16.04), Trusty (14.04) & Debian jessie (8) - 

        sudo apt-get install software-properties-common -y
        sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
        sudo apt-get update

On Debian 8 we will need to change the distro to trusty from the sources.list.d file

        cd /etc/apt/sources.list.d/
        sudo nano ubuntu-toolchain-r-ubuntu-test-jessie.list

> Now change

        deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu jessie main
> to

        deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu trusty main

        ctrl+x then Y  (to save the file)

Now for both Ubuntu versions Xenial (16.04) & below + Debian versions Stretch (9) & below

        sudo apt-get install g++-7 -y

Now you will need to either update your default gcc compiler to gcc-7/g++-7 or
specify it in your configure like below (when you reach the configure step after running autogen.sh) -

        ./configure CC=gcc-7 CXX=g++-7

-----------------------------------------

See the section "Disable-wallet mode" to build DeVault without wallet.

Optional (see --with-miniupnpc and --enable-upnp-default):

    sudo apt-get install libminiupnpc-dev

ZMQ dependencies (provides ZMQ API 4.x):

    sudo apt-get install libzmq3-dev

Dependencies for the GUI: Ubuntu & Debian
-----------------------------------------

If you want to build devault-qt, make sure that the required packages for Qt development
are installed. Qt 5 is necessary to build the GUI.
To build without GUI pass `--without-gui`.

To build with Qt 5 you need the following:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools 

libqrencode (optional) can be installed with:

    sudo apt-get install libqrencode-dev

Once these are installed, they will be found by configure and a devault-qt executable will be
built by default.

Dependency Build Instructions: Fedora
-------------------------------------
Build requirements:

    sudo dnf install gcc-c++ libtool make autoconf automake libsodium-devel libevent-devel boost-devel libdb-devel libdb-cxx-devel

Optional:

    sudo dnf install miniupnpc-devel

To build with Qt 5 you need the following:

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel 

libqrencode (optional) can be installed with:

    sudo dnf install qrencode-devel

Notes
-----
The release is built with GCC and then "strip devaultd" to strip the debug
symbols, which reduces the executable size by about 90%.


Options
------

There are a number of options that can either be enabled or disabled for builds

 * WITH_WALLET, Activate the wallet functionality
 * WITH_CLI, Build devault-cli
 * WITH_TX, Build devault-tx
 * WITH_QT, Build devault-qt
 * WITH_CTESTS, Build cmake unit tests
 * WITH_QRCODE, Enable QR code display
 * WITH_MULTISET, Build libsecp256k1's MULTISET module
 * WITH_MODULE_RECOVERY, Build libsecp256k1's recovery module
 * WITH_WALLETTOOL, Build bdb-check for checking wallet
 * WITH_ZMQ, Activate the ZeroMQ functionalities
 * WITH_SEEDER, Build devault-seeder
 * WITH_ROCKSDB, Build with RocksDB (blockchain restart needed if switched
 * WITH_HARD, Harden the executables
 * WITH_STATIC, Build with static linking
 * WITH_BENCH, Build benchmark code
 * WITH_BUNDLE, For MacOS build MacOS Application
 * WITH_REDUCED_EXPORTS, Reduce amount of exported symbols
 * WITH_STD_FILESYSTEM, Enable trying to use std::filesystem
 * WITH_WARNINGS, Enable extra warnings
 * WITH_LEVELDB_BUILD_TESTS, Build LevelDB's unit tests
 * WITH_ECHD_MODULE, Build libsecp256k1's ECDH module
 * WITH_SCHNORR, Build libsecp256k1's Schnorr module
 * WITH_SECP256K1_TESTS, Build secp256k1's unit tests
 * WITH_SECP256K1_BENCH, Build secp256k1's benchmarks
 * WITH_UNIVALUE_TESTS, Enable univalue's unit tests

miniupnpc
---------

[miniupnpc](http://miniupnp.free.fr/) may be used for UPnP port mapping.  It can be downloaded from [here](
http://miniupnp.tuxfamily.org/files/).  UPnP support is compiled in and
turned off by default.  See the configure options for upnp behavior desired:

Boost
-----
For documentation on building Boost look at their official documentation: http://www.boost.org/build/doc/html/bbv2/installation.html

Security
--------
To help make your devault installation more secure by making certain attacks impossible to
exploit even if a vulnerability is found, binaries are hardened by default.

Hardening enables the following features:

* Position Independent Executable
    Build position independent code to take advantage of Address Space Layout Randomization
    offered by some kernels. Attackers who can cause execution of code at an arbitrary memory
    location are thwarted if they don't know where anything useful is located.
    The stack and heap are randomly located by default but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

    	scanelf -e ./devault

    The output should contain:

     TYPE
    ET_DYN

* Non-executable Stack
    If the stack is executable then trivial stack based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, devault should be built with a non-executable stack
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./devault`

    the output should contain:
	STK/REL/PTL
	RW- R-- RW-

    The STK RW- means that the stack is readable and writeable but not executable.

Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, devault may be compiled in
disable-wallet mode with cmake option

   -DBUILD_WALLET=0
   and
   -DBUILD_QT=0 to disable GUI
    
> Note: The --disable-wallet option is a daemon only feature not compatible with QT.

Mining is also possible in disable-wallet mode, but only using the `getblocktemplate` RPC
call not `getwork`.

Additional Build Flags
--------------------------
TBD


ARM Cross-compilation
-------------------
These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

Make sure you install the build requirements mentioned above.
Then, install the toolchain and curl:

    sudo apt-get install g++-arm-linux-gnueabihf curl

To build executables for ARM:

    cd depends
    make HOST=arm-linux-gnueabihf NO_QT=1
    cd ..
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/platforms/LinuxARM.cmake  -DBUILD_SEEDER=OFF -DENABLE_REDUCE_EXPORTS=ON -DCCACHE=OFF -DBUILD_STD_FILESYSTEM=OFF -DBUILD_QT=0 -DBUILD_CTESTS=0 -DLINK_STATIC_LIBS=1 -GNinja ..
    make


For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.
