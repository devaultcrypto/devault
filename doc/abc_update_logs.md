XXXXX - Partial upgrade of wallet stuff
     
      [CMAKE] Move the upgrade activated tests out of the TestSuite module
      [CMAKE] Make the list of tests a property of the test suite
      [CMAKE] Factorize the test suite target name construction
      lcov: filter depends from coverage report
      Failing functional tests stop lcov
      [SECP256K1] Fix ability to compile tests without -DVERIFY.
      [Automated] Update chainparams
      Clear the IFP bip in the version by default to avoid accidental activation
      Fix a race condition in abc-finalize-block
      Merge #12035: [qt] change µBTC to bits
      Merge #14307: Consolidate redundant implementations of ParseHashStr
      Merge #13424: Consistently validate txid / blockhash length and encoding in rpc calls
      Move github-release to appropriate contrib sub-directory
      Move build_* wrapper scripts to devtools
      lcov: filter /usr/lib/ from coverage reports
21.0      
      Update manpages for 0.21.0 release
      Add missing items to release-notes + formatting fixups
      Bump automatic replay protection to Nov 2020 upgrade
      Change version to 0.21.0
      [sigcheck] Add per tx limit
      Implement miner funding features
      [sigcheck] Remove redundant sigcheck in CheckInputs
      [ConnectBlock] Use an index to refers into blockundo.vtxundo rather than pusing as we go
      [LINTER] Fix the doxygen linter when inline comments are multilined
      fix ASAN error relating to nSigChecksBlockLimiter
      [consensus rule] limit sigchecks in a block after phonon upgrade
      deactivate sigops limits in phonon upgrade
      Merge #12079: Improve prioritisetransaction test coverage
      Enable new ancestor/descendants chains limit at fork
      Merge #14460: tests: Improve 'CAmount' tests
      Merge #14679: importmulti: Don't add internal addresses to address book
      Add --commit to automated-commits to make local testing easier
      Update seeds
      [Automated] Update chainparams
      Fix exit behavior in test-seeds
      Merge #14720: rpc: Correctly name arguments
      [CMAKE] Use the new FindPython module
      Merge #14410: rpcwallet: 'ischange' field for 'getaddressinfo' RPC
      Fix a race condition with rpc ports in check-seeds
      [standardness] activate SCRIPT_VERIFY_INPUT_SIGCHECKS in next upgrade
      [CI] Run the functional tests when wallet is disable
      Add new post-fork ancestor and descendants limit.
      OP_REVERSEBYTES activation logic
      Revert "OP_REVERSEBYTES activation logic"
      Merge #13152: [rpc] Add getnodeaddresses RPC command
      OP_REVERSEBYTES activation logic
      Merge #15321: doc: Add cs_main lock annotations for mapBlockIndex
      Merge #14310: [wallet] Ensure wallet is unlocked before signing
      Merge #14236: qa: generate --> generatetoaddress change to allow tests run without wallet
      Use a temporary datadir and non-default RPC port when testing seeds
      Remove updating chainparams from release process
      Merge #13030: [bugfix] [wallet] Fix zapwallettxes/multiwallet interaction.
      Merge #10816: Properly forbid -salvagewallet and -zapwallettxes for multi wallet.
      rm cruft: contrib/rpm
      Prevent arc land from failing if there is nothing to lint
      Merge #10451: contrib/init/bitcoind.openrcconf: Don't disable wallet by default
      RPCHelpMan fixups
      Merge #14530: Use RPCHelpMan to generate RPC doc strings
      rm cruft: travis-ci doc
      Merge #14398: tests: Don't access out of bounds array index: array[sizeof(array)]
      Merge #14822: bench: Destroy wallet txs instead of leaking their memory
      Merge #17455: tests: Update valgrind suppressions
      Fix nits in RPC help messages
      Fix LockDirectory test failure when the Junit logger is enabled
      [cmake] Add the suite name to the test runner.
      Move mempool policy constants to policy/mempool.h
      [Automated] Update chainparams
      Add a script for building and pushing automated commits
      remove SCRIPT_VERIFY_COMPRESSED_PUBKEYTYPE (aka WITNESS_PUBKEYTYPE)
      [avalanche_tests] fix block index accesses
      test: Build fuzz targets into seperate executables
      docs: Spelling error fix on fuzzing.md
      build: Allow to configure --with-sanitizers=fuzzer
      tests: Use MakeUnique to construct objects owned by unique_ptrs
      [tests] Add libFuzzer support.
      Cache the result of chainparams.GetConsensus() in miner code
      Update weird way to check for MTP in functional tests
      [DOC] Update fuzzing.md to use cmake/ninja build and fix some nits
      Merge #15203: Fix issue #9683 "gui, wallet: random abort (segmentation fault)"
      remove print-debugging statement that ended up in master
      Merge #13679: Initialize m_next_send_inv_to_incoming
      [python linting] tweak options
      Update copyright year in COPYING file
      add missing swap to CScriptCheck
      Update blockchain RPC to report all BIP9 based on versionbitsinfo
      Make standard flags based off consensus flags
      [python linting] apply aggressive mode in autopep8 (line wrapping)
      apply nontrivial changes suggested by autopep8 aggressive
      [python linting] nit
      Update copyright for various files
      [CI] Deduplicate test_bitcoin run for the TSAN build
      Make the CI record and track the unit tests
      [python linting] enforce E722: do not use bare except
      Merge #13823: qa: quote path in authproxy for external multiwallets
      Merge #14179: qa: Fixups to "Run all tests even if wallet is not compiled"
      Merge #14180: qa: Run all tests even if wallet is not compiled
      Update copyright_header.py to not duplicate parts of the header
      Run the linters as part of arc land
      qa: Prevent concurrency issues reading .cookie file
      Merge #9739: Fix BIP68 activation test
      [python linting] Enforce all flake8 F codes
      [LINTER] Prevent using inline doxygen comments on their own line
      [python linting] enforce E731: Do not assign a lambda expression, use a def
      [LINTER] Silent a shellcheck false positive
      mempool_accept nits: use FromHex/ToHex
      [python linting] enforce E713: Test for membership should be 'not in'
      [python linting] enforce E712: Comparison to true should be 'if cond is true:' or 'if cond:'
      [python linting] enforce E265: Block comment should start with '# '
      [python linting] enforce all flake8 E & W codes besides some exceptions
      [lint] trailing whitespaces in python tests
      add E711 to python linter (reject `== None` / `!= None`)
      Merge #14964: test: Fix race in mempool_accept
      Merge #14926: test: consensus: Check that final transactions are valid
      Merge #14940: test: Add test for truncated pushdata script
      fix whitespace lint error
      Add build support for 'gprof' profiling.
      [CMAKE] Get rid of ECM for running the sanitizers
      fix comments //!<  to  //!
      Consolidate check-seeds builds
      Merge #14094: refactoring: Remove unreferenced local variables
      Added OP_REVERSEBYTES+implementation, added (always disabled) activation flag
      Bump timeouts in slow running tests
      Merge #14993: rpc: Fix data race (UB) in InterruptRPC()
      Merge #12153: Avoid permanent cs_main lock in getblockheader
      Merge #15350: qa: Drop RPC connection if --usecli
      Merge #14958: qa: Remove race between connecting and shutdown on separate connections
      Merge #14982: rpc: Add getrpcinfo command
      Merge #14777: tests: Add regtest for JSON-RPC batch calls
      Merge #14670: http: Fix HTTP server shutdown
      fix p2p_compactblocks flakiness
      fix AreInputsStandard sigops counting
      Merge #16538: test: Add missing sync_blocks to feature_pruning
      Merge #12917: qa: Windows fixups for functional tests
      Disable clang-tidy by default
      Remove redundant std::move
      scratch space: use single allocation
      add sigchecks limiter to CheckInputs
      http: add missing header bootlegged by boost < 1.72
      Remove unecessary include of iostream
      Merge #13962: Remove unused dummy_tx variable from FillPSBT
      Avoid redundant calls to GetChainParams and GetConsensus in CChainState::AcceptBlock
      Fix the build with GCC < 8
      sigcheckcount_tests: better macro
      Increase timeout in avalanche test
      [CMAKE] Make ccache to work with clang-tidy
      Automatically add missing braces
      [CMAKE] Fix incompatibility between clang-tidy and -fstack-reuse
      [CMAKE] Enable clang-tidy
      rearrange ATMP in preparation for SigChecks accounting in mempool
      Bump version to 0.20.13
      synchronize validation queue during submitblock
      Ensure the thresold for BIP9 can be configured on a per activation basis.
      Restore BIP9 RPC support in getblockchaininfo
      Ressurect BIP9 style activation mechanism
      Merge #14209/#17218: logging: Replace LogPrintf macro with regular function
      Merge #13938: refactoring: Cleanup StartRest()
      Fix ubsan failure in excessiveblock_tests
      doxygen: Remove misleading checkpoints comment in CMainParams
      Chainparams: Use name constants in chainparams initialization
      fix a deserialization overflow edge case
      Prevent wrapping in setexcessiveblock RPC
      arc lint --everything
      [tests] fix formatting in feature_dbcrash
      ConnectBlock: fix slow usage of AddCoins
      nit: functional test chmod +x
      track nSigChecks in CheckInputs
      Fix missing lock in txvalidationcache_tests
      add sigChecks value to script cache
      [CMAKE] Make Qt protobuf an object library
      Add braces to unit tests files
20.12
      Update manpages for 0.20.12 release
      Update chainparams
      Added some release notes
      Update seeds
      Merge #13967: [walletdb] don't report minversion wallet entry as unknown
      Add braces to various files
      Add braces to bench files
      Add braces to wallet files
      Add braces to GUI files
      Add braces to crypto files
      Add braces to seeder files
      Add braces to bitcoin-tx and bitcoin-cli
      [SECP256K1] CMake: set default build configuration and optimization
      Merge #13913: qa: Remove redundant checkmempool/checkblockindex extra_args
      Merge #13948: trivial: Removes unsed `CBloomFilter` constructor.
      Use virtualsize for mining/mempool priority
      rpc: Make unloadwallet wait for complete wallet unload
      [SECP256K1] CMake: add an install target
      [CMAKE] Allow for component based installation
      [CMAKE] Minor improvements to the install_shared_library function
      [SECP256K1] CMake: Fix in-tree build
      [SECP256K1] Travis: pass extra flags to the CMake build
      [SECP256K1] CMake: make the GMP bignum support optional
      [SECP256K1] CMake: allow to select field and scalar implementation
      Fix autotools build failure
      gui: Defer removeAndDeleteWallet when no modal widget is active
##### wallet: Releases dangling files on BerkeleyEnvironment::Close
      Remove direct node->wallet calls in init.cpp
      [SECP256K1] CMake: add an option to enable endomorphism
      [SECP256K1] Travis : run a 64 bits ninja for building 32 bits targets
      always unpark even when -parkdeepreorg=0
      LastCommonAncestor: use skiplist when available
      AreOnTheSameFork: don't actually need to find common ancestor
      FinalizeBlockAndInvalidate: just use chainActive
      ConnectBlock : count sigops in one place
      make per-tx sigops limit contextual
      rework AcceptToMemoryPoolWorker sigops counting
      add test that coinbase sigops are limited
      Pass chain locked variables where needed
      Remove uses of cs_main in wallet code
      Pass chain and client variables where needed
      Add skeleton chain and client classes
      Remove ENABLE_WALLET from libbitcoin_server.a
      Prevent shared conf files from failing with different available options in different binaries
      [Tests] Suppress output in test_bitcoin for expected errors
      Add an option to set the functional test suite name
      move sigops counting from CheckBlock to ContextualCheckBlock
      split feature_block test into sigops and non-sigops parts
      split abc-p2p-fullblocktest into sigops and non-sigops parts
      add input sigchecks limit to STANDARD_SCRIPT_VERIFY_FLAGS (but not mempool flags)
      add a flag that (if unset) zeroes sigchecks reported by VerifyScript
      wallet: Add missing cs_wallet/cs_KeyStore locks to wallet
      gui: Also log and print messages or questions like bitcoind
      ui: Compile boost:signals2 only once
      tests: Reduce noise level in test_bitcoin output
      Merge #13982: refactor: use fs:: over boost::filesystem::
      Fix parent<->child mixup in UnwindBlock
      Add last missing part from PR12954
      Increase RPC timeout for the feature_assumevalid test
      build with -fstack-reuse=none
      Enable context creation in preallocated memory
      Make WINDOW_G configurable
      [DOC] Add CMake and Ninja to the dependency list
      Make last disconnected block BLOCK_FAILED_VALID, even when aborted
      Use trivial algorithm in ecmult_multi if scratch space is small
      Merge #9963: util: Properly handle errors during log message formatting
      Add instructions for verifying download integrity against release signer keys
      Bump version to 0.20.12
      [CMAKE] Attach the linker flags to target properties
      [CMAKE] Bump minimum version to 3.13
      [CMAKE] Fix the native build when the target is in the current build dir
      [SECP256K1] Use the cmake version from Kitware PPA on Travis
      CreateNewBlock: small tweaks
      CreateNewBlock: insert entries into block slightly earlier so that correct size is logged
      [CMAKE] Get rid of `add_compiler_flags_to_var`
      [CMAKE] Add a check_linker_flag function
      [cmake] Refactor native build cmake generation
      [cmake] Do not generate git_ignored_files.txt
      fix a test in anticipation of SCRIPT_VERIFY_INPUT_SIGCHECKS activation
      [abc-wallet-standardness] do test the signing error code
      Merge #14494: Error if # is used in rpcpassword in conf
      Merge #14413: tests: Allow closed rpc handler in assert_start_raises_init_error
      Merge #14105: util: Report parse errors in configuration file
      Merge #14146: wallet: Remove trailing separators from -walletdir arg
20.11
      [CMAKE] Fix static linkage when building for Windows
      Fix type mismatch for GetVirtualSizeWith<Descendants|Ancerstors>
      simplify ATMP standard flag computation [2/2] - move computation to another function
      simplify ATMP standard flag computation [1/2] - remove CHECKDATASIG_SIGOPS
      [cmake] Only set the native build marker once
      Add virtualsize computation to mempool
      track descendant sigops count in mempool
      tweak auto-unparking message
      [validation.cpp] parking-related comment tweaks
      [CMAKE] Run wallet tests as part of the check-bitcoin target
      Merge #13862: utils: drop boost::interprocess::file_lock
      [CMAKE] Fix getentropy detection on OSX
      [CMAKE] Fix daemon() detection on OSX
      Merge #14108: tests: Add missing locking annotations and locks (g_cs_orphans)
      Temporary fix for recent build flakiness
      Merge #13126: util: Add Clang thread safety annotations for variables guarded by cs_args
      Update manpages for 0.20.11 release
      Merge #12804: [tests] Fix intermittent rpc_net.py failure.
      Update seeds
      Update chainparams
      [cmake] Always run native build standalone
      Note intention of timing sidechannel freeness.
      configure: Use CFLAGS_FOR_BUILD when checking native compiler
      Respect LDFLAGS and #undef STATIC_PRECOMPUTATION if using basic config
      Make sure we're not using an uninitialized variable in secp256k1_wnaf_const(...)
      Pass scalar by reference in secp256k1_wnaf_const()
      Avoid implementation-defined and undefined behavior when dealing with sizes
      Guard memcmp in tests against mixed size inputs.
      Use __GNUC_PREREQ for detecting __builtin_expect
      Add $(COMMON_LIB) to exhaustive tests to fix ARM asm build
      Switch x86_64 asm to use "i" instead of "n" for immediate values.
      Allow field_10x26_arm.s to compile for ARMv7 architecture
      Clear a copied secret key after negation
      Use size_t shifts when computing a size_t
      Fix integer overflow in ecmult_multi_var when n is large
      Add trivial ecmult_multi algorithm which does not require a scratch space
      Make bench_internal obey secp256k1_fe_sqrt's contract wrt aliasing.
      travis: Remove unused sudo:false
      Summarize build options in configure script
      Portability fix for the configure scripts generated
      Correct order of libs returned on pkg-config --libs --static libsecp256k1 call.
      Eliminate scratch memory used when generating contexts
      Optimize secp256k1_fe_normalize_weak calls.
      Assorted minor corrections
      Make constants static: static const secp256k1_ge secp256k1_ge_const_g; static const int CURVE_B;
      secp256k1_fe_sqrt: Verify that the arguments don't alias.
      Make randomization of a non-signing context a noop
      add static context object which has no capabilities
      Fix algorithm selection in bench_ecmult
      Make use of TAG_PUBKEY constants in secp256k1_eckey_pubkey_parse
      improvements to random seed in src/tests.c
      Merge #15507: test: Bump timeout on tests
      Merge #13861: test: Add testing of value_ret for SelectCoinsBnB
      [secp256k1] [ECDH API change] Support custom hash function
      Merge #14056: Docs: Fix help message typo optiona -> optional
      [CMAKE] Use the same debug flags for C and C++
      [CMAKE] Remove useless remove_compile_flag in leveldb
      Revert "Prevent callback overruns in InvalidateBlock and RewindBlockIndex"
      [CMAKE] Add compiler flags to some build configuration only
      [secp256k1] fix tests.c in the count == 0 case
      Merge #13429: Return the script type from Solver
      [CI] Increase the coverage for the build-diff and build-master configs
      Optimization: don't add txn back to mempool after 10 invalidates
      [CI] Use ninja targets instead of calling binaries
      Move the functional test temporary directory under the build directory
      Move the JUnit file to the temporary directory
      Prevent callback overruns in InvalidateBlock and RewindBlockIndex
      [CMAKE] Add compiler flags to properties rather than CFLAGS/CXXFLAGS
      [CMAKE] Remove extra -fPIE flag
      [secp256k1] scratch: add stack frame support
      Revert "Call FinalizeBlockAndInvalidate without cs_main held"
      Call FinalizeBlockAndInvalidate without cs_main held
      Add a script to generate sha256sums from Gitian output
      Release cs_main during InvalidateBlock iterations
      Call InvalidateBlock without cs_main held
      Call RewindBlockIndex without cs_main held
      [wallet] Support creating a blank wallet
      add a flag that restricts sigChecks per-input
      save ScriptExecutionMetrics during CScriptCheck
      parameterize ecmult_const over input size
      Merge #12559: Avoid locking cs_main in some wallet RPC
      Add some braces to policy/policy.cpp
      [CI] Add a configuration to build and run the benchmarks
      [CI] Split build-default into build-diff and build-master
      [CMAKE] Add a check-upgrade-activated-extended target
      Merge #13988: Add checks for settxfee reasonableness
      Merge #13142: Separate IsMine from solvability
      move ScriptExecutionMetrics to its own file
      move MANDATORY_SCRIPT_VERIFY_FLAGS to policy.h
      [tests] remove test_bitcoin.h dependency on txmempool.h
      Fix -Wshadow warnings
      test: add "diamond" unit test to MempoolAncestryTests
      scripted-diff: Remove unused first argument to addUnchecked
