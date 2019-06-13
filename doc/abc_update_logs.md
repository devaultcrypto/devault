# 19.7
      Update seeds for 0.19.7 release
      Merge #13985: [trivial] Fix slightly confusing mispelling in feature_blocksdir.py log message
      Update manpages for the 0.19.7 release
      Update chainparams for 0.19.7 release
      Reoder various argument declarations
      Remove SCRIPT_VERIFY_CHECKDATASIG_SIGOPS flag from Schnorr test
      Add post-upgrade testnet checkpoint
      [Tests] Require exact match in assert_start_raises_init_eror()
      Give ZMQ consistent order with UpdatedBlockTip on scheduler thread
      Don't use the functional test arguments in the tmp directory name
      wallet: Display non-HD error on first run
      Merge #10056: [zmq] Call va_end() on va_start()ed args.
      Backport current GetDifficulty logic (& tests) from Core
      remove chain.h dependency from txdb.h
      Create new wallet databases as directories rather than files
      Remove SCRIPT_VERIFY_CHECKDATASIG_SIGOPS flag from script tests
      index: Move index DBs into index/ directory.
      MOVEONLY: Move BaseIndex to its own file.
      index: Generalize logged statements in BaseIndex.
      index: Extract logic from TxIndex into reusable base class.
      db: Make reusable base class for index databases.
      Allow wallet files not in -walletdir directory
      Support downgrading after recovered keypool witness keys
      SegWit wallet support
      [CMAKE] Fix Miniupnpc error message
      Remove IsSolvable
      Simplify "bool x = y ? true : false". Remove unused function and trailing semicolon.
      Extend validateaddress information for P2SH-embedded witness
      Allow wallet files in multiple directories
      Bump wallet version to 190700 and remove the `usehd` option
      [CMAKE] Add support for Miniupnpc
      Implicitly know about P2WPKH redeemscripts
      Use GetKeyForDestination in various RPCs
      Merge #13396: Drop unused arith_uint256 ! operator
      Add comments indicating "Schnorr" in Schnorr-related script tests.
      qa: Use node.datadir instead of tmpdir in test framework
      Don't create another wallet db directory in walletdb_tests
      Rename SCRIPT_ENABLE_CHECKDATASIG to SCRIPT_VERIFY_CHECKDATASIG_SIGOPS.
      Fixed multiple typos
      Merge #14513: Avoid 1 << 31 (UB) in calculation of SEQUENCE_LOCKTIME_DISABLE_FLAG
      Merge #14510: Avoid triggering undefined behaviour in base_uint<BITS>::bits()
      Update prevector
      Merge #13894: shutdown: Stop threads before resetting ptrs
      Expose method to find key for a single-key destination
      Abstract out IsSolvable from Witnessifier
      Merge #10308: [wallet] Securely erase potentially sensitive keys/values
      Merge #10341: rpc/wallet: Workaround older UniValue which returns a std::string temporary for get_str
      [script] Unit tests for IsMine
      [script] Unit tests for script/standard functions
      Fix code style in keystore.cpp/crypter.cpp
      Remove the virtual specifier for functions with the override specifier
      db: Remove obsolete methods from CBlockTreeDB.
      Set InitMessage for txindex migration
      [txindex] Activate new transaction index code that runs in background
      Merge #11468: [tests] Make comp test framework more debuggable
      Comments: More comments on functions/globals in standard.h.
      [CMAKE] Avoid displaying the console when launching bitcoin-qt.exe
      [CMAKE] Add cross compiling support for ARM32 and ARM64
      [CMAKE] Add cross build toolchain files for PC Linux platform
      [CMAKE] Disable OpenGL in Qt static build for OSX
      [CMAKE] Add QT plugins according to the target platform
      [refactor] GetAccount{PubKey, Address} -> GetAccountDestination
      Merge #12425: Add some script tests
      Merge #12468: Add missing newline in init.cpp log message
      [qa] don't pad transactions during make_conform_to_ctor
      Add a test to ensure memory isn't consumed for blocks pre-checkpoint
      [rpc] Add initialblockdownload to getblockchaininfo
      Interpret scripts with CHECKDATASIG opcode always valid.
      tests: move pwalletMain to wallet test fixture
      Merge #12151: rpc: Remove cs_main lock from blockToJSON and blockheaderToJSON
      Merge #13527: policy: Remove promiscuousmempoolflags
      Merge #11742: rpc: Add testmempoolaccept
      Refactor walletdb_tests to use the wallet test fixture
      Fix for mismatched extern definition in wallet test
      Fix uninitialized atomic variables
      [CMAKE] Move-only: refactor qt/CMakeLists.txt
      Remove BIP9 dead code in util.py
      Add CHECKDATASIG to standard flags.
      re-fix feature_cltv.py
      Remove AcceptToMemoryPoolWithTime default args
      Merge #11872: [rpc] createrawtransaction: Accept sorted outputs
      Merge #10503: Use REJECT_DUPLICATE for already known and conflicted txn
      [tests] bind functional test nodes to 127.0.0.1
      Avoid calling add_nodes multiple times in functional tests
      qa: Cache only chain and wallet for regtest datadir
      [qa] Delete cookie file before starting node
      Remove sleep in feature_config_args
      Consensus: Minimal way to move dust out of consensus
      Make tests independent of whether CHECKDATASIG is included in mandatory or standard flags
      Merge #13522: [tests] Fix p2p_sendheaders race
      Merge #13350: [tests] Add logging to provide anchor points when debugging p2p_sendheaders
      Merge #13192: [tests] Fixed intermittent failure in p2p_sendheaders.py.
      Merge #12849: Tests: Add logging in loops in p2p_sendhears.py
      Merge #11707: [tests] Fix sendheaders
      Merge #14364: doc: Clarify -blocksdir usage
      New -includeconf argument for including external configuration files
      Fix undefined behavior in avalanche.cpp
      Simplify semantics of ChainStateFlushed callback
      scripted-diff: Rename SetBestChain callback ChainStateFlushed
      [index] Create new TxIndex class.
      Remove config argument from blockToJSON
      Merge #10095: refactor: Move GetDifficulty out of `rpc/server.h`
      Consensus: Minimal way to move dust out of consensus
      Merge #12564: [arith_uint256] Avoid unnecessary this-copy using prefix operator
      Merge #12182: Remove useless string initializations
      Merge #11877: Improve createrawtransaction functional tests
      Merge #12278: Add special error for genesis coinbase to getrawtransaction
      [CMAKE] Add support for libqrencode
      [CI] Run functional tests both pre and post graviton
      [CMAKE] Add an option to enable the glibc compatibility features
      [CMAKE] Add missing files to build bitcoin-qt on OSX
      [CMAKE] Fix bitcoind cross build for OSX
      [CMAKE] Cleanup the OSX platform file
      Use angle brackets in windows resource files
      Separate version info into bitcoin-version.h from bitcoin-config.h
      [tests] Remove unused variables
      Make release-process.md IBD instruction more precise
      Clarify comment for SCRIPT_ENABLE_CHECKDATASIG
      Remove redundant items from STANDARD_SCRIPT_VERIFY_FLAGS
      Have gArgs handle printing help
      [docs] Reformat -help output for help2man
      Fix braces in warnings.cpp
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
      [rpc] Move tojson.h into blockchain.h
      Updated seed list for 0.19.6 release
      update release notes
      Merge #9804: Fixes subscript 0 (&var[0]) where should use (var.data()) instead.
      [tests] Remove unused and duplicate imports
      [CMAKE] Fix the prl conversion script library path issue
      Merge #10045: [trivial] Fix typos in comments
      build: Remove -I for everything but project root
      [tests] remove txdb.h dependency from test_bitcoin.h
      Merge #11351: Refactor: Modernize disallowed copy constructors/assignment
      Merge #11155: Trivial: Documentation fixes for CVectorWriter ctors
      Merge #12415: Interrupt loading thread after shutdown request
      [CMAKE] Fix the prl conversion script linking against lib_NOTFOUND
      [CMAKE] Determine the Qt library directory from a public variable
      [CMAKE] Add an option to statically link libstdc++
      [CMAKE] Add an option to reduce exports
      [backport PR12653] Allow to optional specify the directory for the blocks storage
      Merge #10115: Avoid reading the old hd master key during wallet encryption
      Merge #10258: Fixed typo in documentation for merkleblock.h
      Revert "Merge #10126: Compensate for memory peak at flush time"
      Merge #10126: Compensate for memory peak at flush time
      refactor: Include obj/build.h instead of build.h
      Merge #11281: Avoid permanent cs_main/cs_wallet lock during RescanFromTime
      Merge #10489: build: silence gcc7's implicit fallthrough warning
      Merge #10154: init: Remove redundant logging code
      Merge #10128: Speed Up CuckooCache tests
      Merge #12842: Prevent concurrent savemempool
      Merge #11599: scripted-diff: Small locking rename
      Merge #10351: removed unused code in INV message
      Merge #10180: [trivial] Fix typos (tempoarily → temporarily, inadvertantly → inadvertently)
      Merge #11578: net: Add missing lock in ProcessHeadersMessage(...)
      Merge #10309: Trivial: remove extra character from comment
      Merge #9794: Minor update to qrencode package builder
      Support serialization as another type without casting
      Support deserializing into temporaries
      Merge READWRITEMANY into READWRITE
      [mining] Rename several CBlockTemplateEntry members for clarity
      Merge #9333: Document CWalletTx::mapValue entries and remove erase of nonexistent "version" entry.
      Merge #9724: Qt/Intro: Add explanation of IBD process
      Merge #9916: Fix msvc compiler error C4146 (minus operator applied to unsigned type)
      Merge #13080: mempool: Add compile time checking for ::mempool.cs runtime locking assertions
      Make functions in rpc/blockchain.cpp static.
#####  Merge #9906: Disallow copy constructor CReserveKeys
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
      Disallow using addresses in createmultisig
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