##### Free BerkeleyEnvironment instances when not in use
      Fix the abc-p2p-compactblocks when running whith UBSAN
      Fix extra parenthesis in python .format()
      Mark CTxMemPoolEntry members that should not be modified const
      fix misc places that refer to virtual transaction size
      Reintroduce IsSolvable
      Merge #13002: Do not treat bare multisig outputs as IsMine unless watched
      document MANDATORY_SCRIPT_VERIFY_FLAGS accurately
      simplify checkdatasig_tests
      fix scriptSig analysis in sign.cpp
      [mempool_tests] add sigop counting check in TestPackageAccounting
      redefine virtual transaction size to something useful
      remove segwit 'sigops cost' leftovers
      Add keys to source package
      VerifyScript: accumulate ScriptExecutionMetrics and return them
      fix inappropriate uses of virtual size
      Wrap paths in codeblocks in release-process.md
      Increase sparsity of pippenger fixed window naf representation
      Remove unnecessary major.minor version from gitian-descriptors
####  Add Clang thread safety annotations for variables guarded by cs_db
      [CMAKE] Actually run the seeder tests
      [SECP256K1] Update Travis deprecated keywords
      [SECP256k1] Add the CMake/Ninja build to Travis
      [CMAKE] Add a check-extended target
      Log env path in BerkeleyEnvironment::Flush
XXXXX wallet: detecting duplicate wallet by comparing the db filename.
##### [bugfix] wallet: Fix duplicate fileid detection
##### [wallet] Reopen CDBEnv after encryption instead of shutting down
      Make ECM error message more helpful
##### wallet: Reset BerkeleyDB handle after connection fails
      Use best-fit strategy in Arena, now O(log(n)) instead O(n)
      Nits in rpcdump
      Various formating fix in validation.h
      [DEPENDS] Allow to limit the jobs when building packages with ninja
      [DEPENDS] Make the boost package build parallel
      [SECP256K1] Move the autotools Travis build to it's own script
      Added guide for OSX users on how to install clang-format-8.
      Do not import private keys to wallets with private keys disabled
      Add pippenger_wnaf for multi-multiplication
      [DEPENDS] Make the qt package build parallel
      [DEPENDS] Make bdb, dbus, event, miniupnpc, zmq packages build parallel
      [DEPENDS] Make qrencode, protobuf and zlib packages build parallel
      [DEPENDS] Make expat, fontconfig and freetype packages build parallel
      [DEPENDS] Make all the x* packages build parallel
      [DEPENDS] Make all the libX_* packages build parallel
      [DEPENDS] Make all the native_* packages build parallel
      [SECP256k1] Update the README to include the CMake build instructions
      [SECP256K1] Add a case with Schnorr disabled to the Travis matrix
      [CMAKE] Don't use C++ features for building secp256k1
      Bump version to 0.20.11
      [DEPENDS] Use parallel compilation when building packages
      Add a facility to extract libsecp256k1 from the repository
      Add Accelerate book to Bitcoin ABC reading list
      Clean up more instances of create_transaction()
      [CMAKE] Allow building secp256k1 as a standalone project
      [UBSAN] Fix UBSAN issue in test_bitcon_main.cpp
      [CMAKE] Cleanup secp256k1 module path
      [cmake] Use list append when adding path to CMAKE_MODULE_PATH
      [CI] Disable crypto assembly when building with ASAN
      Avoid custom main for passing arguments to the unit tests
      GUI: Change the receive button to respond to keypool state changing
      [Tests] Cleanup feature_block.py, remove unnecessary PreviousSpendableOutput object
      [Tests] Cleanup extra instances of create_transaction
      [Tests] Rename create_tx and move to blocktools.py
      Various nits in receivecoinsdialog.cpp
      Various nits in walletmodel.cpp
      [CMAKE] Fix DLL exports
      [cmake] Make bench-secp256k1 actually run benchmarks, not simply build them.
      Update secp256k1 README
      add nSigChecks counting to EvalScript
      [rpc] Finish backporting changes to prioritisetransaction's priority_delta argument
      Revert "[mining] Rename several CBlockTemplateEntry members for clarity"
20.10
      Merge #14568: build: Fix Qt link order for Windows build
      Update seeds
      Update manpages for 0.20.10 release
      Update chainparams
      Fix protobuf linking when building without the wallet
      finish backporting PR9602 - misc
      finish backporting PR9602 : remove unused priority number in mempool mapDeltas
      finish backporting PR9602 - test_bitcoin clean up unused param
      finish backporting PR9602 - remove unused and untested miner code
      finish backporting PR9602 - remove unused functional test framework code
      finish backporting PR9602 - remove unused modified-size computations
      Add sig files to Github release
      Fix nits in check-keys.sh
      Fix missing newline at end of keys file
      [CMAKE] Allow to run the extended functional tests
      [CMAKE] Make the functional test run with upgrade activated
      [CMAKE] Add a check-upgrade-activated magic target
      [tests] allow BOOST_CHECK_EQUAL for ScriptErrors
      [CMAKE] Separate the target for running the unit tests with upgrade
#     Merge #13791: gui: Reject dialogs if key escape is pressed
      refactor null-signature checks in OP_CHECKSIG, OP_CHECKDATASIG
      Backport PR11309: Minor cleanups for AcceptToMemoryPool
      [diagnostic] perform more aggressive checking of the index during ActivateBestChain
      Merge #15101: gui: Add WalletController
      Merge #14451: Allow building GUI without BIP70 support
      Fix heap-use-after-free in activation_tests
      Remove GetPriority from CCoinsViewCache
      Bump version to 0.20.10
      Remove priority from CTxMemPoolEntry
      kill CTxMemPoolEntry::GetPriority
      Remove deprecated startingpriority and currentpriority from RPC
      CompareIteratorByHash => CompareIteratorById
      Remove TxCoinAgePriorityCompare
      Do not log priority when using -printpriority
      [cmake] Use more sensible name for individual test targets
      Merge #13844: doc: correct the help output for -prune
      Merge #13412: Make ReceivedBlockTransactions return void
      Rename activation tests which are now feature tests
#     Merge #13824: doc: Remove outdated net comment
      Cleanup graviton activation
      Make the tests use phonon activation instead of graviton
#     Merge #13776: net: Add missing verification of IPv6 address in CNetAddr::GetIn6Addr(...)
#     Some formatting nits in netaddress.cpp

20.9
     [test-seeds] Stop the script if starting bitcoind failed
     Update seeds
#    Filter IPv6 by ASN
     Update manpages for 0.20.9 release
     Update chainparams
     Remove blockprioritypercentage config parameter.
     Add phonon activation logic
     [CMAKE] Override default flags
     Merge #13451: rpc: expose CBlockIndex::nTx in getblock(header)
#    [backport] net: Allow connections from misbehavior banned peers
     [CI] Don't override the default Junit file name
     [CMAKE] Unbreak the activated tests
#    [CMAKE] Add a status message to inform the user that ccache is used
     [AUTOTOOLS] Don't build the seeder tests if --disable-tests is set
#    Merge #13419: [tests] Speed up knapsack_solver_test by not recreating wallet 100 times.
     Merge #13775: doc: Remove newlines from error message
#    Merge #13773: wallet: Fix accidental use of the comma operator
     stop rewinding post-segwit blocks on startup
     [backport] gui: Show messages as text not html
#    Fix -Wcast-align in crypto_hash.cpp
     [LINTER] Enable new autopep8 rules
     [LINTER] Remove empty lines at the beginning of a block
     test: Add missing LIBBITCOIN_ZMQ to test_test_bitcoin_LDADD
     autoconf: Sane --enable-debug defaults.
     [CMAKE] Allow to run boost unit tests in parallel
     Static assert with no message is a C++17 feature and warns on C++14
     [LINTER] Fix the tests linter
     Add a separate executable for seeder tests
     Fix enum NumConnections shadowing warning
     checkmultisig: refactor nullfail check
     remove priority free transactions mechanism (currently off by default)
     fix some tests that rely on free transactions being submittable via RPC
     Replace integer literals in dnshandle() with an enum class
     clean up some tests that needlessly use -replayprotectionactivationtime
     Add test-seeds CI configs
     Fetch and check signature and sha256sum of debian ISO instead of comparing against a hardcoded hash
     [DOC] Update build documentation and contributing to use cmake and ninja
#    Merge #14350: Add WalletLocation class
     [CI] Always move the Gitian install and build log
     Update Debian 10.x ISO link in the Gitian setup docs
     We assume uint8_t is an alias of unsigned char.
     Merge #14474: bitcoin-tx: Use constant for n pubkeys check
     Merge #11866: Do not un-mark fInMempool on wallet txn if ATMP fails.
     test: Move main_tests to validation_tests
     Merge #14206: doc: Document -checklevel levels
#    Merge #13534: Don't assert(foo()) where foo() has side effects
     Merge #13662: Explain when reindex-chainstate can be used instead of reindex
     Fixes AES benchmarks
     qt: Replace objc_msgSend with native syntax
     [CMAKE] Fix wrong Openssl include directory variable name
     add SCHNORR_MULTISIG to mandatory flags
     Make more script validation flags backward compatible
     fix fee estimation bug in functional tests
     fix some tests that misuse MANDATORY_SCRIPT_VERIFY_FLAGS
     Sanity check for mempool acceptance when doing standardness checks
     Merge #13656: Remove the boost/algorithm/string/predicate.hpp dependency
     Daemonize bitcoind in test-seeds.sh rather than run in a background process
XXXX Merge #13667: wallet: Fix backupwallet for multiwallets
     refactor: replace qLowerBound & qUpperBound with std:: upper_bound & lower_bound
     Merge #13633: Drop dead code from Stacks
     [CI] Migrate the CI to use cmake and ninja
     [CMAKE] Fix Qt tests when the wallet is not built
     Remove unused functions in seeder/db.h
     [UBSAN] Update exceptions
     [CMAKE] SYS_getrandom is expected to be linux only
     [CMAKE] Fix FindBerkeleyDB for FreeBSD
#    [CMAKE] Enable SSE4.1, SHA-NI and AVX2 for the crypto library
     reduce code duplication in UpdateFlags
     Merge #13298: Net: Bucketing INV delays (1 bucket) for incoming connections to hide tx time
     [wallet] Add wallet name to log messages
     [tests] Fix race in rpc_deprecated.py
     add SCRIPT_VERIFY_MINIMALDATA to mandatory flags
     Add braces to support/lockedpool.cpp
     Abstract EraseBlockData out of RewindBlockIndex
     [CI] Print the sanitizer logs
     [CI] Move the sanitizer log directory to /tmp
     [CMAKE] Add an option to promote some warnings to errors
     Remove Unused CTransaction tx in wallet.cpp
     Privatize CWallet::AddToWalletIfInvolvingMe
     Extract CWallet::MarkInputsDirty
     [CMAKE] Silent the Qt translation files generation
     allow cuckoocache to function as a map
     Merge #12944: [wallet] ScanforWalletTransactions should mark input txns as dirty
     Log debug build status and warn when running benchmarks
     bench_bitcoin: Avoid read/write to default datadir
     test_bitcoin: Avoid read/write to default datadir
     Merge #13074: [trivial] Correct help text for `importaddress` RPC
#    Merge #13500: [wallet] Decouple wallet version from client version
     partial revert of tx decode sanity check backport
     fix incomplete txvalidationcache_tests
     Fix cuckoocache_tests -Wcast-align warnings
     [cuckoocache] Use getKey and KeyType for contains
     Merge #13627: Free keystore.h from file scope level type aliases
     Merge #13603: bitcoin-tx: Stricter check for valid integers
     [move only] Move BIP70 code together in preparation to backport PR14451 BIP70 changes
     Sanity-check script sizes in bitcoin-tx
     Fix unused variable warning when building with wallet disabled
     Bump version to 0.20.9
20.8     
     Update seeds
     Update manpages for 0.20.8 release
     Update chainparams
     Remove errant --testnet argument in chainparams README
     Fix apptest build failure on Xenial
     Small fix in CONTRIBUTING.md - clang-format-7 still mentioned, but 8 is required
     Small fix in backporting.md - remove unneeded trailing . for git remote add command
     Fix missing blockhash cast in wallet_tests.cpp
     Add wallet acceptance / mempool acceptance tests for non-standard variants
     Fix a bug where test-autopatch would fail when the local master branch does not have the same HEAD as origin/master
     Fix a bug where test-autopatch would fail in an env where no global git config is set
     Fix ninja check with TEST_WITH_UPGRADE_ACTIVATED for newer versions of boost
     [CMAKE] Make the source control tools inclusion an option
     Merge #13566: Fix get balance
     Remove check-source-control-tools from check and check-all targets
     [cuckoocache] Use matchKey instead of the == operator.
     refactoring: add a method for determining if a block is pruned or not
     Integrate gArgs and chainparams into the Seeder
     Various nits in cuckoocache_tests.cpp
     doc: Doxygen-friendly CuckooCache comments
     Rename contrib/arcanist to source-control-tools
     Various nits in Seeder files
     Merge #11293: Deduplicate CMerkleBlock construction code, add test coverage
     Fix -Wunused-const-variable in descriptor_tests.cpp
     Bugfix: Include <memory> for std::unique_ptr
     fix undefined behaviour in seeder (select() aliasing)
     Update the github issue template to include disclosure policy
     remove NULLDUMMY
     refactor: Add and use HaveTxsDownloaded() where appropriate
     Added autopatch script for patching and rebasing phabricator diffs
     Make TxIndex::FindTx use BlockHash
     Update GetTransaction's parameters
     Update mempool and compact block logic to use TxHash
     tx pool: Use class methods to hide raw map iterator impl details
     Update mempool's mapDelta to use TxId
     Update mempool's mapTx to index from TxId.
     Use TxId in setInventoryTxToSend
     refactor: Drop boost::this_thread::interruption_point and boost::thread_interrupted in main thread
     Use BlockHash in BlockTransactionsRequest
     [cmake] link test runners by default
     Drop minor GetSerializeSize template
     Avoid creating a temporary vector for size-prefixed elements
#    Drop unused GetType() from CSizeComputer
     validation: assert that pindexPrev is non-null when required
     [CMAKE] Avoid rebuilding sec256k1
     [CMAKE] Fix scope issue in the remove_<lang>_compiler_flags() functions
#    Rationalize lock anotation in validation code
     tests: Add missing cs_main locks required when accessing pcoinsdbview, pcoinsTip or pblocktree
     Introduce BlockHash to represent a block hash
     Add braces in block.h
     Use size_t where apropriate in skiplist_tests.cpp
     Add Benchmark to test input de-duplication worst case
     Add const modifier to HTTPRequest methods
     Add braces in txdb.cpp
     Move pindexFinalized in CChainState
#    Explain GetAncestor check for m_failed_blocks in AcceptBlockHeader
#    Remove unnecessary const_cast
     Fix activation_tests
     Add fuzz testing for BlockTransactions and BlockTransactionsRequest
     [test] Speed up fuzzing by ~200x when using afl-fuzz
     [CMAKE] Build test_bitcoin_fuzzy
     drop 'check3' upgrade-conditional-script-failure for Schnorr multisig
     Nit in net_processing.cpp
     Backport PR14897, PR15834 and PR16196
     Merge #15149: gui: Show current wallet name in window title
     Update timings.json
     Various nits in net_processing.cpp
#    p2p: Clarify control flow in ProcessMessage()
#    Backport of Core PR14728: fix uninitialized read when stringifying an addrLocal
     previous link was dead
     Merge #14784: qt: Use WalletModel* instead of the wallet name as map key
     test: Fix test failures
     qa: fix deprecated log.warn in feature_dbcrash test
     [tests] fix block time in feature_pruning.py
     [tests] make pruning test faster
     [tests] style fixes in feature_pruning.py
     [test] Rename rpc_timewait to rpc_timeout
     Minor improvements to add_nodes
     qa: Extract rpc_timewait as test param
     Fix copy in loop
     Trivial: Corrected comment array name from pnSeeds6 to pnSeed6
     Merge #13498: [wallet] Fixups from account API deprecation
     Merge #12639: Reduce cs_main lock and avoid extra lookups of mapAddressBook in listunspent RPC
     [wallet] [rpc] Remove getlabeladdress RPC
     Fix wrong version in clang-format error message and update the doc
     Merge #13138: [tests] Remove accounts from wallet_importprunedfunds.py
     Merge #13437: wallet: Erase wtxOrderd wtx pointer on removeprunedfunds
     Add test coverage for messages requesting invalid blocks
     Drop IsLimited in favor of IsReachable
#    Remove undue lock assertion in GuessVerificationProgess
     Revert use of size_t in ParseParameters
     Fixes broken link to disclosure policy
     [tests] Remove 'account' API from wallet_listsinceblock
     [tests] Remove 'account' API from wallet_basic
     Move to clang format 8
     Added support for -gravitonactivationtime to unit tests
     [tests] Remove 'account' API from wallet_txn_doublespend
     [tests] Remove 'account' API from wallet_txn_clone
     [tests] Remove 'account' API from wallet_listtransactions
     [tests] Remove 'account' API from wallet_keypool_topup
     [tests] Remove 'account' API from wallet_import_rescan
     Merge #13055: qt: Don't log to console by default
     [cmake] Add comments to express what tests do.
     [cmake] Remove useless copy of create_cache.py
     [tests] Remove 'account' API from wallet_listreceivedby
     Split out key-value parsing in ArgsManager into its own function
     IsReachable is the inverse of IsLimited (DRY). Includes unit tests
     Bump version to 0.20.8
     [cmake] Use terminal when runnign integration tests
     Rename GetLogger() to LogInstance()
     Add missing parts from PR12954
     Use RdSeed when available, and reduce RdRand load
     Print to console by default when not run with -daemon
     Improve formatting in rpcwallet.cpp
#     Leftover from PR13423
     Stop translating command line options
     minor refactor to use ranged_for, auto and const-ness
     test: Make bloom tests deterministic
     qa: Increase includeconf test coverage
     Bump minimum Qt version to 5.5.1
     Add BitcoinApplication & RPCConsole tests
     Kill AddKeypathToMap

20.7 
      Update manpages for 0.20.7 release
      Update chainparams
      Update seeds
      util: Add [[nodiscard]] to all {Decode,Parse}[...](...) functions returning bool
      Make SignPSBTInput operate on a private SignatureData object
      Pass HD path data through SignatureData
      Implement key origin lookup in CWallet
      wallet: Fix non-determinism in ParseHDKeypath(...). Avoid using an uninitialized variable in path calculation.
      Generalize PublicOnlySigningProvider into HidingSigningProvider
      More tests of signer checks
      Update valid PSBT test vectors so that they properly use the value 0 for utxos's key
      Add aarch64 qt depends support for cross compiling bitcoin-qt
      [CMAKE] Migrate windows gitian build to cmake and ninja
      [GITIAN] Migrate OSX Gitian builds to CMake and Ninja
      [CMAKE] Migrate linux gitian build to cmake and ninja
      Allow to pass the chain parameters when formatting a bitcoin URI
      Move BitcoinApplication to header so it can be tested
#     Don't use systray icon on inappropriate systems
      add a couple more Schnorr checkmultisig tests
      Make SigningProvider expose key origin information
      [MOVEONLY] Move ParseHDKeypath to utilstrencodings
      Introduce KeyOriginInfo for fingerprint + path
      Merge #9662: Add createwallet "disableprivatekeys" option: a sane mode for watchonly-wallets
      Use local instance of ArgsManager in getarg_tests
      [CMAKE] Add DBus support to bitcoin-qt
      qt: All tray menu actions call showNormalIfMinimized
      qt: Use GUIUtil::bringToFront where possible
      qt: Add GUIUtil::bringToFront
#     Remove obj_c for macOS Dock icon menu
      Use Qt signal for macOS Dock icon click event
      [CMAKE] Use a NSIS custom template
      Test that a non-witness script as witness utxo is not signed
      Use 72 byte dummy signatures when watching only inputs may be used
      Use 71 byte signature for DUMMY_SIGNATURE_CREATOR
      Always create 70 byte signatures with low R values
      Additional sanity checks in SignPSBTInput
#     Introduce a maximum size for locators.
      test: Add tests for RPC help
      Remove obj_c for macOS Dock icon setting
      gui: Favor macOS show / hide action in dock menu
      Add missing braces on key.cpp
      qa: Premine to deterministic address with -disablewallet
      RPC: Add new getzmqnotifications method.
#     Break circular dependency: init -> * -> init by extracting shutdown.h
#     Drop unused init.h includes
#     Add Windows shutdown handler
      Add checkpoints for graviton upgrade
      [github-release] Add optional param to specify release notes path
      Fix merging of global unknown data in PSBTs
      Check that PSBT keys are the correct length
      Add brace in bitcoin-tx.cpp
      Fix PSBT error test vectors
      Add outputtype module
      [CMAKE] Use a stripped binary to build the dist and DMG for OSX
      [CMAKE] Only build the bitcoin-qt application bundle on OSX
      [CI] Add a new build configuration to run tests with cmake and ninja
      [CI] Rename build.sh to build_autotools.sh
      Merge #14025: p2p: Remove dead code for nVersion=10300
      fix txvalidationcache_tests to not rely on NULLDUMMY
      Various nits in the ChainParams code.
      Added missing instructions for OSX gitian building
      Various nits in cuckoocache.h
      [CMAKE] Avoid dependencies when building native bin during cross build
      Add/update copyright lines to top of seeder code
      Added CMake function to detect BOOST_TEST_DYN_LINK
      Add descriptor reference documentation
      Swap in descriptors support into scantxoutset
      Output descriptors module
      [CMAKE] Actually build the DMG image
      Get rid of ambiguous OutputType::NONE value
      Fix unecessary copies in rpcwallet.cpp
      Add scantxoutset RPC method
      [CMAKE] Install DLL to bin/ by default
      [CMAKE] Complete the dist directory to prepare for the DMG image build
      [CMAKE] Build the background dist image on native OSX
      [CMAKE] Add the background image to the dist directory
      [CMAKE] Create a distribution directory with translations
      [CMAKE] Prepare the application bundle for localization
      Add simple FlatSigningProvider
      Tests for PSBT
      Create wallet RPCs for PSBT
      Bump version to 0.20.7
      Document RNG design in random.h
      Use secure allocator for RNG state
      Encapsulate RNGState better
      DRY: Implement GetRand using FastRandomContext::randrange
      Remove hwrand_initialized.
      Sprinkle some sweet noexcepts over the RNG code
      [CI] Run the thread sanitizer as part of the CI
      Switch all RNG code to the built-in PRNG.
      Integrate util/system's CInit into RNGState
20.6      
      Updated manpages for 0.20.6 release
      Updated chainparams
      Updated seeds
      Added a release note regarding builds needing python 3.5
      Abstract out seeding/extracting entropy into RNGState::MixExtract
      Add thread safety annotations to RNG state
      Rename some hardware RNG related functions
      Automatically initialize RNG on first use.
      Don't log RandAddSeedPerfmon details
      Use FRC::randbytes instead of reading >32 bytes from RNG
      [CMAKE] Generate the split-debug.sh script
      Create utility RPCs for PSBT
      [CMAKE] Use native strip on OSX
      build: depends: Switch to python3
      depends: biplist 1.0.3
      Upgrade mac_alias to 2.0.7
      [depends] mac_alias 2.0.6, ds_store 1.1.2
      Add more methods to Span class
      Fix comment layout in client.h
      Deprecate wallet 'account' API
      [wallet] Deprecate account RPC methods
      [wallet] [rpc] Remove duplicate entries in rpcwallet.cpp's CRPCCommand table
      [tests] Rename rpc_listtransactions.py to wallet_listtransactions.py
      Merge #12892: [wallet] [rpc] introduce 'label' API for wallet
      Merge #9894: remove 'label' filter for rpc command help
      test: Remove python3.4 workaround in feature_dbcrash
      build: Require python 3.5
      Run all extended tests on master for build-default
      Display the default values as part of the test_runner.py help
      scripted-diff: Update copyright in ./test
      Fix various linter issues
      scripted-diff: test: Remove brackets after assert
      scripted-diff: test: Use py3.5 bytes::hex() method
      [GITIAN] Sort dependencies lexically
      [GITIAN] Remove symbolic link to asm for 32-bits gitian build
      test: Add lint to prevent SIGNAL/SLOT connect style
      [contrib] Support ARM symbol check
      Migrate Gitian container to Debian 10 Buster
      Various fixups for PR13557
      [CMAKE] Move FDELT_TYPE declaration to config
#     Update qt/optionsdialog.cpp with Qt5 connect syntax
      Update qt/walletview.cpp with Qt5 syntax
      Update qt/test/paymentservertests.cpp to use Qt5 connect syntax
      Update qt/walletmodel.cpp with Qt5 connect syntax
#     Update qt/bitcoin.cpp with Qt5 connect syntax
#     Update qt/qvalidatedlinedit.cpp to use Qt5 connect syntax
      Update qt/walletframe.cpp with Qt5 connect syntax
      Update qt/bitcoingui with Qt5 connect syntax
      [CMAKE] Fix missing inclusion when libbitcoinconsensus is disabled
      Update qt/transactionview.cpp with Qt5 connect syntax
      Update qt/sendcoinsdialog.cpp with Qt5 connect syntax
      Update qt/rpcconsole with Qt5 connect syntax
#     Update qt/recentrequestdialog.cpp with Qt5 connect syntax
#     Update qt/receivecoinsdialog.cpp with Qt5 connect syntax
#     Update qt/transactiontablemodel.cpp with Qt5 connect syntax
#     Update qt/trafficgraphwidget.cpp with Qt5 connect syntax
      Update qt/sendcoinsentry.cpp with Qt5 connect syntax
#     Update qt/qvaluecombobox.cpp to use Qt5 connect syntax
#     Update qt/receiverequestdialog.cpp with Qt5 connect syntax
      Update user agent filter in makeseeds.py
#     Update qt/peertablemodel.cpp with Qt5 connect synax
      Update qt/paymentserver.cpp with Qt5 connect syntax
      Update qt/overviewpage.cpp with Qt5 connect syntax
#     Update qt/modaloverlay.cpp with Qt5 connect syntax
#     Update qt/intro.cpp to use Qt5 connect syntax
      Update qt/guiutil.cpp to use Qt5 connect syntax
      Update qt/coincontroldialog.cpp to use Qt5 connect syntax
      SignPSBTInput wrapper function
      Methods for interacting with PSBT structs
      Add pubkeys and whether input was witness to SignatureData
      Implement PSBT Structures and un/serialization methods per BIP 174
      [CMAKE] Build the OSX application bundle for bitcoin qt
      [CMAKE] Create a windows installer
      [CMAKE] Build bitcoinconsensus library both static and shared
      Add testnet-seed.bchd.cash to seeder lists
#     Update qt/clientmodel.cpp to use Qt5 syntax
      Merge #12924: Fix hdmaster-key / seed-key confusion
      Rename master key to seed
      Update qt/bitcoinamountfield.cpp to use Qt5 connect syntax
      Update qt/askpassphrasedialog.cpp to use Qt5 connect syntax
      Update qt/addressbookpage.cpp to use Qt5 connect syntax
#     rpc: Expose g_is_mempool_loaded via getmempoolinfo
#     Refactor transaction creation and transaction funding logic
      Separate CSeederNode class declaration from definition
      Move parse_name() to dns.h so it can be tested
      Bump version to 0.20.6
      Merge #13481: doc: Rewrite some validation docs as lock annotations
20.5      
      Updated manpages for 0.20.5 release
      Updated chainparams
      Update seeds
      bench: Benchmark MempoolToJSON
#     Merge #14444: Add compile time checking for cs_main locks which we assert at run time
      Merge #13114: wallet/keystore: Add Clang thread safety annotations for variables guarded by cs_KeyStore
      Add test_runner flag to suppress ASAN errors from wallet_multiwallet.py
      [CMAKE] Move package name and copyright to the top level
      Added build-werror config to error on build warnings
      Merge #13248: [gui] Make proxy icon from statusbar clickable
      Merge #13043: [qt] OptionsDialog: add prune setting
      Fix -Wrange-loop-analysis warnings
      Fix -Wthread-safety-analysis warnings
      [CMAKE] Use CPack to build source packages
      [CMAKE] Use CPack to build packages
#     mempool: remove unused magic number from consistency check
#     Merge #13258: uint256: Remove unnecessary crypto/common.h dependency
      Merge #11491: [gui] Add proxy icon in statusbar
XXXXX bugfix: Remove dangling wallet env instance and Delete walletView in WalletFrame::removeWallet
      ui: Support wallets unloaded dynamically
      rpc: Add unloadwallet RPC, release notes, and tests
      rpc: Extract GetWalletNameFromJSONRPCRequest from GetWalletForJSONRPCRequest
#     [mempool] Mark mempool import fails that were found in mempool as 'already there'
      [CMAKE] Propagate requirements for cmake >= 3.12
      Merge #11050: Avoid treating null RPC arguments different from missing arguments
      Merge #11191: RPC: Improve help text and behavior of RPC-logging.
      Merge #11626: rpc: Make `logging` RPC public
      [rpc] Add logging RPC
      Change CDnsSeedOpts to use std::string instead of c-strings
      Introduce constant default variables to initialize seeder options
      Remove strlcpy.h
      Fix misnamed 0.20.4 release notes
      [CMAKE] Add resource file to bitcoin-qt
#     Merge #13722: trivial: Replace CPubKey::operator[] with CPubKey::vch where possible
      Fix --wipeignore and add message indicating if wipe options are set and successful
#     Decodehextx scripts sanity check
      Update seeder files to use fprintf() instead of printf()
#     Merge #9598: Improve readability by removing redundant casts to same type (on all platforms)
      Merge #13275: Qt: use [default wallet] as name for wallet with no name
      Merge #13506: Qt: load wallet in UI after possible init aborts
      Merge #13564: [wallet] loadwallet shouldn't create new wallets.
      Merge #13097: ui: Support wallets loaded dynamically
      Merge #13273: Qt/Bugfix: fix handling default wallet with no name
      Version Bump to v20.5
      [gitian-build] Move manifest files to output directory
      [gitian-build] Refactor to calculate output directories in fewer places
20.4
      Update manpages for 0.20.4 release
      Update chainparams
      Update seeds
      Do not cache version in cmake build
      mempool, validation: Explain cs_main locking semantics
      [build-configurations] Make BUILD_DIR configurable
      [LINTER] Check for header guard closure comment
      Merge #13058: [wallet] `createwallet` RPC - create new wallet at runtime
      Update Seeder to use fsbridge::fopen() instead of fopen()
XXXXX Make objects in range declarations immutable by default. Avoid unnecessary copying of objects in range declarations.
#     cli: Ignore libevent warnings
      Merge #13252: Wallet: Refactor ReserveKeyFromKeyPool for safety
      [build-configurations] Resurface the more informative error message when ABC_BUILD_NAME is not set
      Added translations for new send coins dialog box
      Merge #13158: [Qt]: Improve sendcoinsdialog readability
      Bump wallet version for pre split keypool
      Allow -upgradewallet to upgradewallets to HD and use a keypool of presplit keys after upgrading to hd chain split
#     Remove redundant assignments (dead stores)
#     Drop ParseHashUV in favor of calling ParseHashStr
      Add 'sethdseed' RPC to initialize or replace HD seed and test
      Remove CombineSignatures and replace tests
      Replace CombineSignatures with ProduceSignature
      Make SignatureData able to store signatures and scripts
      Drop UpdateTransaction in favor of UpdateInput
      Generic TransactionSignatureCreator works with both CTransaction and CMutableTransaction
      Introduce Span type and use it instead of FLATDATA
      [cmake] Create a lib specifically for script related components
      Inline Sign1 and SignN
      Separate HaveKey function that checks whether a key is in a keystore
#     Merge #13176: Improve CRollingBloomFilter performance: replace modulus with FastMod
      Add native support for serializing char arrays without FLATDATA
      refactor: Avoid locking tx pool cs thrice
#     Return void instead of bool for functions that cannot fail
      Relayout comment in sign.h
#     Merge #11411: script: Change SignatureHash input index check to an assert.
      Minor improvements to github-release script
#     validation: Pass tx pool reference into CheckSequenceLocks
#     utils: Convert Windows args to utf-8 string
#     Merge #12240: [rpc] Introduced a new `fees` structure that aggregates all sub-field fee types denominated in BCH
      Change CI builds to use utf-8 encoding
      Added support for `export LC_ALL=C.UTF-8` to the shell linter
      Updated release-process to use github-release.sh
      Added more explicit instructions for release tagging
      Bump version to 0.20.4
      Add phpcs exclusion for strict_types declararion
      Add static_assert to prevent VARINT(<signed value>)
      [CMAKE] Enable Large File Support on platforms that don't enable it by default
      Add support for Glibc version 2.28
      util: Replace boost::signals2 with std::function
      Added a script for creating Github release drafts
      doxygen: Fix member comments
      [CMAKE] Use CMake built-in to set PIC and PIE
      [CMAKE] Fix check-security to allow running on windows executables
      Use only 3 levels for defining the version
#     tiny refactor for ArgsManager
#     Document RPC method aliasing
#     Add AssertLockHeld assertions in CWallet::ListCoins
      Merge #13304: qa: Fix wallet_listreceivedby race
      Merge #13284: gui: fix visual "overflow" of amount input.
20.3
      Update seeds
      Updated manpages for 0.20.3 release
      Updated chainparams
      Added README for generating chainparams constants
#     Avoid locking mutexes that are already held by the same thread
      Added net debug info to IBD builds
      Merge #12265: [test] fundrawtransaction: lock watch-only shared address
      Added a script to generate chainparams constants from intermediate files
      [CMAKE] Detect if the compiler supports visibility function attribute
      [CMAKE] Install executables
      [CMAKE] Install the man pages
      [CMAKE] Add the check-security target
      [CMAKE] Add the check-symbols target
#     Move cs_main locking annotations from .cpp to .h
      Merge #11220: Check specific validation error in miner tests
      Remove redundant variables, statements and forward declarations
      Fix errant newline in make_chainparams
      [CMAKE] Consistently find and use the python interpreter
      [CMAKE] Migrate the python header generation scripts to python 3
      [CMAKE] Only change obj/build.h if the content has changed
      Run miner_tests with fixed options
      Abstract out BlockAssembler options
      Fix compiler warnings emitted when compiling under stock OpenBSD 6.3
      Merge #13201: [qa] Handle disconnect_node race
      Merge #13402: Document validationinterace callback blocking deadlock potential.
      Revert change to PreciousBlock() comment made in D1182
      Fix missing newline in make_chainparams
      [RPC] Fix header guard comment
      Merge #13197: util: warn about ignored recursive -includeconf calls
#     Merge #13079: Fix rescanblockchain rpc to properly report progress
      Merge #13012: [doc] Add comments for chainparams.h, validation.cpp
      Merge #12716: Fix typos and cleanup in various files
      wallet: Use shared pointer to retain wallet instance
#     scripted-diff: Replace boost::bind with std::bind
#     refactor: Use boost::scoped_connection in signal/slot, also prefer range-based loop instead of std::transform
      Use TxId where apropriate in wallettests.cpp
      [CMAKE] Fix linux cross compilation with the glibc compatibility
      [CMAKE] Remove useless dependency for the check-rpcauth target
#     scripted-diff: batch-recase BanMan variables
#     banman: Add, use CBanEntry ctor that takes ban reason
#     banman: reformulate nBanUtil calculation
#     banman: add thread annotations and mark members const where possible
#     scripted-diff: batch-rename BanMan members
#     net: move BanMan to its own files
#     banman: pass in default ban time as a parameter
#     banman: pass the banfile path in
#     banman: create and split out banman
      Move util files to directory
#     Use C++11 default member initializers
      debug log number of unknown wallet records on load
#     Add compile time verification of assumptions we're currently making implicitly/tacitly
#     Use std::make_unique more consistently
#     bench: Use non-throwing ParseDouble(...) instead of throwing boost::lexical_cast<double>(...)
      [qt] send: Clear All also resets coin control options
      [qt] Replaces numbered place marker %2 with %1.
      Remove unecessary obj folder
      [CMAKE] Always build obj/build.h
      Revert change to ActivateBestChain() comments made in D1182
#     Merge #13234: Break circular dependency: chain -> pow -> chain
      Partial Merge #12920: test: Fix sign for expected values
#     Merge #13431: validation: count blocks correctly for check level < 3
#     Merge #13428: validation: check the specified number of blocks (off-by-one)
#     Merge #12885: Reduce implementation code inside CScript
#     net: split up addresses/ban dumps in preparation for moving them
      depends: qt: avoid system harfbuzz and bz2
      [gitian-build] Change output file destinations to separate directories for each platform
      Merge #13022: [qa] Attach node index to test_node AssertionError and print messages
      Merge #15239: scripts and tools: Move non-linux build source tarballs to "bitcoin-binaries/version" directory
      Fix avalanche test with boost 1.58
#     Remove GetNextBlockScriptFlags's requirement to hold cs_main
#     Add compile time checking for all cs_main runtime locking assertions
#     Use C++11 default member initializers
#     Drop unused pindexRet arg to CMerkleTx::GetDepthInMainChain
#     Merge #13149: Handle unsuccessful fseek(...):s
      [CI] Workaround ubsan failure in functional tests
      [CI] Refactor the build configuration by moving out tests from the build
      Add recommendation: By default, declare single-argument constructors `explicit`
      Nits in httpserver.cpp
#     Move SocketHandler logic to private method.
      Update the agent pattern filter in the makeseeds.py script
      [CMAKE] Add support to build secp256k1 JNI binding and tests
#     Move InactivityCheck logic to private method.
#     Move DisconnectNodes logic to private method.
#     Move NotifyNumConnectionsChanged logic to private method.
      tests: remove member connman/peerLogic in TestingSetup
      [SECP256K1] Create a different library when building with JNI
#     Convert comments to thread safety annotations
#     net: Add Clang thread safety annotations for guarded variables in the networking code
#     Report minfeefilter value in getpeerinfo rpc
      Prefer wait_until over polling with time.sleep
      [SECP256K1] Build java class files out of tree
      Add seed.bchd.cash to seeder lists
      Bump version to 0.20.3
#     net: Break disconnecting out of Ban()
#     Improve reliability of avalanche test
      [ibd.sh] Fix IBD progress logging
      [trivial,doc] Fix memory consistency model in comment
#     [qt] coincontrol: Remove unused qt4 workaround
      [build] .gitignore: add QT Creator artifacts
      Adding test case for SINGLE|ANYONECANPAY hash type in tx_valid.json
      Remove TestBlockValidity's dependency on Config
      Remove ConnectBlock's dependency on Config
      Remove CheckBlock's dependency on Config
      [net] Tighten scope in net_processing
#     Add documentation to PeerLogicValidation interface and related functions
20.2      
      [CI] Fix missing parameters transfer from ibd.sh to bitcoind
      Update manpages for 0.20.2 release
      Update chainparams
#     Merge #12159: Use the character based overload for std::string::find.
#     Merge #13983: rpc: Return more specific reject reason for submitblock
#     Merge #13399: rpc: Add submitheader
#     Merge #13439: rpc: Avoid "duplicate" return value for invalid submitblock
      Updated seeds
#     Fixes compilation of leveldb tests broken in D4004
      [CI] Make IBD a standard build configuration
      Remove ContextualCheckBlock's dependency on Config
      Remove CheckBlockHeader's dependency on Config
      [CMAKE] Fix Linux64 toolchain name
#     Source the Excessive block size from BlockValidationOptions
#     Do not construct out-of-bound pointers in SHA2 code
      Avoid triggering undefined behaviour (std::memset(nullptr, 0, 0)) if an invalid string is passed to DecodeSecret(...)
#     Pull leveldb subtree
      [CMAKE] Move version to the top level CMakeLists.txt
      [CMAKE] Rename the top-level and `src/` cmake projects
      Generalized ibd.sh to provide a logging tool for running similar types of tests
      Modify ContextualCheckBlockHeader to accept a CChainParam rather than a Config
      Remove ReadBlockFromDisk's dependency on Config
      Remove dependency on Config from the PoW code
      Use Consensus::Params in ContextualCheckTransaction and variations instead of Config
      Activate consensus rule based on consensus params rather than config
      Add warning about redundant moves
      Remove redundant call to std::move
      Fix the build-osx target for the depends subsystem
      Merge #12853: qa: Match full plain text by default
      Added a script to test seeds
      Added some release notes
      [CI] Improve error catching and build log verbosity
#     Merge #10537: Few Minor per-utxo assert-semantics re-adds and tweak
      Merge #12928: qt: Initialize non-static class members that were previously neither initialized where defined nor in constructor
      Merge #13747: tests: Skip P2PConnection's is_closing() check when not available
      Merge #13916: qa: wait_for_verack by default
      [SECP256K1] JNI tests : remove dependency to obsolete DatatypeConverter
      [TRIVIAL] Cleanup the JNI test file
      [Linter] Check the log prints are terminated with a newline
      [LINTER] Enforce using C++ style for void parameters
      Bugfix: NSIS: Exclude Makefile* from docs
      Merge #12503: [RPC] createmultisig no longer takes addresses
      Merge #13658: [moveonly] Extract RescanWallet to handle a simple rescan
#     Merge #11338: qt: Backup former GUI settings on `-resetguisettings`
#     [rebase] threads: fix unitialized members in sched_param
      Update univalue subtree
#     Merge leveldb subtree
#     Bump leveldb subtree
      Merge #13517: qa: Remove need to handle the network thread in tests
      Merge #11818: I accidentally [deliberately] killed it [the ComparisonTestFramework]
      Merge #13512: [qa] mininode: Expose connection state through is_connected
      Fix incorrectly backported return statements in mininode.py
#     Increase LevelDB max_open_files unless on 32-bit Unix.
      Remove extra newline that leads to linter warning
#     Added a script to generate chainparams intermediate files
      wallet: Catch filesystem_error and raise InitError
#     During IBD, when doing pruning, prune 10% extra to avoid pruning again soon after
#     Merge #13081: wallet: Add compile time checking for cs_wallet runtime locking assertions
#     Merge #11044: [wallet] Keypool topup cleanups
      Migrated abc-p2p-compactblocks.py off of ComparisonTestFramework
      Migrated abc-p2p-fullblocktest.py off of ComparisonTestFramework
      Migrated abc-mempool-coherence-on-activations.py off of ComparisonTestFramework
      Migrated abc-transaction-ordering.py off of ComparisonTestFramework
#     Merge #13077: Add compile time checking for all cs_KeyStore runtime locking assertions
      Add missing override keyword to DummySignatureCreator::CreateSig()
#     Merge #13159: Don't close old debug log file handle prematurely when trying to re-open (on SIGHUP)
#     Merge #13148: logging: Fix potential use-after-free in LogPrintStr(...)
#     Default to defining endian-conversion DECLs in compat w/o config
#     Consistently log CValidationState on failure
      Remove deprecated features for the 0.20.x branch
      Merge #12507: Interrupt rescan on shutdown request
      Make sure LC_ALL=C is set in all shell scripts
      Fix deprecated copy warnings in amount.h
      Bump version to 0.20.2
20.1
      Update manpages for 0.20.1 release
      Update chainparams
#     Merge #12923: util: Pass pthread_self() to pthread_setschedparam instead of 0
#     Merge #12618: Set SCHED_BATCH priority on the loadblk thread.
      Update seeds
      Add some more release notes for 0.20.1
      Fix running teamcity builds when multiple configure flags are set
      Fix shellcheck version >= 0.5.0 errors
      [LINTER] Enforce using `#!/usr/bin/env bash` for shell scripts
      Improve formatting of developer notes
      Merge #12702: [wallet] [rpc] [doc] importprivkey: hint about importmulti
      Merge #12709: [wallet] shuffle sendmany recipients ordering
      Increase timeout on avalanche test.
      [DOC] Fix out of order sections in the developer notes
      Add shell script linting: Check for shellcheck warnings in shell scripts
      Remove the unused git-subtree-check.sh script
      Shell script cleanups
      Remove unused variables in shell scripts.
      Delete the contrib/verify-commits subtree
      Migrate gitian-build.sh to python
      [docs] Add instructions for lcov coverage report generation
      Remove script to clean up datadirs
#     Add DynamicMemoryUsage() to LevelDB
      [contrib] Add Valgrind suppressions file
      Merge #12747: Fix typos
#     Merge #11193: [Qt] Terminate string *pszExePath after readlink and without using memset
      use base58 map instead of strchr()
      Disable wallet and address book Qt tests on macOS minimal platform
      Merge #12305: [docs] [refactor] Add help messages for datadir path mangling
#     Merge #12770: Use explicit casting in cuckoocache's compute_hashes(...) to clarify integer conversion
#     Merge #12561: Check for block corruption in ConnectBlock()
#     Merge #11131: rpc: Write authcookie atomically
      Merge #12721: Qt: remove "new" button during receive-mode in addressbook
      Add python3 script shebang lint
      Remove the ZMQ example for Python 2
      Various textual improvements in build docs
      Docs: Add disable-wallet section to OSX build instructions
      Remove Qt4 from the OSX build documentation
      trivial: Improve include comment in src/interfaces/wallet.h
      Cleanup TODOs leftover from PR14119 backporting
      Cleanup reject_code in abc-schnorr.py
      Cleanup reject_code in abc-segwit-recovery
      trivial: Fixed typos and cleaned up language
      release: require macOS 10.10+
      Rename “OS X” to the newer “macOS” convention
      doc: add qrencode to brew install instructions
      [docs] initial QT documentation, move Qt Creator instructions
      Create dependencies.md, and link it from README & build docs
      Merge #12803: Make BaseSignatureCreator a pure interface
      [CMAKE] Add missing -DMAC_OSX definition
      Update timing.json
      Bump version to 0.20.1
      [CMAKE] Add the -commit suffix to version number through obj/build.h
#     Merge #12779: Qt: Remove unused method setupAmountWidget(...)
      Cleanup reject_code in abc-schnorrmultisig-activation
      Cleanup reject_code in abc-minimaldata-activation
      docs: Update osx brew install instruction
      remove brew c++ flag
      Remove the tested versions from the OSX build guide
      Add python3 to list of dependencies on some platforms
      [doc] Minor corrections to osx dependencies
      Merge #12714: Introduce interface for signing providers
      Merge #12762: Make CKeyStore an interface
#     Merge #12752: [MOVEONLY] Move compressor utility functions out of class
      Merge #12811: test: Make summary row bold-red if any test failed and show failed tests at end of table
      Merge #12787: rpc: Adjust ifdef to avoid unreachable code
      Migrated abc-replay-protection off of ComparisonTestFramework
      Migrate abc-mempool-accept-txn.py off of ComparisonTestFramework
      Migrate abc-invalid-chains off of the ComparisonTestFramework
      Read more reject messages from debug logs in feature_cltv.py
      Read more reject messages from debug logs in feature_dersig.py
      Cleanup reject_code in feature_block.py
      Merge #11200: Allow for aborting rescans in the GUI
#     Merge #12837: rpc: fix type mistmatch in `listreceivedbyaddress`
#     Merge #12650: gui: Fix issue: "default port not shown correctly in settings dialog"
      Merge #11353: Small refactor of CCoinsViewCache::BatchWrite()
#     Merge #12621: Avoid querying unnecessary model data when filtering transactions
20.0
      Update manpages for 0.20.0 release
      Update chainparams
      Cleanup reject_code in p2p_invalid_block.py
      Cleanup reject_code in feature_csv_activation.py
      recognize bare multisigs as standard only when using minimal pushes
      Update seeds
      Added some more release notes for 0.20.0
      [5 of 5] Style cleanup.
      [4 of 5] scripted-diff: Rename CBlockDiskPos to FlatFilePos.
      Bump automatic replay protection to May 2020 upgrade
      Merge #11417: Correct typo in comments
#     make CheckMinimalPush available to codebase
      Add upgrade features to release notes
      Merge #13188: qa: Remove unused option --srcdir
      qa: Read reject reasons from debug log, not p2p messages
      Merge #14101: qa: Use named args in validation acceptance tests
      [CMAKE] Fix typos in the secp256k1 CMakeLists.txt file
      Merge #11422: qa: Verify DBWrapper iterators are taking snapshots
      Merge #12436: [rpc] Adds a functional test to validate the transaction version number in the RPC output
      Merge #14024: qa: Add TestNode::assert_debug_log
      Merge #11842: [build] Add missing stuff to clean-local
      Merge #12489: Bugfix: respect user defined configuration file (-conf) in QT settings
      Merge #12996: tests: Remove redundant bytes(…) calls
#     Merge #11395: Qt: Enable searching by transaction id
#     Merge #11015: [Qt] Add delay before filtering transactions
      [teamcity/gitian] Do not remove src tarball from results
#     Merge #10642: Remove obsolete _MSC_VER check
      Merge #12447: test: Add missing signal.h header
      Bump version to 0.20.0
      [CMAKE] Make secp256k1 build standalone
      Fix missing plural form from commit reversal
#     Merge #9910: Docs: correct and elaborate -rpcbind doc
#     Merge #10085: Docs: remove 'noconnect' option
      Merge #10036: Fix init README format to render correctly on github
      Fix incorrect node being checked in segwit recovery test
#     Merge #11617: Avoid lock: Call FlushStateToDisk(...) regardless of fCheckForPruning
#     Merge #12969: Drop dead code CScript::Find
#     Merge #11573: [Util] Update tinyformat.h
      Revert "macOS: Prevent Xcode 9.3 build warnings"
      Revert "Bump version to 0.20.0"
#     [CMAKE] Avoid duplicating the compiler/linker flags
#     [CMAKE] Fix -Wunused-command-line-argument when adding linker flag
      build: avoid getifaddrs when unavailable
#     build: Enable -Wredundant-decls where available
      Bump version to 0.20.0
      Merge #12947: Wallet hd functional test speedup and clarification
#     Merge #12942: rpc: Drop redundant testing of signrawtransaction prevtxs args
#     Merge #13162: [net] Don't incorrectly log that REJECT messages are unknown.
#     Merge #13194: Remove template matching and pseudo opcodes
19.12      
      Updated manpages for 0.19.12 release
      Update chainparams
      Update seeds
      Added some release notes for 0.19.12 release
      Merge #12284: Remove assigned but never used local variables. Enable linter checking for unused local variables.
#     Merge #12569: net: Increase signal-to-noise ratio in debug.log by adjusting log level when logging failed non-manual connect():s
#     better error message for mandatory-flag tx rejections
#     Merge #12537: [arith_uint256] Make it safe to use "self" in operators
      build: add missing leveldb defines
      Fix leveldb compilation for NetBSD
#     Detect if char equals int8_t
      Enforce clang-format version 7.x
#     Merge #12925: wallet: Logprint the start of a rescan
      Merge #12918: test: Assert on correct variable
#     Merge #12797: init: Fix help message for checkblockindex
      Fix some more copies in loops
#     Merge #12460: Assert CPubKey::ValidLength to the pubkey's header-relevant size
#     [CMAKE] Refactor the AddCompilerFlags facilities
      Add -ftrapv to DEBUG_CXXFLAGS when --enable-debug is used
#     [CMAKE] Use plural form when multiple arguments are expected
#     [CMAKE] Avoid warning when checking flags
#     [CMAKE] Enable -Wthread-safety-analysis
      [CMAKE] Refactor warnings for secp256k1
      Add a message to static_assert
      [CI] Enable the undefined behaviour sanitizer
#     Merge #12128: Refactor: One CBaseChainParams should be enough
      Merge #12225: Mempool cleanups
#     Merge #12118: Sort mempool by min(feerate, ancestor_feerate)
#     Merge #12780: Reduce variable scopes
      Fix a copy in sigencoding_tests
      Use a regex to determine the list of sources for check-rpc-mappings
#     Avoid copies in range-for loops and add a warning to detect them
      scripted-diff: Avoid temporary copies when looping over std::map
      macOS: Prevent Xcode 9.3 build warnings
#     [CMAKE] Improve link flag compiler support detection
      [CMAKE] Fix OSX native build
#     Fix missing lock in denialofservice_tests
      rpcauth: Improve by using argparse and getpass modules
      Properly generate salt in rpcauth.py, update tests
      rpcauth: Make it possible to provide a custom password
      Tests: add usage note to check-rpc-mappings.py
      Add linter: Make sure we explicitly open all text files using UTF-8
      tests/tools: Enable additional Python flake8 rules for automatic linting
      [tests] simplify binary and hex response parsing in interface_rest.py
      [tests] only use 2 nodes in interface_rest.py
      [tests] refactor interface_rest.py to avoid code repetition
      [tests] Make json request building more consistent in interface_rest.py
      [tests] improve logging and documentation in interface_rest.py
      [tests] fix flake8 warnings in interface_rest.py test
      [REST] Handle UTXO retrieval when ignoring the mempool
      [tests] Make rpcauth.py testable and add unit tests
#     Remove dead code in BasicTestingSetup
      Use the existing config in CreateAndProcessBlock
      Merge #10825: net: set regtest JSON-RPC port to 18443 to avoid conflict with testnet 18332
      Fix -Wthread-safety-analysis warnings
      MINIMALDATA consensus activation
      New Schnorr multisig activation
      Implement new checkmultisig trigger logic and execution logic.
      When build fails due to lib missing, indicate which one
      build: split warnings out of CXXFLAGS
#     bench: Don't return a bool from main
      test: Add rpcauth pair that generated by rpcauth
#     RPC: Introduce getblockstats to plot things
#     Refactor: RPC: Separate GetBlockChecked() from getblock()
#     Merge #11889: Drop extra script variable in ProduceSignature
#     Merge #11753: clarify abortrescan rpc use
      Merge #12556: [Trivial] fix version typo in getpeerinfo RPC call help
#     Merge #10657: Utils: Improvements to ECDSA key-handling code
#     Merge #10682: Trivial: Move the AreInputsStandard documentation next to its implementation
#     Merge #11900: [script] simplify CheckMinimalPush checks, add safety assert
      Merge #12693: Remove unused variable in SortForBlock
      Add src/rpc/abc.cpp to the files checked by check-rpc-mappings
      [LINTER] Integrate check-rpc-mappings to arcanist
      Fix nits in blockchain.cpp RPC table
      [tests] rename TestNode to TestP2PConn in tests
      Merge #12431: Only call NotifyBlockTip when chainActive changes
      Bump version to 0.19.12
      [CI] Enable debug for the ASAN build
      qa: Avoid checking reject code for now
      Fix undefined behavior in rcu_tests
      Add phpcs to the code formatting tools in the CONTRIBUTING document
# 19.11
      Updated seeds
      Update manpages for 0.19.11 release
      Update chainparams
      Merge #12545: test: Use wait_until to ensure ping goes out
#     Make countBits available to the whole codebase
      [CI] Run the functional tests when debug is enabled
      Merge #12659: Improve Fatal LevelDB Log Messages
#     Add a facility to parse and validate script bitfields.
      net: Add option `-enablebip61` to configure sending of BIP61 notifications
#     Fix bitcoin-cli --version
      Format mm files
#     Make ScriptError a C++11 first class enum
      Restrict CTransaction size assertion to x86
      Replace boost program_options
      Move PHPCS config file where it is used
#     convert C-style (void) parameter lists to C++ style ()
#     Add braces in script.h
      Integrate the lint-boost-dependency.sh linter into arcanist
      build: Guard against accidental introduction of new Boost dependencies
      Fix phpcs warnings for unused function parameters
      Don't replay the gitian initial setup at each build
#     Merge #8330: Structure Packing Optimizations in C{,Mutable}Transaction
#     Give an error and exit if there are unknown parameters
      [teamcity/build] Fix a bug where CONFIGURE_FLAGS would not parse multiple flags correctly
      Add circular dependencies script
#     Use a struct for arguments and nested map for categories
      Pass WalletModel down to SendCoinsEntry by construct
      Allow linters to run once
      Use nullptr instead of 0 in various places in Qt code
      Merge #11886: Clarify getbalance meaning a tiny bit in response to questions.
#     Merge #10777: [tests] Avoid redundant assignments. Remove unused variables
      Remove inexistant parameter keypoolmin from integration test
      Add license blurb to server_tests
      test: Move linters to test/lint, add readme
      Nits in Qt test
      miliseconds => milliseconds
      Migrate build configs from TeamCity to build-configurations.sh
      Merge #11997: [tests] util_tests.cpp: actually check ignored args
      Merge #11879: [tests] remove redundant univalue_tests.cpp
      scripted-diff: Rename CChainState::g_failed_blocks to m_failed_blocks
#     Merge #11714: [tests] Test that mempool rejects coinbase transactions
#     Merge #11133: Document assumptions that are beoing made to avoid division by zero
      [Part 5 of 5] Add a CChainState class to clarify internal interfaces
      QA: Fix race condition in wallet_encryption test
      [tests] [qt] Add tests for address book manipulation via EditAddressDialog
      [wallet] [rpc] Add loadwallet RPC
      [3 of 5] Move CDiskBlockPos from chain to flatfile.
      [script_tests] improve test coverage of minimal push rules
      [test_runner] Fix unidash option for junit output to be one character
      [gitian-build] Fix repo directory to point at bitcoin-abc
      [gitian-build] Bump default memory setting
      [gitian-build] Default the number of jobs to the number of CPUs
      [test_runner] Fix junitoutput typo
      Fix a bug where the TeamCity build only reports one of the test_runner runs
#     Merge #11495: [trivial] Make namespace explicit for is_regular_file
#     Merge #10845: Remove unreachable code
      Merge #10793: Changing &var[0] to var.data()
      [script_tests] improve coverage of minimal number encoding
      add SCRIPT_ENABLE_SCHNORR_MULTISIG flag for new multisig mode
#     [refactor multisig] remove redundant counters
      [2 of 5] validation: Extract basic block file logic into FlatFileSeq class.
#     Error messages in LoadBlockIndexGuts() use __func__ instead of hardcoding function name.
#     [qt] Display more helpful message when adding a send address has failed
#     Add purpose arg to Wallet::getAddress
#     [tests] [qt] Introduce qt/test/util with a generalized ConfirmMessage
      Merge #9544: [trivial] Add end of namespace comments. Improve consistency.
#     Merge #12298: Refactor HaveKeys to early return on false result
#     Add virtual transaction size to the transaction description in Qt
      wallet: Change output type globals to members
      Merge #11330: Trivial: Fix comments for DEFAULT_WHITELIST[FORCE]RELAY
      Merge #11469: fix typo in comment of chain.cpp
      [wallet] Add change type to CCoinControl
      [wallet] use P2WPKH change output if any destination is P2WPKH or P2WSH
      [qt] receive tab: bech32 address opt-in checkbox
      Bech32 addresses in dumpwallet
#     fix backported comment placement
#     [refactor multisig] separate nullfail from stack cleanup
#     [refactor multisig] move nulldummy check to front
#     [refactor multisig] make const values up front
      remove ComparisonTestFramework dependency from segwit recovery test
      CONTRIBUTING.md - update instructions on linting dependencies
#     Use size_t for stack index in OP_MULTISIG
#     Merge #12988: Hold cs_main while calling UpdatedBlockTip() signal
      Version number bumped to 0.19.11
# 19.10      
      Update manpages for 0.19.10 release
      Update chainparams
#     Enforce the use of TxId when constructing COutPoint
      Update rpcwallet.cpp to use TxId
      Factor out outpoint generation in transaction_tests.cpp
      Use TxId in miner_tests.cpp
      Update seeds for 0.19.10 release
      [wallet] Pass error message back from CWallet::Verify()
      Place sanitizers log into their own directory
#     Avoid std::locale/imbue in DateTimeStrFormat
      [wallet] Add CWallet::Verify function
      [wallet] setup wallet background flushing in WalletInit directly
      Use txid in bloom tests
#     Wrap generation of randomized outpoint in tests
      Remove criptolayer.net from seeder lists
      outpt => output
#     use TxId properly in coincontroldialog
      [wallet] Fix potential memory leak in CreateWalletFromFile
#     Rename wallet database classes
#     wallet: Initialize m_last_block_processed to nullptr. Initialize fields where defined.
#     Merge #10728: fix typo in help text for removeprunedfunds
      wallet: Make vpwallets usage thread safe
      Regenerate timing.json
      Merge #14985: test: Remove thread_local from test_bitcoin
      Rename wallet_accounts.py test
#     [qt] Add support to search the address book
      [script_tests] add tests for pubkey encoding with strictenc + nullfail
      [schnorr functional test] improve tx handling
      [schnorr functional test] simplify funding logic
      [schnorr functional test] change block setup logic
      [schnorr functional test] remove ComparisonTestFramework dependency
#     Merge #10684: Remove no longer used mempool.exists(outpoint)
#     Merge #10581: Simplify return values of GetCoin/HaveCoin(InCache)
      Merge #10685: Clarify CCoinsViewMemPool documentation.
      Merge #14571: [tests] Test that nodes respond to getdata with notfound
#     Move CheckBlock() call to critical section
#     Merge #10191: [trivial] Rename unused RPC arguments 'dummy'
      Update doc/release-process.md
      Merge #10627: fixed listunspent rpc convert parameter
#     Fix scheduler test race due to BOOST_CHECK in multithreaded context
#     tests: Fix lock-order-inversion (potential deadlock) in DoS_tests.
#     Merge #13039: Add logging and error handling for file syncing
      Optimize PNG images
#     [1 of 5] util: Move CheckDiskSpace to util.
#     Merge #10559: Change semantics of HaveCoinInCache to match HaveCoin
      [script_tests] add small NUM2BIN case
      Various nits in the wallet code
      Implements a virtual destructor on the BaseRequestHandler class.
      Add the sanitizers suppression files and use them in teamcity build
#     Fix checkqueue_tests use-after-scope
#     Merge #10626: doc: Remove outdated minrelaytxfee comment
      Use functional tests timings from the source directory
      [LINTER] Enforce source code file name conventions
#     doc: Make build system insert version in Doxyfile
      Make various functions in src/test/ static
      Merge #10347: Use range-based for loops (C++11) when looping over vector elements
      Split off key_io_tests from base58_tests
#     Make printchunk() in support/lockedpool.cpp static
#     Merge #10530: Fix invalid instantiation and possibly unsafe accesses of array in class base_uint<BITS>
      [LINTER] Add a linter to replace `unsigned char` with `uint8_t`
      Move make check test logs out of tree
      Partial backport of Core PR11167 to have similar feature in base58_tests.cpp
      Merge #13145: Use common getPath method to create temp directory in tests.
      Use cstdint in generated test headers
#     Merge #10248: Rewrite addrdb with less duplication using CHashVerifier
#     Merge #11737: Document partial validation in ConnectBlock()
#     Merge #11747: Fix: Open files read only if requested
      whitespaces nits in CMakeLists
      Split key_io (address/key encodings) off from base58
      Stop using CBase58Data for ext keys
      Replace CBitcoinSecret with {Encode,Decode}Secret
      tests: Remove compatibility code not needed now when we're on Python 3
      Change all python files to use Python3
      Rename rpcuser.py to rpcauth.py
      Remove test for legacy address when parsing URL in the GUI
      [coverage] Remove subtrees and benchmarks from coverage report
      test: Check RPC argument mapping
      Add share/rpcuser to dist. source code archive
      Various nits in the Qt code
      Remove EncodeDestination's overload relying on global state
      Default to cashaddr in most of the Qt API
      Only generate cashaddr URI from the GUI
      docs: Add a note about the source code filename naming convention
      Use CashAddr everywhere in the Qt interface
      Take CChainParams explicitely in PaymentServer
      Always use cashaddr for dummy addresses in the GUI
      Only handle cashaddr prefix in OpenURIDialog
      [teamcity/build.sh] Read CONFIGURE_FLAGS from environment
      [ibd.sh] Store datadir and debug.log paths in variables
      Fix memory access violation in tests
      Fix memory leak in work_comparator_tests.cpp
#     Merge #10313: [Consensus] Add constant for maximum stack size
      [rpc] fix verbose argument for getblock in bitcoin-cli
      rpcuser.py: Use 'python' not 'python2'
      [build] Warn that only libconsensus can be built without boost
#     [build] Don't fail when passed --disable-lcov and lcov isn't available
#     Use AC_ARG_VAR to set ARFLAGS.
#     Make CWallet IsSpent and IsLockedCoin take a COutPoint as parameter
      [tests] fix flake8 nits in feature_csv_activation.py
      [tests] Change feature_csv_activation.py to use BitcoinTestFramework
      Move utility functions in feature_csv_activation.py out of class.
      [tests] Remove nested loops from feature_csv_activation.py
      [tests] improve logging in feature_csv_activation.py
      fix mistake in test framework's schnorr signing module
      Merge #12926: Run unit tests in parallel
      Merge #12293: [rpc] Mention that HD is enabled if hdmasterkeyid is present
      Activate cashaddr by default
      qa: Fix python TypeError in script.py
      Merge #11772: [tests] Change invalidblockrequest to use BitcoinTestFramework
      RPC: Add child transactions to getrawmempool verbose output
      [LINTER] Fix the python format string when dealing with arrays
      [test] Round target fee to 8 decimals in assert_fee_amount
      Merge #13003: qa: Add test for orphan handling
      Merge #13048: [tests] Fix feature_block flakiness
      Merge #11773: [tests] Change feature_block.py to use BitcoinTestFramework
      Revert "Workaround to Travis-CI Wine/Mingw build hanging occasionally"
      Backport base58 tests from PR11167
#     Merge #12127: Remove unused mempool index
      Merge #12424: Fix rescan test failure due to unset g_address_type, g_change_type
      Merge #12150: Fix ListCoins test failure due to unset g_address_type, g_change_type
      Merge #13163: Make it clear which functions that are intended to be translation unit local (bitcoin-cli.cpp)
      Merge #13163: Make it clear which functions that are intended to be translation unit local (bitcoind.cpp)
      Merge #13163: Make it clear which functions that are intended to be translation unit local (rpc/rawtransaction.cpp)
      Merge #13163: Make it clear which functions that are intended to be translation unit local (init.cpp)
      Prettify README title
      Merge #13163: Make it clear which functions that are intended to be translation unit local (validation.cpp)
      Merge #13163: Make it clear which functions that are intended to be translation unit local (wallet files)
      Fix typos
      qa: Move common args to bitcoin.conf
      Fix 'mempool min fee not met' debug output
      Merge #12861: [tests] Stop feature_block.py from blowing up memory.
      Merge #10479: [trivial] Fix comment for ForceSetArg()
      test: Make g_insecure_rand_ctx thread_local
      Make unit tests use the insecure_rand_ctx exclusively
#     Handle various leftover from PR10321
      Various nits in the Qt code
#     Merge #11870: wallet: Remove unnecessary mempool lock in ReacceptWalletTransactions
      Merge #11516: crypto: Add test cases covering the relevant HMAC-SHA{256,512} key length boundaries
@     Merge #14556: qt: fix confirmed transaction labeled "open"
      Merge #14305: Tests: enforce critical class instance attributes in functional tests, fix segwit test specificity
      Merge #11771: [tests] Change invalidtxrequest to use BitcoinTestFramework
      Merge #9577: Fix docstrings in qa tests
      Restore and backport missing changes from clientversion.h
#     Merge #10310: [doc] Add hint about getmempoolentry to getrawmempool help.
#     Merge #14554: qt: Remove unused `adjustedTime` parameter
#     Merge #13452: rpc: have verifytxoutproof check the number of txns in proof structure
      Merge #11024: tests: Remove OldSetKeyFromPassphrase/OldEncrypt/OldDecrypt
      wallet: Add HasWallets
      qa: Prepare functional tests for Windows
      Fix a-vs-an typos
      wallet: Add AddWallet, RemoveWallet, GetWallet and GetWallets
      wallet: Disallow abandon of conflicted txes
      Fix a shadow warning in askpassphrasedialog.cpp
      Add documentation on how to display python deprecation warnings
      refactor: Drop CWalletRef typedef
#     Prevent mutex lock fail even if --enable-debug
#     qa: Initialize lockstack to prevent null pointer deref
      Add braces in sync.{h|cpp}
#     Clamp walletpassphrase timeout to 2^(30) seconds and check its bounds
      test: Fix dangling wallet pointer in vpwallets
#     Merge #13622: Remove mapRequest tracking that just effects Qt display.
      Merge #11094: Docs: Hash in ZMQ hash is raw bytes, not hex
#     Merge #11083: Fix combinerawtransaction RPC help result section
#     Merge #11011: [Trivial] Add a comment on the use of prevector in script.
      Merge #10912: [tests] Fix incorrect memory_cleanse(…) call in crypto_tests.cpp
      Merge #10655: Properly document target_confirmations in listsinceblock
      Add unit tests for signals generated by ProcessNewBlock()
      Fix concurrency-related bugs in ActivateBestChain
      [LINTER] Enforce flake8 W605 warning
      Merge #11635: trivial: Fix typo – alreardy → already
XXXXX Merge #11480: [ui] Add toggle for unblinding password fields
      Enable flake8 warnings for all currently non-violated rules
      qa: Add missing syncwithvalidationinterfacequeue to tests
      qa: Make TestNodeCLI command optional in send_cli
      qa: Rename cli.args to cli.options
      test_runner: Readable output if create_cache.py fails
      Fix a typo in functional tests documentation
#     Trivial: Fix spelling in zapwallettxes test description
#     Merge #10765: Tests: address placement should be deterministic by default
      Merge #11210: Stop test_bitcoin-qt touching ~/.bitcoin
      Bump version to 0.19.10
      add new encoding checker for Schnorr sigs
#     [sigencoding] refactor schnorr size check
#     Do not unlock cs_main in ABC unless we've actually made progress.
#     Do not permit copying FastRandomContexts
#     Simplify testing RNG code
#     Use a local FastRandomContext in a few more places in net
#     Use a FastRandomContext in LimitOrphanTxSize
#     Introduce a Shuffle for FastRandomContext and use it in wallet and coinselection
#     Bugfix: randbytes should seed when needed (non reachable issue)
#     Make addrman use its local RNG exclusively

# 19.9
##### Merge #10587: Net: Fix resource leak in ReadBinaryFile(...)
##### Merge #10408, #13291, and partial #13163
      Update manpages for 0.19.9 release
      Update chainparams for 0.19.9 release
      Update seeds for 0.19.9 release
##### [RPC] Adding ::minRelayTxFee amount to getmempoolinfo and updating help
      Fix currency/fee-rate unit string in the help text
      getmempool mempoolminfee is a BCH/kB feerate
      Revert "add flags to VerifySignature and sigcache"
      remove SCRIPT_ENABLE_SCHNORR flag and clean up tests
      clean up script_tests -- move segwit recovery into static json
##### Merge #12287: Optimise lock behaviour for GuessVerificationProgress()
      Merge #10789: Punctuation/grammer fixes in rpcwallet.cpp
      Fix incorrect Markdown link
##### Diagnose unsuitable outputs in lockunspent().
      rename schnorr functional test (rename-only)
      shuffle selected coins before transaction finalization
      Fix string concatenation to os.path.join and add exception case
      [rpc] mempoolinfo should take ::minRelayTxFee into account
##### Don't attempt mempool entry for wallet tx on start if already in mempool
      [tests] Combine logs on failure
##### Fix the timestamp format when -logtimemicros is set
      [Trivial] BTC => BCH in functional tests comments
##### Fix exit in generate_header.py and some formatting nits
      bench: Make CoinSelection output groups pass eligibility filter
      Remove unused .travis.yml file
      Fix struct vs class mismatch for Amount
##### Reject headers building on invalid chains by tracking invalidity
      wallet: shuffle coins before grouping, where warranted
      Make FastRandomContext support standard C++11 RNG interface
      wallet: sum ancestors rather than taking max in output groups
##### [CMAKE] Build checkblock benchmark
      sigencoding_tests: improve test coverage
      Remove unused great wall activation code
      remove effect of SCRIPT_ENABLE_SCHNORR flag
      Remove Schnorr activation
      Backport last relevant bit of #11389
      Update importprivkey named args documentation
      Improve signmessages functional test
      Add getmininginfo functional test
##### Merge #9750: Bloomfilter: parameter variables made constant
      Merge #10432: [Trivial] Add BITCOIN_FS_H endif footer in fs.h
      doc: Add release notes for -avoidpartialspends
      test: Add basic testing for wallet groups
      wallet: Remove deprecated OutputEligibleForSpending
      clean-up: Remove no longer used ivars from CInputCoin
      wallet: Switch to using output groups instead of coins in coin selection
      Add -avoidpartialspends and m_avoid_partial_spends
      wallet: Add output grouping
      Fix inconsistencies and grammar in various files
##### fix BIP37 processing for non-topologically ordered blocks
##### unsigned int -> size_t in merkleblock-related code
      Remove unknown version warning from UpdateTip
      Merge #13726: Utils and libraries: Removes the boost/algorithm/string/join dependency
##### Merge #10744: Use method name via __func__ macro
      [Trivial] Docs: Capitalize bullet points
      Trivial: spelling fixes
      Merge #10380: [doc] Removing comments about dirty entries on txmempool
      Replumb ibd.sh to prepare for better post-IBD checks
##### Merge #12250: Make CKey::Load references const
      Fix mining_prioritisetransaction
      fix linting bug in script.py
      Speedup coinselector_tests by using a dummy WalletDBWrapper when apropriate
      Actually disable BnB when there are preset inputs
      Fix Clang Static Analyzer warnings
      wallet: Make fee settings non-static members
      wallet: Add input bytes to CInputCoin
      moveonly: CoinElegibilityFilter into coinselection.h
##### utils: Add insert() convenience templates
      add gdb attach process to doc/functional-tests.md
      qa: Warn when specified test is not found
      Use a unique index for the running jobs in case of duplicated names
      [LINTER] Prevent including a source file
      [consensus] Pin P2SH activation to block 173805 on mainnet
##### Merge #8498: Near-Bugfix: Optimization: Minimize the number of times it is checked that no money...
##### Merge #10196: Bugfix: PrioritiseTransaction updates the mempool tx counter
##### Merge #10228: build: regenerate bitcoin-config.h as necessary
XXXXX Remove unecessary include of boost/version
      Merge #10162: [trivial] Log calls to getblocktemplate
      Merge #10088: Trivial: move several relay options into the relay help group
      Merge #11237: qt: Fixing division by zero in time remaining
      De-witnessing some comments
      Merge #12392: Fix ignoring tx data requests when fPauseSend is set on a peer
      Merge #10151: [logging] initialize flag variable to 0 (and continue if GetLogCategory() fails)
      Merge #11284: Fix invalid memory access in CScript::operator+= (guidovranken, ajtowns)
      [rpc] deprecate ancient softforks' information from getblockchaininfo
      Merge #10376: [tests] fix disconnect_ban intermittency
      Merge #10999: Fix amounts formatting in `decoderawtransaction`
      [refactor] Make TransactionWithinChainLimit more flexible
      clarify GetBlockScriptFlags
      Clarify include guard naming convention
      Improve comment in top-level CMakeLists
      Restore CWallet::minTxFee
      bench: Simplify CoinSelection
      Add a test to make sure that negative effective values are filtered
      Have SelectCoinsMinConf and SelectCoins use BnB or Knapsack and use it
      tests: Avoid copies of CTransaction
      Use mempool's descendent count in the wallet code
      Restore minRelayTxFee
      Implement getRequiredFee in the node interface and use it
      Merge #12652: bitcoin-cli: Provide a better error message when bitcoind is not running
      Merge #10577: Add an explanation of quickly hashing onto a non-power of two range.
      Cleanup ibd.sh
##### Dead code removal
      [LINTER] Set the rules for the phpdoc comments
##### Add benchmark for AES
##### Merge #14085: index: Fix for indexers skipping genesis block.
##### Fix lock reference in miner.h
      Merge #14409: utils and libraries: Make 'blocksdir' always net specific
      create net-specific data directory early in init process
      [LINTER] Enforce using angle brackets in #include directives
      Make CMutableTransaction constructor explicit
##### bitcoin-tx: Remove unused for loop
##### Remove redundant code in MutateTxSign(CMutableTransaction&, const std::string&)
##### Kill MAX_FREE_TRANSACTION_CREATE_SIZE
##### [txindex] transaction Hash -> TxId
      Explicitly call out updating makeseeds.py after major releases
      Directly use CMutableTransaction more often in txvaidationcache_tests.cpp
##### mempool: Fix missing locking in CTxMemPool::check(…) and CTxMemPool::setSanityCheck(…)
      Take a CTransaction as argument for ValidateCheckInputsForAllFlags
      Avoid creating transaction copies in script_tests.cpp and multisig_tests.cpp
      Use MakeTransactionRef in serialize_tests.cpp
##### Update mempool eviction benchmark
      Make m_coinbase_txns a vector of CTransactionRef
      coinbaseTxns => m_coinbase_txns
      Relayout comments in validation.cpp
##### Make float <-> int casts explicit outside of test, qt, CFeeRate
      Bump version to 0.19.9
##### Merge #9949: [bench] Avoid function call arguments which are pointers to uninitialized values
# 19.8
      Add missing release notes for 0.19.8 release
      Update manpages for 0.19.8 release
      Update seeds for 0.19.8 release
      Updated chainparams for 0.19.8 release
      Move original knapsack solver tests to coinselector_tests.cpp
      Move current coin selection algorithm to coinselection.{cpp,h}
      Benchmark BnB in the worst case where it exhausts
      Add tests for the Branch and Bound algorithm
#___  Calculate and store the number of bytes required to spend an input
      Remove coinselection.h -> wallet.h circular dependency
      Implement Branch and Bound coin selection in a new file
      Fix eligibilty_filter => eligibility_filter
      Build wallet dependent benchmark using cmake
      Fix braces in multisig_test
      Fix comments in miner.h
      fix `test_runner.py --help`
      Remove unused DoWarning function
      Merge #15471: rpc/gui: Remove 'Unknown block versions being mined' warning
      Use a struct for output eligibility
      Store effective value, fee, and long term fee in CInputCoin
#___  Move output eligibility to a separate function
#___  Add a GetMinimumFeeRate function which is wrapped by GetMinimumFee
      Fix rounding errors in calculation of minimum change size
#___  Don't create change at the dust limit, even if it means paying more than expected
#___  Eliminate fee overpaying edge case when subtracting fee from recipients
      [LINTER] Move phpcs ruleset to a test/lint/phpcs directory
      [LINTER] Fix some PHP linter rules
#___  Fix make distcheck
      Merge #9350: [Trivial] Adding label for amount inside of tx_valid/tx_invalid.json
      Merge #10090: Update bitcoin.conf with example for pruning
      Merge #10177: Changed "Send" button default status from true to false
#___  Merge #10404: doc: Add logging to FinalizeNode()
      Merge #10460: Broadcast address every day, not 9 hours
      Merge #10536: Remove unreachable or otherwise redundant code
#___  [tests] Better stderr testing
#___  qa: Normalize executable location
#___  Avoid unintentional unsigned integer wraparounds in tests
      [tests] Use FastRandomContext instead of boost::random::*
      Don't assert(...) with side effects
      [LINTER] Add a spell checker to arcanist
      Add script tests with valid 64-byte ECDSA signatures.
      utils: run commands using utf-8 string on Windows
#___  Merge #10560: Remove unused constants
#___  Merge #10524: [tests] Remove printf(...)
      Update timing.json Segwit Recovery functional test name
      [qa] util: Remove unused sync_chain
      Merge #10568: Remove unnecessary forward class declarations in header files
      Merge #9909: tests: Add FindEarliestAtLeast test for edge cases
XXXXX Merge #10522: [wallet] Remove unused variables
      Merge #11198: [Qt] Fix display of package name on 'open config file' tooltip
      Merge #9890: Add a button to open the config file in a text editor
      Cleanup remaining boost includes
#___  Merge #13877: utils: Make fs::path::string() always return utf-8 string on Windows
#___  Merge #12904: [qa] Ensure bitcoind processes are cleaned up when tests end
#___  Merge #10514: Bugfix: missing == 0 after randrange
#___  Merge #10500: Avoid CWalletTx copies in GetAddressBalances and GetAddressGroupings
      Merge #10469: Fixing typo in rpcdump.cpp
#___  Rework the wallet fees interface to make it closer to core's
#___  Merge #9977: QA: getblocktemplate_longpoll.py should always use >0 fee tx
      Merge #10911: [qt] Fix typo and access key in optionsdialog.ui
#___  remove four duplicate tests from script_tests.json
#___  Merge #10679: Document the non-DER-conformance of one test in tx_valid.json.
#___  Merge #11435: build: Make "make clean" remove all files created when running "make check"
#___  Merge #12877: doc: Use bitcoind in Tor documentation
#___  Merge #11620: [build] .gitignore: add background.tiff
#___  Merge #11380: Remove outdated share/certs/ directory
#___  Merge #12843: [tests] Test starting bitcoind with -h and -version
      Merge #10961: Improve readability of DecodeBase58Check(...)
      Merge #10464: Introduce static DoWarning (simplify UpdateTip)
      Avoid spurious boost output in scheduler_tests
      Merge #11710: cli: Reject arguments to -getinfo
      Fix a bunch of spelling errors
#___  Remove unused function for fees.h exposed interface
      Clean up Segwit Recovery feature
#___  Merge #11884: Remove unused include in hash.cpp
      Merge #12434: [doc] dev-notes: Members should be initialized
      Merge #12501: [qt] Improved "custom fee" explanation in tooltip
      Merge #12584: Fix typos and cleanup documentation
      Merge #12999: qt: Show the Window when double clicking the taskbar icon
      Merge #13052: trivial: Fix relevent typo
      Merge #12616: Set modal overlay hide button as default
#___  Merge #12384: [Docs] Add version footnote to tor.md
#___  Merge #10714: Avoid printing incorrect block indexing time due to uninitialized variable
#___  Replace childs => children in radix.h
      [LINTER] Enforce using C++ headers instead of C compatible headers
#___  Replace c compatibility header with native c++ header
#___  Merge #11406: Add state message print to AcceptBlock failure message.
#___  Remove useless priority calculation in wallet
#___  Use constexpr in the RCU code
#___  Merge #11683: tests: Remove unused mininode functions {ser,deser}_int_vector(...). Remove unused imports.
#___  Merge #11655: net: Assert state.m_chain_sync.m_work_header in ConsiderEviction
#___  Remove implicit parameter from GetSerializeSize
#___  Merge #11864: Make CWallet::FundTransaction atomic
#___  [wallet] Tidy up CWallet::FundTransaction
#___  Merge #11337: Fix code constness in CBlockIndex::GetAncestor() overloads
      Remove BIP9 from chain parameters
      Fix Gitian instructions to setup LXC container networking on Debian
#___  Move WalletRescanner to match Bitcoin Core codebase
#___  Remove billable size from CTransaction
#___  Remove billable size from the mempool
#___  Do not update billable size in descendents
#___  Do not update billable size in ancestors
#___  Remove billable size from mining
#___  Limit variable scope
#___  Merge #9533: Allow non-power-of-2 signature cache sizes
#___  Merge #10278: [test] Add Unit Test for GetListenPort
#___  Reintroduce the concept of virtual size from core
      Deprecate parts of validateaddress and introduce getaddressinfo
      Merge #12198: rpc: Add deprecation error for `getinfo`
#___  Merge #12333: Make CWallet::ListCoins atomic
#___  Improve ZMQ functional test
#___  Remove redundant pwallet nullptr check
#___  Add missing locks and locking annotations for CAddrMan
      [rpc] Move DescribeAddressVisitor to rpc/util
      [rpc] split wallet and non-wallet parts of DescribeAddressVisitor
      (finally) remove getinfo in favor of more module-specific infos
#___  [mining] Add a test for TestCBlockTemplateEntry
      Bump version number to 0.19.8
#___  Fix wallet RPC race by waiting for callbacks in sendrawtransaction
#___  Also call other wallet notify callbacks in scheduler thread
      Merge #14374: qt: Add "Blocksdir" to Debug window
#___  Add tests to SingleThreadedSchedulerClient() and document the memory model
#___  Various improvements to the scheduler
#___  scheduler: Add Clang thread safety annotations for variables guarded by m_cs_callbacks_pending
#___  unsigned char => uint8_t
#___  [nit] do not capture unused `this` in wallet interface

GIT SHA e1029deba1b8 [CMAKE] update miniupnpc build (#173) - corresponds to this point

# 19.7
      Update seeds for 0.19.7 release
      Merge #13985: [trivial] Fix slightly confusing mispelling in feature_blocksdir.py log message
      Update manpages for the 0.19.7 release
      Update chainparams for 0.19.7 release
      Reoder various argument declarations
##### Remove SCRIPT_VERIFY_CHECKDATASIG_SIGOPS flag from Schnorr test
      Add post-upgrade testnet checkpoint
##### [Tests] Require exact match in assert_start_raises_init_eror()
##### Give ZMQ consistent order with UpdatedBlockTip on scheduler thread
##### Don't use the functional test arguments in the tmp directory name
      wallet: Display non-HD error on first run
##### Merge #10056: [zmq] Call va_end() on va_start()ed args.
##### Backport current GetDifficulty logic (& tests) from Core
##### remove chain.h dependency from txdb.h
XXXXX Create new wallet databases as directories rather than files
      Remove SCRIPT_VERIFY_CHECKDATASIG_SIGOPS flag from script tests
##### index: Move index DBs into index/ directory.
##### MOVEONLY: Move BaseIndex to its own file.
##### index: Generalize logged statements in BaseIndex.
##### index: Extract logic from TxIndex into reusable base class.
##### db: Make reusable base class for index databases.
XXXXX Allow wallet files not in -walletdir directory
      Support downgrading after recovered keypool witness keys
      SegWit wallet support
##### [CMAKE] Fix Miniupnpc error message
##### Remove IsSolvable
##### Simplify "bool x = y ? true : false". Remove unused function and trailing semicolon.
##### Extend validateaddress information for P2SH-embedded witness
XXXXX Allow wallet files in multiple directories
      Bump wallet version to 190700 and remove the `usehd` option
##### [CMAKE] Add support for Miniupnpc
##### Implicitly know about P2WPKH redeemscripts
      Use GetKeyForDestination in various RPCs
##### Merge #13396: Drop unused arith_uint256 ! operator
##### Add comments indicating "Schnorr" in Schnorr-related script tests.
##### qa: Use node.datadir instead of tmpdir in test framework
##### Don't create another wallet db directory in walletdb_tests
##### Rename SCRIPT_ENABLE_CHECKDATASIG to SCRIPT_VERIFY_CHECKDATASIG_SIGOPS.
      Fixed multiple typos
##### Merge #14513: Avoid 1 << 31 (UB) in calculation of SEQUENCE_LOCKTIME_DISABLE_FLAG
##### Merge #14510: Avoid triggering undefined behaviour in base_uint<BITS>::bits()
##### Update prevector
##### Merge #13894: shutdown: Stop threads before resetting ptrs
##### Expose method to find key for a single-key destination
##### Abstract out IsSolvable from Witnessifier
##### Merge #10308: [wallet] Securely erase potentially sensitive keys/values
##### Merge #10341: rpc/wallet: Workaround older UniValue which returns a std::string temporary for get_str
##### [script] Unit tests for IsMine
##### [script] Unit tests for script/standard functions
##### Fix code style in keystore.cpp/crypter.cpp
      Remove the virtual specifier for functions with the override specifier
      db: Remove obsolete methods from CBlockTreeDB.
##### Set InitMessage for txindex migration
##### [txindex] Activate new transaction index code that runs in background
      Merge #11468: [tests] Make comp test framework more debuggable
##### Comments: More comments on functions/globals in standard.h.
##### [CMAKE] Avoid displaying the console when launching bitcoin-qt.exe
##### [CMAKE] Add cross compiling support for ARM32 and ARM64
##### [CMAKE] Add cross build toolchain files for PC Linux platform
##### [CMAKE] Disable OpenGL in Qt static build for OSX
      [CMAKE] Add QT plugins according to the target platform
##### [refactor] GetAccount{PubKey, Address} -> GetAccountDestination
##### Merge #12425: Add some script tests
##### Merge #12468: Add missing newline in init.cpp log message
##### [qa] don't pad transactions during make_conform_to_ctor
##### Add a test to ensure memory isn't consumed for blocks pre-checkpoint
##### [rpc] Add initialblockdownload to getblockchaininfo
##### Interpret scripts with CHECKDATASIG opcode always valid.
##### tests: move pwalletMain to wallet test fixture
##### Merge #12151: rpc: Remove cs_main lock from blockToJSON and blockheaderToJSON
##### Merge #13527: policy: Remove promiscuousmempoolflags
##### Merge #11742: rpc: Add testmempoolaccept
##### Refactor walletdb_tests to use the wallet test fixture
      Fix for mismatched extern definition in wallet test
##### Fix uninitialized atomic variables
      [CMAKE] Move-only: refactor qt/CMakeLists.txt
##### Remove BIP9 dead code in util.py
##### Add CHECKDATASIG to standard flags.
##### re-fix feature_cltv.py
##### Remove AcceptToMemoryPoolWithTime default args
##### Merge #11872: [rpc] createrawtransaction: Accept sorted outputs
##### Merge #10503: Use REJECT_DUPLICATE for already known and conflicted txn
##### [tests] bind functional test nodes to 127.0.0.1
##### Avoid calling add_nodes multiple times in functional tests
##### qa: Cache only chain and wallet for regtest datadir
##### [qa] Delete cookie file before starting node
##### Remove sleep in feature_config_args
      Consensus: Minimal way to move dust out of consensus
##### Make tests independent of whether CHECKDATASIG is included in mandatory or standard flags
##### Merge #13522: [tests] Fix p2p_sendheaders race
##### Merge #13350: [tests] Add logging to provide anchor points when debugging p2p_sendheaders
##### Merge #13192: [tests] Fixed intermittent failure in p2p_sendheaders.py.
##### Merge #12849: Tests: Add logging in loops in p2p_sendhears.py
##### Merge #11707: [tests] Fix sendheaders
      Merge #14364: doc: Clarify -blocksdir usage
      New -includeconf argument for including external configuration files
##### Fix undefined behavior in avalanche.cpp
##### Simplify semantics of ChainStateFlushed callback
##### scripted-diff: Rename SetBestChain callback ChainStateFlushed
##### [index] Create new TxIndex class.
##### Remove config argument from blockToJSON
##### Merge #10095: refactor: Move GetDifficulty out of `rpc/server.h`
##### Consensus: Minimal way to move dust out of consensus
##### Merge #12564: [arith_uint256] Avoid unnecessary this-copy using prefix operator
##### Merge #12182: Remove useless string initializations
##### Merge #11877: Improve createrawtransaction functional tests
##### Merge #12278: Add special error for genesis coinbase to getrawtransaction
##### [CMAKE] Add support for libqrencode
      [CI] Run functional tests both pre and post graviton
      [CMAKE] Add an option to enable the glibc compatibility features
##### [CMAKE] Add missing files to build bitcoin-qt on OSX
##### [CMAKE] Fix bitcoind cross build for OSX
##### [CMAKE] Cleanup the OSX platform file
      Use angle brackets in windows resource files
##### Separate version info into bitcoin-version.h from bitcoin-config.h
##### [tests] Remove unused variables
      Make release-process.md IBD instruction more precise
##### Clarify comment for SCRIPT_ENABLE_CHECKDATASIG
##### Remove redundant items from STANDARD_SCRIPT_VERIFY_FLAGS
##### Have gArgs handle printing help
##### [docs] Reformat -help output for help2man
##### Fix braces in warnings.cpp
      Bump version number to 0.19.7
      wallet: Refactor g_wallet_init_interface to const reference
      wallet: Make WalletInitInterface members const
      Move RPC registration out of AppInitParameterInteraction
      Fix D2181 by including DumpRPC
      [CMAKE] Allow to use sanitizers with cmake
      [CMAKE] Fix missing protobuf include directory
      Fix more init bugs.
      Fix boost::thread::interruption_point causing build failure on Windows
      Count checkdatasig for transaction entering the mempool
# 19.6
      Update man pages for v0.19.6
      Update chainparams for 0.19.6 release
      Merge #11385: Remove some unused functions and methods
      Remove useless include of boost thread
      Merge #9833: Trivial: fix comments referencing AppInit2
      Merge #10280: [test] Unit test amount.h/amount.cpp
      Merge #9980: Fix mem access violation merkleblock
      Update disclosure policy standards
      Added missing release notes
##### [rpc] Move tojson.h into blockchain.h
      Updated seed list for 0.19.6 release
      update release notes
##### Merge #9804: Fixes subscript 0 (&var[0]) where should use (var.data()) instead.
      [tests] Remove unused and duplicate imports
##### [CMAKE] Fix the prl conversion script library path issue
      Merge #10045: [trivial] Fix typos in comments
      build: Remove -I for everything but project root
      [tests] remove txdb.h dependency from test_bitcoin.h
##### Merge #11351: Refactor: Modernize disallowed copy constructors/assignment
##### Merge #11155: Trivial: Documentation fixes for CVectorWriter ctors
##### Merge #12415: Interrupt loading thread after shutdown request
##### [CMAKE] Fix the prl conversion script linking against lib_NOTFOUND
##### [CMAKE] Determine the Qt library directory from a public variable
      [CMAKE] Add an option to statically link libstdc++
##### [CMAKE] Add an option to reduce exports
##### [backport PR12653] Allow to optional specify the directory for the blocks storage
      Merge #10115: Avoid reading the old hd master key during wallet encryption
      Merge #10258: Fixed typo in documentation for merkleblock.h
      Revert "Merge #10126: Compensate for memory peak at flush time"
      Merge #10126: Compensate for memory peak at flush time
      refactor: Include obj/build.h instead of build.h
##### Merge #11281: Avoid permanent cs_main/cs_wallet lock during RescanFromTime
##### Merge #10489: build: silence gcc7's implicit fallthrough warning
##### Merge #10154: init: Remove redundant logging code
##### Merge #10128: Speed Up CuckooCache tests
##### Merge #12842: Prevent concurrent savemempool
##### Merge #11599: scripted-diff: Small locking rename
##### Merge #10351: removed unused code in INV message
##### Merge #10180: [trivial] Fix typos (tempoarily → temporarily, inadvertantly → inadvertently)
##### Merge #11578: net: Add missing lock in ProcessHeadersMessage(...)
      Merge #10309: Trivial: remove extra character from comment
##### Merge #9794: Minor update to qrencode package builder
##### Support serialization as another type without casting
##### Support deserializing into temporaries
##### Merge READWRITEMANY into READWRITE
##### [mining] Rename several CBlockTemplateEntry members for clarity
##### Merge #9333: Document CWalletTx::mapValue entries and remove erase of nonexistent "version" entry.
## ?      Merge #9724: Qt/Intro: Add explanation of IBD process
##### Merge #9916: Fix msvc compiler error C4146 (minus operator applied to unsigned type)
##### Merge #13080: mempool: Add compile time checking for ::mempool.cs runtime locking assertions
##### Make functions in rpc/blockchain.cpp static.
##### Merge #9906: Disallow copy constructor CReserveKeys
##### Merge #9555: [test] Avoid reading a potentially uninitialized variable in tx_invalid-test (transaction_tests.cpp)
      Merge #14511: doc: Remove explicit storage requirement from README.md
##### Merge #9962: [trivial] Fix typo in rpc/protocol.h
      Merge #9960: Trivial: Add const modifier to GetHDChain and IsHDEnabled
##### Merge #10033: Trivial: Fix typo in key.h comment
##### Merge #9690: Change 'Clear' button string to 'Reset'
      Merge #9952: Add historical release notes for 0.14.0
##### Merge #12172: Bugfix: RPC: savemempool: Don't save until LoadMempool() is finished
##### Merge #11099: [RPC][mempool]: Add savemempool RPC
##### Merge #10265: [wallet] [moveonly] Check non-null pindex before potentially referencing
##### Merge #12681: Fix ComputeTimeSmart test failure with -DDEBUG_LOCKORDER



##### Merge #11744: net: Add missing locks in net.{cpp,h}
##### Revert removal of code block
##### Merge #12206: qa: Sync with validationinterface queue in sync_mempools
      test: refactor: Use absolute include paths for test data files
      qt: refactor: Changes to make include paths absolute
      qt: refactor: Use absolute include paths in .ui files
##### Merge #12283: Fix typos
##### Sanitize some wallet serialization
~~    No need to use OpenSSL malloc/free~~
##### Merge #9539: [net] Avoid initialization to a value that is never read
      [trivial] Fix recently introduced typos in comments
##### Merge #12326: net: initialize socket to avoid closing random fd's
##### Merge #11252: [P2P] When clearing addrman clear mapInfo and mapAddr.
##### Merge #12448: Interrupt block generation on shutdown request
##### Merge #11585: addrman: Add missing lock in Clear() (CAddrMan)
##### Merge #10914: Add missing lock in CScheduler::AreThreadsServicingQueue()
##### Merge #11831: Always return true if AppInitMain got to the end
##### Merge #10057: [init] Deduplicated sigaction() boilerplate
##### Init: Remove redundant exit(EXIT_FAILURE) instances and replace with return false
##### Ignore macOS daemon() depracation warning
      [LINTER] Revive the locale dependent functions linter in arcanist
##### Merge #9693: Prevent integer overflow in ReadVarInt.
##### Backport dev notes on RPC
##### blockfilter: Use unordered_set instead of set in blockfilter.
##### Disallow using addresses in createmultisig
##### Merge #10027: Set to nullptr after delete
##### Merge #10029: Fix parameter naming inconsistencies between .h and .cpp files
##### Merge #12349: shutdown: fix crash on shutdown with reindex-chainstate
##### Merge #12367: Fix two fast-shutdown bugs
##### Merge #11238: Add assertions before potential null deferences
##### Add developer notes about blocking GUI code and src/interfaces/README.md
      Incremented version number to 0.19.6.
##### test: Plug memory leaks and stack-use-after-scope
##### Fix a memory leak in DoS_tests
      [CMAKE] Make RelWithDebInfo the default CMake configuration
#####  [db] Migration for txindex data to new, separate database.
#####  [db] Create separate database for txindex.
~~      Add additional unit tests for segwit recovery~~
##### fix out-of-bounds memory write in key_tests
##### blockfilter: Remove sharp edge (uninitialized m_filter_type) when using the compiler-generated constructor for BlockFilter
##### blockfilter: Refactor and add tests for BlockFilter construction
##### blockfilter: add block filters
#####  Remove obsolete comment from MANDATORY_SCRIPT_VERIFICATION_FLAGS
~~    Use angle bracket in include for qt tests~~
~~    Use angle bracket in include for remaining qt files~~
~~    Use angle bracket in include for qt pages, widgets and views~~
~~    Use angle bracket in include for qt model files~~
~~    Use angle bracket in include for qt dialog files~~
~~    Use angle bracket in include for qt/bitcoin* files~~
      Fix typos.
      
# 19.5

~~    Added some notes for the 0.19.5 release~~
~~    Updated mainnet seeds for 0.19.5 release~~
##### Merge #10569: Fix stopatheight
~~    Updated manpages for 0.19.5 release~~
~~    Update chainparams for 0.19.5 release~~
##### Merge #11880: Stop special-casing phashBlock handling in validation for TBV
##### Use WalletBalances struct in Qt
##### Remove direct bitcoin calls from qt/sendcoinsdialog.cpp
##### Do not allow users to get keys from keypool without reserving them
~~    nits in lcg_tests~~
##### Merge #9517: [refactor] Switched httpserver.cpp to use RAII wrapped libevents.
~~    [LINTER] Remove the lint-format-strings.sh script~~
~~    [LINTER] Move the string formatting function list to the python script~~
##### Remove direct bitcoin access from qt/guiutil.cpp
##### Remove direct bitcoin calls from qt transaction table files
~~    Remove direct bitcoin calls from qt/paymentserver.cpp~~
      [CI] Make gitian builds run on multiple cpus
#####  Merge #11012: Make sure to clean up mapBlockSource if we've already seen the block
##### Remove direct bitcoin calls from qt/addresstablemodel.cpp
      [LINTER] Improve lint-format-strings.sh performance
##### Remove direct bitcoin calls from qt/coincontroldialog.cpp
##### Remove most direct bitcoin calls from qt/walletmodel.cpp
##### Remove direct bitcoin calls from qt/optionsdialog.cpp
~~    Add function 'IsGravitonEnabled'~~
#####  e8c680bd5840d7b055cc618b98242ed6f980d393 serialize: Serialization support for big-endian 32-bit ints.
      [DOC] Add headers inclusion guidelines to the developer notes
      Use angle bracket in include for wallet test
~~ [qt] Simplifies boolean expression model && model->haveWatchOnly()~~
##### [qt] Avoid potential null pointer dereference in TransactionView::exportClicked()
      Use angle bracket in include for seeder
      Use angle bracket in include for wallet
##### [nit] Remove redundant parameter from `CTxMemPool::PrioritiseTransaction`
##### Remove direct bitcoin calls from qt/rpcconsole.cpp
##### Remove direct bitcoin calls from qt/bantablemodel.cpp
##### Remove direct bitcoin calls from qt/peertablemodel.cpp
##### Remove direct bitcoin calls from qt/intro.cpp
##### Remove direct bitcoin calls from qt/clientmodel.cpp
##### Remove direct bitcoin calls from qt/splashscreen.cpp
      [qa] [nit] remove extranous variable in mining_prioritisetransaction.py

# 1.0.0 Release

#####  Extract CSipHasher to it's own file in crypto/ directory.
      Use angle bracket in include for src (part 6)
      Use angle bracket in include for src (part 5)
      Use angle bracket in include for src (part 4)
      Use angle bracket in include for src (part 3)
      Use angle bracket in include for src (part 2)
      Use angle bracket in include for src (part 1)
      Use angle bracket in include for test (part 5)
      Bump version number to 0.19.5
# 19.4
      Added missing notes to release notes
      Use angle bracket in include for zmq
      Use angle bracket in include for test (part 4)
##### Nits in abc-high_priority_transaction.py
##### [schnorr] Refactor the signature process in reusable component
      Use angle bracket in rpcwallet and rpcdump
      Use angle bracket in include for test (part 3)
      build: Move interfaces/* to libbitcoin_server
      Use angle bracket in include for tests (part 2)
      Use angle bracket in include for tests (part 1)
      Use angle bracket in include for support
      Use angle bracket in include for script
      Use angle bracket in include for rpc
##### Remove direct bitcoin calls from qt/utilitydialog.cpp
      Use angle bracket in include for primitives
      Use angle bracket in include for policy
      Use angle bracket in include for crypto
      Use angle bracket in include for consensus
      Use angle bracket in include for compat
      Use angle bracket in include for bench
      Move interface -> interfaces
##### Add Sent and Received information to the debug menu peer list
      Update seeds
      Updated manpages for 0.19.4 release
      Updated chainparams for 0.19.4 release
      Remove the boost/algorithm/string/case_conv.hpp dependency
      scripted-diff: Replace NET_TOR with NET_ONION
      Log warning message when deprecated network name 'tor' is used (e.g. option onlynet=tor)
##### Remove Safe mode
      Use angle bracket in include for net and netbase
##### Remove direct bitcoin calls from qt/bitcoingui.cpp
##### Remove direct bitcoin calls from qt/optionsmodel.cpp
##### Remove direct bitcoin calls from qt/bitcoin.cpp
##### Merge #12630: Provide useful error message if datadir is not writable.
      Merge #12422: util: Make LockDirectory thread-safe, consistent
      Merge #11904: Add a lock to the wallet directory
#####  blockfilter: add GCSFilter class
#####  streams: Implement BitStreamReader/Writer classes.
#####  streams: Create VectorReader stream interface for vectors.
#####  [wallet] Close DB on error.
#####  Replace C compatibility headers by their C++ equivalent
      [CMAKE] Move .h files transformed from .ui to the form subdirectory
      GUI: Receive: Remove option to reuse a previous address
      Merge #11041: Add LookupBlockIndex
      Update release notes for 0.19.3 to include instructions for re-enabling deprecated signrawtransaction
#####  [secp256k1] add schnorr sign jni binding
      Revert "[LINTER] Integrate lint-locale-dependence.sh into arcanist"
      [LINTER] Enforce custom coding standard for PHP files
#####  Merge #11838: qa: Add getrawtransaction in_active_chain=False test
      Add additional test in segwit recovery activation
#####  Add functional tests for rejecting headers that build on invalid chains
#####  Merge #10368: [wallet] Remove helper conversion operator from wallet
      Remove much of the remaining BIP9 code
#####  rpc: Handle `getinfo` locally in bitcoin-cli w/ `-getinfo`
#####  Fix increment in rpc/mining.cpp when coinbase tx is skipped
#####  Migrate MakeUnique to c++14 std::make_unique
#####  Use c++14 generic std::rbegin() and std::rend() instead of class methods
#####  [secp256k1] refactor nativeECDSABuffer to a more generic name
      [Part 4 of 5] Add a CChainState class to clarify internal interfaces
#####  Remove unused depends list from `getblocktemplate` transactions
#####  Remove unused parameter `validFeeEstimates` from `CTxMempool::addUnchecked`
#####  Fix comment in CheckInputs to match changed code
      Merge #10493: Use range-based for loops (C++11) when looping over map elements
      doc: update FreeBSD build guide for 12.0
      FreeBSD: Document Python 3 requirement for 'gmake check'
      doc: split FreeBSD build instructions out of build-unix.md
      Drop support for OpenBSD
      Make c++14 standard the default for compilation
#####  Fix signrawtransaction failing when a wallet URI is specified
      [CI] Add an option to build and run unit tests with debug enabled
      Remove communication style article from CONTRIBUTING.md
      Add dev articles to CONTRIBUTING.md
      Merge #10858: [RPC] Add "warnings" field to getblockchaininfo and unify "warnings" field in get*info RPCs
      Incremented version number, moved and renamed old release-notes.d, and added new release-notes.md
# 19.3

193d2b9c296a05f64db546093df127f463b36490 Fix Windows build errors introduced in D2765
4c92f4c7b84fcd9fcf1dece3147eb72bfa472b57 Format the php files
c670a0701e3acc84b4f5f4dd68814745177fec5b qa: Improve getchaintxstats functional test
a367935cda4538e61ede31b5dbd5b4677ee3f194 Update manpages for 0.19.3 release
1bcdf75ba2a73e29ce0b3ddbb49085168f5a8a2e Update chainparams for 0.19.3 release
219e15d6d7d812414ab01e78d52b2d0ba8285a22 Update seeds.
129395a132a37d1df993ca53e64efa363e879ea2 Document method for reviewers to verify chainTxData
2c3a95f445a27abc99c3467b649f69a4e00647eb Add tip to the create Debian VM instructions
92d66e964a479a5ef1c5a808ae79062dea436d8a [Target v0.19] Deprecate and add test for signrawtransaction
cffcc6a785585ccebb08fd0860a624ea06d8a714 Update wallet_listsinceblock.py to use signrawtransactionwithwallet rpc
e5900c671dadebafabec19f878b1749df8bea5ba Add missing step in Debian VM creation process
f8adcd8e16f4393d14f5eb3c3abd6b45bb6e64fd Update Debian VM version link to latest
8dfaa2198450cf83c578ad98ed724414f72b9279 Add schnorr verify benchmark
5ea73621a21f2300fded58e5dd6953724344fcb8 Add schnorr sign benchmark
256ef1e899be702aabe6f44cf431d1f5a4e45273 Use static_cast instead of C-style casts for non-fundamental types
a12048663e1fd0c1646c440100983e9ac05d87b3 Merge #10275: [rpc] Allow fetching tx directly from specified block in getrawtransaction
5ff091f6080f93a86a5e2a7fba1fa5b176ceaf95 Backport remaining changes from Core PR 10742
c0c5f98dcdfb419872f358c788d74b99cd705dc8 Bump version numbers to 0.19.3
0fbd8d77fa7f0588267e11c8541a25c073872ed2 Lint everything
be1b951a84b00ae657986a22686cab25f565501b Fix arc liberate complaining about concatenation in constant definition
9d53b39b2564d6949e5b3452fc29ff8ce6af5f37 [LINTER] Escape the % char in the description of PythonFormatLinter
c80bf903f9794a536ad699156570866f8dbfb014 Fix comment about s in schnorr sigs
20f59d582b2b42854c0631be988824738bf6c4cc [Part 3 of 5] Add a CChainState class to clarify internal interfaces
c018bd540341eca027e939e6e3b67b464bada074 [LINTER] Enforce a minimum version for autopep8
463c4d9d59ea9238cd5aa4ab8401f8794f0b40cc [CI] Improve performance when running the tests
2e36d9883c059f7ac1873fd2c32d875a7fe35f78 Improve teamcity agent setup documentation and service script
66dda0fc732bbe8f0850dc3075ae127362ebfc8f Adjusted indentation in getblockchaininfo's help text to properly align.
a639e292d4b4f436b9fe7391d9d085e0d46d263f remove unused fnoncriticalerrors variable from cwalletdb::findwallettx
2e5a9ba1e9cff7b61d443ccb345fce32502062df Merge #10858: [RPC] Add "warnings" field to getblockchaininfo and unify "warnings" field in get*info RPCs
6a1c4b55f41174d593e601a98d956b0778182b27 [secp256k1] add schnorr verify jni binding
3e946cea37712ab671f4a6cbf8fa85cfa1299445 Allow setting an env variable to skip tests in build.sh
4e4fb6e1bb28de4c04a981b976dac54010e09661 [Part 2 of 5] Add a CChainState class to clarify internal interfaces
f00d08b715e911a68dcc14cb6eb6f77bdaf48acf [LINTER] Integrate lint-locale-dependence.sh into arcanist
272aedf4f88187ad72dc58fe7dda44a404a5c697 Fix a potential segfault in the seeder
b9c102a733b94da1674efbff4791ccefd6151f24 Remove nonnull warning when calling secp256k1_schnorr_sign with NULL noncefp
095183ecff21e74d063c3b930130942566f9184e Fix unlocked_until in getwalletinfo rpc
816dbd9150e9ec9a0b45c34af75cf0283e34b973 Update seeds
2b2cb1607a7ab9171adcfa8b11075d917f206e55 [secp256k1] remove unused byte array
fe7f30d365c104cc2f9d2609979774460a79465d Fix seeder in the gitian build
70575e5662a5c33d3e387754eb35b783f012bf53 Avoid slow transaction search with txindex enabled
31f5ed8c61eaa4c6e65a58b0dfd85e9ebe024394 [secp256k1] remove guava dep
624f189f44bd8964719f89959c14ad5859fef632 Added better error reporting for running individual functional tests
723f52765f373e81a717b10ea28e268baa6c130f Update manpages
7fd6095e4604c126ca23b69a23813fc4698d68f7 Update chainparams for 0.19.2 release
41a74c2c9a6abe937c726310ae218dab253d2865 [LINTER] Display the line number in the lint-locale-dependence.sh linter
568a8f9c4b09b9143263293b28a350f60362d125 Merge #11028: Avoid masking of difficulty adjustment errors by checkpoints
9ae3baa3bba187681bdbbfd055895be371f02dff Add some release notes
379dd8366d1087d94341fc89e5b949b1a7c079d1 Merge #8665: Assert all the things!
737ff23ed292c5b8b51834d311487d810bea4863 [secp256k1] fix java secp256k1 test
298b6cbf751c67500c2ce02d47bc681203b1b237 Remove unused seeder/compat.h
cbc12967970fdcb32f2a3d16d0dcf557246ea3d3 Added build-secp256k1.sh for running secp tests in TeamCity
232eb7ae769dbbf684fb85e5eca40dfdfd849cf3 Allow for running secp256k1 java build/tests out of tree
c3913fe8a6af3660ec0f23e5c20c8a78ca7691b7 Fix braces in qt/clientmodel.cpp
1000e62d1f8ab0cd6555bdef0d1439468218d35e Backport VerifyResult enum class
b51031caa043b46ed4e47b29207762e0ec4ec2ca Backport FlushStateMode enum class
bb492d7f8fba4288ba40fbf7a91e98b812e3ba8f Backport ThresholdState enum class
a07b6bc7a128ba8e2f1ed776b45656e09bc8f202 Backport HelpMessageMode enum class
ae4d2b98623eeecbcce1760ab6b111f94f489007 Backport BlockSource enum class
9b29a548adb3577a51b3223f203462cc3e0f60be Backport RetFormat enum class
e3b6fe0ed1ba7e813d69f38122882c7ddf563b84 Merge #11027: [RPC] Only return hex field once in getrawtransaction
1d6970e887415230803fbd3547599fddab1e8601 Merge #11565: Make listsinceblock refuse unknown block hash
c22ea26bba07c90fdd3826daf9c9bfae02ecf95f refactor: remove usage of locale dependent std::isspace
6a7b8df807b2b0bff266cda307306538c540e6d0 [LINTER] build: Add linter for checking accidental locale dependence
a423ecd14cd92b0293c48ffaa37a5acf4ee636c2 [CMAKE] Harden the executables
810c42e0a173263f2616c66549df8e668d79ac58 Fix braces in utilstrencodings
1c54ad92152d183e406cd92a5c0a75b09a5203e0 Fix braces in netbase.cpp
a4e4876986e28ca3e96e935117b4a341e74bc2a2 [CMAKE] Add DEBUG and DEBUG_LOCKORDER definitions to the Debug config
b98b41c54ab4bc08af741043834edba6e465305d test: Replace remaining sprintf with snprintf
b189803bde626d9d1bdf9f920248b2c6e2133d2e [LINTER] Fix check doc incompatibility with BSD grep
ddade70b79a0e18dc5708e8ecd99a712a8c421a3 [LINTER] Improve check-doc regex
c0dc1ba156bedb50245687199eafaac6ed9cc249 Pass SendCoinsRecipient (208 bytes) by const reference
bf97bcfb8254b834f6c0e21cbff9fcbaac990d96 Remove redundant locks
6a8115b98cca7b2a64417b634dc9a59a842d1935 Merge #12327: [gui] Defer coin control instancing
812ae90b6ceed2ac6c9f55f69f61d24f024e9521 Merge #11039: Avoid second mapWallet lookup
d1c1b1620a48df63b998cdcae1e6a5166bdbe914 Merge #9622: [rpc] listsinceblock should include lost transactions when parameter is a reorg'd block
68777835645ab730841337beb1780d8f13fca476 Merge #10775: nCheckDepth chain height fix
97a60cd531dae2e546ca0f5eedd7da15c7ab295f Bump version to 0.19.2
