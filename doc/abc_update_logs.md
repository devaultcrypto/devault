      [Backport]gui: hide HD & encryption icons when no wallet loaded
      [CI] Update the clang-tidy build
      [CMAKE] Make clang-tidy fail the build rather than trying to auto-fix
      [CMAKE] Don't enable clang tidy on native builds
      Fix missing braces in validationinterface.cpp
      [CI] Run the circular dependencies linter on CI for each diff
      [CI] Use script paths relative to the project root in the configuration
      More thread safety annotation coverage
      [fix] only use horizontalAdvance for Qt versions which support it
      depends: Add --sysroot option to mac os native compile flags
      build: remove chrono package from depends Boost
      [Backport]bug-fix macos: give free bytes to F_PREALLOCATE
      [lint] update expected circular dependencies
      [land-bot] Introduce an autogen amendment step and update version numbers using it
      [backport#12173] [Qt] Use flexible font size for QRCode image address
      [backport#16194] refactoring: remove mapBlockIndex global
      [backport#16194] refactoring: make pindexBestInvalid internal to validation.cpp
      [backport#16194] refactoring: add block_index_candidates arg to LoadBlockIndex
      IsUsedDestination should count any known single-key address
      [backport#16194] refactoring: move block metadata structures into BlockManager
      wallet: Tidy CWallet::SetUsedDestinationState
      Prevent UB in DeleteLock() function
      Decouple archiving release notes from automated commits pipeline
      Decouple updating timings from automated commit pipeline
      [backport#15931 9/9] Remove getBlockDepth method from Chain::interface
      wallet: Refactor WalletRescanReserver to use wallet reference
      [backport#15931 8/9] Remove locked_chain from GetDepthInMainChain and its callers
      [backport#15931 7/9] Use CWallet::m_last_block_processed_height in GetDepthInMainChain
      [backport#15931 6/9] Only return early from BlockUntilSyncedToCurrentChain if current tip is exact match
      [backport#15931 5/9] Refactor some importprunedfunds checks with guard clause
      [backport#15931 4/9] Add block_height field in struct Confirmation
      [backport#15931 3/9] Replace CWalletTx::SetConf by Confirmation initialization list
      [backport#15931 2/9] Add m_last_block_processed_height field in CWallet
      Drop deprecated and unused GUARDED_VAR and PT_GUARDED_VAR annotations
      [backport#15931 1/9] Pass block height in Chain::BlockConnected/Chain::BlockDisconnected
      [Backport]util: Filter control characters out of log messages
      Remove extra CBlockIndex declaration
      test: Try once more when RPC connection fails on Windows
      [Backport]Add some general std::vector utility functions
      Decouple updating seeds from automated commits pipeline
      Decouple updating chainparams from automated commits pipeline
      Add a check for unstaged changes when generating automated commits
      [backport#14930]test: pruning: Check that verifychain can be called when pruned
      [buildbot] Strip out commonly used separators when detecting backports in diff summaries
      Decouple AUR patch recipe from the rest of the automated commits pipeline
      [land-bot] Bail early if there's nothing to land
      [CMAKE] Rename the man pages generation target
      [CMAKE] Silent the man pages generation
      Use pure python for functional tests schnorr computation
      [CMAKE] Move the manpages generation logic to doc/man
      Add ChaCha20Poly1305@Bitcoin AEAD implementation
      Add Poly1305 implementation
      Relayout a comment
      Remove dead folder obj-test
      Add ChaCha20 encryption option (XOR)
      update outdated links related to UAHF and use explicit MarkDown syntax
      [land-bot] Fix git HEAD after checking revision
      Use heredoc for outputting help text in automated commit scripts
      fix MarkDown links and code formatting
      [Backport] Fix Markdown formatting issues in init.md
      [LINTER] Accept hard line breaks in markdown files
      [CMAKE] Rename the test wrapper util
      Add missing release flag to the debian package build
      use markdown syntax for links (part 2)
      [land-bot] Call land-patch when generating automated commits
      [land-bot] Split land bot logic into distinct scripts
      [land-bot] Split arc land into distinct parts in preparation for supporting non-revision patches
      Remove freebsd build instructions
      use markdown syntax for links
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [land-bot] Use --nobranch for revisions by default
      Update the debian package build to use the generated man pages
      Revert "[land-bot] Add buildbot support for requests to land diff patches"
      Move version comparison utility functions to their own file
      Update the AUR packages builds to use the generated man pages
      [CMAKE] Exclude the man pages from the install-debug target
      [CMAKE] Fix NSIS installing failing to find the man pages
      [backport#15699] Remove no-op CClientUIInterface::[signal_name]_disconnect. Disconnect BlockNotifyGenesisWait and RPCNotifyBlockChange properly.
      Bump bitcoin-abc-qt AUR package version to 0.22.3
      Bump bitcoin-abc AUR package version to 0.22.3
      [Automated] Archive release notes for version 0.22.2
      Bump version to 0.22.3
      [CI] Add gettext-base to the base image
      [CMAKE] Generate the man pages at install time
      [DOC] Fix missing dependency in build-osx instructions
      [CMAKE] Fix issues when looking for libraries installed with homebrew
      [CI] Add the failed functional tests tmp directories to artifacts
      [QT] Fix QByteArray.append(const QString) deprecation warning in 5.15.1
      [DOC] Fix various doc issues, improve the linter accordingly
      Merge #18866: test: Fix verack race to avoid intermittent test failures
      Merge #18496: test: remove redundant sync_with_ping after add_p2p_connection
      Move man2html.sh in build/config
      [build] correctly set -fstack-reuse=none only for gcc
      [land-bot] Add buildbot support for requests to land diff patches
      [backport#17381 5/5] Add missing SetupGeneration error handling in EncryptWallet
      [backport#17381 4/5] Clean up nested scope in GetReservedDestination
      [backport#17381 3/5] Get rid of confusing LegacyScriptPubKeyMan::TopUpKeyPool method
      [backport#17381 2/5] Pass CTxDestination to ScriptPubKeyMan::GetMetadata
      [backport#17381 1/5] Add EnsureLegacyScriptPubKeyMan and use in rpcwallet.cpp
      [backport#17292] Add new mempool benchmarks for a complex pool
      [land-bot] Cleanup unnecessary brace expansion
      [land-bot] Cleanup unused code paths for diff patches
      [land-bot] Extract revision status check code
      [LINTER] Add a markdown linter
      [CI] Allow to configure a post build script
      [CMAKE] Make the doc-rpc target actually generate the rpc docs
      [Automated] Update timing.json
      [Automated] Update manpages
      [Automated] Update seeds
      [Automated] Update chainparams
      Add a bunch of release notes
      [backport#19022] test: Fix intermittent failure in feature_dbcrash
      [CMAKE] Add a check rule for the buildbot test
      [CMAKE] Don't ship the AUR package sources as part of the release
      [CI] Split the secp256k1 build in parts
      [CI] Add an option to select the generator
      [backport#17280] refactor: Change occurences of c_str() used with size() to data()
      [backport#14047 4/4] QA: add test for HKDF HMAC_SHA256 L32
      [backport#14047 3/4] Add HKDF HMAC_SHA256 L=32 implementations
      [backport#14047 2/4] QA: add test for CKey::Negate()
      [backport#14047 1/4] CKey: add method to negate the key
      [backport#17318] replace asserts in RPC code with CHECK_NONFATAL and add linter
      [backport#16285] rpc: Improve scantxoutset response and help message
      Bump bitcoin-abc AUR package version to 0.22.2
      Bump bitcoin-abc-qt AUR package version to 0.22.2
      [CI] Move the environment variables to the configuration object
      Add check-buildbot CI config
      [backport#15991] Bugfix: fix pruneblockchain returned prune height
      [backport#17316] refactor: Replace all uses of boost::optional with our own Optional type
      [backport#16911] wallet: Only check the hash of transactions loaded from disk
      [backport#16689 2/2] Add missing fields in TransactionDescriptionString and others
      [backport#16689 1/2] MOVEONLY : move RPC wallets helpers to TransactionDescriptionString
      [CI] Don't call the build_cmake.sh script
      [CI] Migrate the OSX cross build to use no script
      [CI] Add cross builds configuration
      [CI] Automated commits: use nameref to get the current version
      [backport#16397] doc: Clarify includeWatching for fundrawtransaction
      [CI] Automated commits: fix ninja not running from the build directory
      Clarify source control tools cmake flag
      [backport#16866] wallet: Rename 'decode' argument in gettransaction method to 'verbose'
      [CI] Archive the release notes on version change
      [CI] Automatically update the AUR package version
      [backport#16185 3/3] doc: Add release note for the new gettransaction argument
      [backport#16185 2/3] tests: Add a new functional test for gettransaction
      [backport#16185 1/3] gettransaction: add an argument to decode the transaction
      [backport#16503] Remove p2pEnabled from Chain interface
      [backport#16144] wallet: do not encrypt wallets with disabled private keys
      [land-bot] Extract out conduit token sanitization code
      Rename buildbot teamcity wrapper module
      [backport#16063] rpc: Mention getwalletinfo where a rescan is triggered
      [backport#15880] utils and libraries: Replace deprecated Boost Filesystem functions
      [backport#16071] RPC: Hint for importmulti in help output of importpubkey and importaddress
      [CI] Fix wrong extension for the configuration file in the build bot
      [CI] Migrate configuration file to YAML
      Add an option to the build_cmake.sh to skip the build phase
      Reduce noise in check-source-control-tools
      [LINTER] Add a YAML linter
      Add check-source-control-tools build config
      [CI] Ensure the artifact directory is always created
      [CMAKE] Add a target to print the current version number
      [CI] Allow for describing the build from the configuration file
      [backport#15917] wallet: Avoid logging no_such_file_or_directory error
      [backport#15583] wallet: Log and ignore errors in ListWalletDir and IsBerkeleyBtree
      Port the abcbot code into this repository
      Fix detection of binary open() calls in the Python encoding linter
      [backport#15426] [Doc] importmulti: add missing description of keypool option
      [backport#19507] Have zmq reorg test cover mempool txns
      [backport#19507] Add zmq test for transaction pub during reorg
      [backport#19507] Add test case for mempool->block zmq notification
      [backport#19507] Make ordering of zmq consumption irrelevant to functional test
      [backport#17445] zmq: Fix due to invalid argument and multiple notifiers
      [backport#16598] test: Remove confusing hash256 function in util
      [backport#16404] qa: Test ZMQ notification after chain reorg
      [backport#16404] qa: Refactor ZMQ test
      [backport#15209] zmq: log outbound message high water mark when reusing socket
      [backport#16404] doc: Add note regarding ZMQ block notification
      Merge #19632: test: Catch decimal.InvalidOperation from TestNodeCLI#send_cli
      [AUR] Use SSH to clone and update the repo
      Update AUR package to 0.22.1
      [CMAKE] Add an install target for the secp256k1 benchmarks
      [Automated] Update timing.json
      [Automated] Update manpages
      [Automated] Update seeds
      [Automated] Update chainparams
      [backport#15390] [wallet-tool] Close bdb when flushing wallet
      [backport#15122] [RPC] Expand help text for importmulti changes
      [backport#15334] wallet: Log absolute paths for the wallets
      [backport#15102] test: Run invalid_txs.InputMissing test in feature_block
      [backport#14268] Make SafeDbt DB_DBT_MALLOC on default initialization
      [backport#14268] Introduce SafeDbt to handle DB_DBT_MALLOC raii-style
      [backport#14268] Drop unused setRange arg to BerkeleyBatch::ReadAtCursor
      [CI] Move some more path definitions to the Configuration object
      [CI] Move the project root directory to the Configuration object
      [backport#14653] Test coinbase category in wallet rpcs
      [backport#14653] Add all category options to wallet rpc help
      [CI] Clear the artifacts directory before the build
      [CI] Prevent artifacts from the same diretory to fail the build
      [backport#14478] Show error to user when corrupt wallet unlock fails
      [backport#14890] rpc: Avoid creating non-standard raw transactions
      [backport#13966] gui: Show watch-only eye instead of HD disabled
      [backport#13966] Hide spendable label if private key is disabled
      [AUR] A few fixes to the update-aur.sh script
      [AUR] Make the update script executable
      [CI] Add pandoc to the base image
      [backport#16964] gui: Change sendcoins dialogue Yes to Send
      [backport#15886] Do not show list for the only recipient.
      [backport#15886] Show recipient list as detailedText of QMessageBox
      [backport#15886]Make SendConfirmationDialog fully fledged
      [backport#14771]test: Add BOOST_REQUIRE to getters returning optional
      [backport#17593][test] move wallet helper functions into test library
      [backport#17593][test] move mining helper functions into test library
      [backport#17593][test] move string helper functions into test library
      [Automated] Update manpages
      [backport#17342][refactor] Remove global int nScriptCheckThreads
      [backport#17342][tests] Don't use nScriptCheckThreads in the checkqueue_tests
      [backport#13551] tests: Fix incorrect documentation for test case cuckoocache_hit_rate_ok
      Add miner fund address list to getblocktemplate output
      Add wrapper function for miner fund amount
      [Automated] Update manpages
      Bump version to 0.22.2
      [CI] Run automated commit builds in separate directories
      [CI] Continuously update the man pages
      Add more flexibility to the update-aur.sh script
      Add a script to update the AUR packages
      Add the files from AUR repository to contrib/aur
      [CI] Prevent stdout buffer overflow
      [Automated] Update manpages
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [CMAKE] Flag the HTML man pages as generated
      [CMAKE] Transform man pages for web rendering
      [CMAKE] Don't enforce LC_ALL=C.UTF-8 for the cmake test wrapper script
      Cleanup autotools workaround in apptest.cpp
      Kill autotools
      [GITIAN] Use the new build targets to split debug info and stripped bin
      [CMAKE] Better debug install targets
      [CMAKE] Fix split debug not working for libs on Debian
      [CMAKE] Don't install secp256k1 by default unless it is standalone
      Add test coverage for getblocktemplate's sigoplimit
      [CMAKE] Fix the Info.plist template minimum version
      Add test coverage for getblocktemplate's mintime
      Add a test for ABC-specific getblocktemplate behavior
      [CMAKE] Install stripped and debug parts of targets
      build: pass -fcommon when building genisoimage
      [CMAKE] Make the split-debug script template executable
      [backport#15971] validation: Add compile-time checking for negative locking requirement in LimitValidationInterfaceQueue
      [backport#15402] Prevent callback overruns in InvalidateBlock and RewindBlockIndex
      build: Skip i686 build by default in gitian
      [CI] Only build the required static dependencies
      Fix comment referencing incorrect activation
      [CI] Cross build and run the tests for Linux 32 bits
      test: remove rapidcheck integration and tests
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [CMAKE] Add global installation targets
      [CMAKE] Wrap documentation installation into a function
      [CMAKE] Properly clean the junit reports
      [CMAKE] Move the test log files to test/log
      [CI] Push coverage data to Teamcity statistics
      Replace phonon activation time with the height it activated at
      Bump version to 0.22.1
      [CMAKE] Use new default for CMP0071
      Update makeseeds
      [RPC Docs] Use .html instead of .md extension for generated RPC docs
      [CMAKE] Allow for installing test executables
      [backport#16849] Fix block index inconsistency in InvalidateBlock()
      [thread safety] prevent double lock of cs_main in calls to CChainState::UnwindBlock
      [refactor] move Park and InvalidateBlock to CChainState public API
      Make descriptor test deterministic
      Fix a typo in a comment
      Output proper coinbase value in getblocktemplate once the new coinbase rule is active
      [Automated] Update manpages
      Add release notes
      Add the coinbase rule
      [test] A few more tests for ASERT
      Add aserti3-2d support
      Rename seed for bitcoinforks.org
      [RPC docs] Remove unused index files
      Remove Eoan from PPA releases
      Cleanup and revise release process
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      Ensure two newlines in help text between RPC command and description
      Fix typo in buildavalancheproof help text
      Move replay protection activation to May, 15 2021
      Document 2-complement assumption in assumption.h
      [backport#17304 18/18] Refactor: Move nTimeFirstKey accesses out of CWallet
      [backport#17304 17/18] Refactor: Move GetKeypoolSize code out of CWallet
      [backport#17304 16/18] Refactor: Move RewriteDB code out of CWallet
      [backport#17304 15/18] Refactor: Move SetupGeneration code out of CWallet
      [backport#17304 14/18] Refactor: Move HavePrivateKeys code out of CWallet::CreateWalletFromFile
      [backport#17304 13/18] Refactor: Move Upgrade code out of CWallet::CreateWalletFromFile
      [backport#17304 12/18] Refactor: Move MarkUnusedAddresses code out of CWallet::AddToWalletIfInvolvingMe
      [backport#17304 11/18] Refactor: Move GetMetadata code out of getaddressinfo
      [backport#17304 10/18] Refactor: Move LoadKey LegacyScriptPubKeyMan method definition
      [backport#17304 9/18] Refactor: Move SetAddressBookWithDB call out of LegacyScriptPubKeyMan::ImportScriptPubKeys
      [backport#17304 8/18] refactor: Replace UnsetWalletFlagWithDB with UnsetBlankWalletFlag in ScriptPubKeyMan
      [backport#17304 7/18] Refactor: Remove UnsetWalletFlag call from LegacyScriptPubKeyMan::SetHDSeed
      [backport#17304 6/18] Remove SetWalletFlag from WalletStorage
      [backport#17304 5/18] Refactor: Move SetWalletFlag out of LegacyScriptPubKeyMan::UpgradeKeyMetadata
      [backport#17304 4/18] Refactor: Move SetAddressBook call out of LegacyScriptPubKeyMan::GetNewDestination
      [backport#17304 3/18] Refactor: Add new ScriptPubKeyMan virtual methods
      [backport#17304 2/18] Refactor: Declare LegacyScriptPubKeyMan methods as virtual
      [backport#17304 1/18] MOVEONLY: Reorder LegacyScriptPubKeyMan methods
      [backport#16383 3/3] tests: functional watch-only wallet tests
      [backport#16383 2/3] rpcwallet: document include_watchonly default for watchonly wallets
      [backport#16383 1/3] rpcwallet: default include_watchonly to true for watchonly wallets
      Bump version to 0.22.0
      Clean up separated ban/discourage interface
      [backport#18417 3/3] tests: Add fuzzing harness for functions in net_permissions.h
      [backport#18417 2/3] tests: Add fuzzing harness for functions in timedata.h
      [backport#18417 1/3] tests: Add fuzzing harness for functions in addrdb.h
      [Automated] Update manpages
      [backport#18206] tests: Add fuzzing harness for bloom filter classes (CBloomFilter + CRollingBloomFilter)
      [backport#17300] LegacyScriptPubKeyMan code cleanups
      [backport#17260 3/3] Refactor: Split up CWallet and LegacyScriptPubKeyMan and classes
      [backport#17260 2/3] MOVEONLY: Move key handling code out of wallet to keyman file
      [backport#17260 1/3] Move wallet enums to walletutil.h
      [autotools] Fixed fuzzer build
      Ignore cppcheck syntax errors related to prevector
      Fix signed shift by 31 bits
      Allow large integers to be used in maxtries
      Polish release notes
      Add signed right shift assumption in assumptions.h
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [backport#16798] Refactor rawtransaction_util's SignTransaction to separate prevtx parsing
      [backport#17154 3/3][wallet] Remove `state` argument from CWallet::CommitTransaction
      [backport#17154 2/3][wallet] Remove return value from CommitTransaction()
      [backport#17154 1/3][wallet] Add doxygen comment to CWallet::CommitTransaction()
      [backport#15894 3/3] Make AbortNode() aware of MSG_NOPREFIX flag
      [backport#15894 2/3] Add MSG_NOPREFIX flag for user messages
      [backport#15894 1/3] Prepend the error/warning prefix for GUI messages
      Remove language sub-route from RPC doc permalinks
      [backport#15457] Check std::system for -[alert|block|wallet]notify
      [CI] Increase build-coverage timeout
      [backport#17070] wallet: Avoid showing GUI popups on RPC errors
      [backport#15450 5/5] Add Create Wallet menu action
      [backport#15450 4/5] Expose wallet creation to the GUI via WalletController
      [backport#15450 3/5] Add CreateWalletDialog to create wallets from the GUI
      [backport#15450 2/5] Optionally allow AskPassphraseDialog to output the passphrase
      [backport#15450 1/5] gui: Refactor OpenWalletActivity
      [CI] Make the bench use the ENABLE_JUNIT_REPORT option and use it on CI
      [backport#16394] Allow createwallet to take empty passwords to make unencrypted wallets
      [backport#15896] QA: feature_filelock, interface_bitcoin_cli: Use PACKAGE_NAME in messages rather than hardcoding Bitcoin Core
      [backport#16524] Wallet: Disable -fallbackfee by default
      [backport#16402] Remove wallet settings from chainparams
      [CMAKE] Generate textual coverage report
      [TRAVIS] Install cmake version 3.16
      [CMAKE] Run the leveldb tests serially
      [GITIAN] Stop distributing the *-unsigned.tar.gz archive
      [backport#17203] wallet: Remove unused GetLabelName
      [backport#17138 2/2][wallet] Remove pruning check for -rescan option
      [backport#17138 1/2][wallet] Remove package limit config access from wallet
      [CMAKE] Bump minimum cmake to 3.16
      Fix wallet_reorgsrestore functional test flakiness
      [GITIAN] Build with cmake 3.16 from the backport repository
      [DOC] Improve instructions for setting up shellcheck
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [backports#18561#18633] test: Properly raise FailedToStartError when rpc shutdown before warmup finished
      [backport#17633] tests: Add option --valgrind to run the functional tests under Valgrind
      [backport#15963] [tests] Make random seed logged and settable
      [backport#15927] [tests] log thread names by default in functional tests
      [backport#15415] [test] functional: allow custom cwd, use tmpdir as default
      [backport#14519] tests: add utility to easily profile node performance with perf
      [backport#14465] tests: Stop node before removing the notification file
      Do not generate RPC doc index files multiple times
      Fix RPC example for finalizeblock
      Fixup comment regarding finalized block
      [CI] Prevent copying undesired test/tmp content to the artifacts
      [DOC] clang-*-8 tools are now in the backport repository for Debian 10
      Don't generate junit report for functional test unless being asked to
      [CI] Enable Junit reporting via the build system and use it in scripts
      Disable Jemalloc for the debian package builds
      [backport#13546] wallet: Fix use of uninitialized value bnb_used in CWallet::CreateTransaction(...)
      [backport#14103] docs: Fix broken Doxygen comments
      [backport#16624 4/4] Add a test wallet_reorgsrestore
      [backport#16624 3/4] Modify wallet tx status if has been reorged out
      [backport#16624 2/4] Remove SyncTransaction for conflicted txn in CWallet::BlockConnected
      [backport#16624 1/4] Encapsulate tx status in a Confirmation struct
      [CMAKE] Move the functional tests junit reports to the test/junit dir
      [backport#15906] [wallet] Move min_depth and max_depth to coin control
      [CMAKE] Generate junit output for boost unit tests
      [backport#16952] gui: make sure to update the UI when deleting a transaction
      [backport#16796] wallet: Fix segfault in CreateWalletFromFile
      [backport#16620] util: Move ResolveErrMsg to util/error
      [backport#16745] wallet: Translate all initErrors in CreateWalletFromFile
      [backport#16557] [wallet] restore coinbase and confirmed/conflicted checks in SubmitMemoryPoolAndRelay()
      Set correct markdown extension for generated RPC docs
      Bump version to 0.21.13
      [backport#16572] wallet: Fix Char as Bool in Wallet
      Add golang to the CI base image
      [backport#16451 3/3][wallet] Remove CMerkleTx serialization logic
      [backport#16451 2/3][wallet] Flatten CWalletTx class hierarchy
      [backport#16451 1/3][wallet] Move CMerkleTx functions into CWalletTx
      Add Kent Beck link to CONTRIBUTING.md
      [backport#16399 3/3] Use switch on status in RpcWallet
      [backport#16399 2/3] Return error for ignored passphrase through disable private keys option
      [backport#16399 1/3] Place out args at the end for CreateWallet
      [backport#15901 2/2] remove extraneous scope
      [backport#15901 1/2] wallet: log on rescan completion
      Arc lint everything
      [backport#15530] doc: Move wallet lock annotations to header
      [Automated] Update manpages
      Trigger DAA underflow in DAA unit tests
      [backport#15853] wallet: Remove unused import checkpoints.h
      [backport#15491] wallet: Improve log output for errors during load
      [backport#14138] wallet: Set encrypted_batch to nullptr after delete. Avoid double free in the case of NDEBUG.
      [backport#13657] wallet: assert to ensure accuracy of CMerkleTx::GetBlocksToMaturity
      [backport#16502] wallet: Drop unused OldKey
      [backport#15709] wallet: Do not add "setting" key as unknown
      [backport#16475 2/2] wallet: Rename CWalletKey to OldKey
      [backport#16475 1/2] wallet: Enumerate walletdb keys
      [CMAKE] Add the test suite to the log name
      Fix the expected naming violation in test_runner.py
      [backport#15588 3/3] Remove ReadVersion and WriteVersion
      [backport#15588 2/3] Log the actual wallet file version
      [backport#15588 1/3] Remove nFileVersion from CWalletScanState
      Remove the rule that prevent retargeting on regtest from the EDA
      [backport#15870 3/3][doc] rpcwallet: Only fail rescan when blocks have been pruned
      [backport#15870 2/3] scripted-diff: Bump copyright headers in wallet
      [backport#15870 1/3] wallet: Only fail rescan when blocks have actually been pruned
      [backport#15730 4/4] doc: Add release notes for 15730
      [backport#15730 3/4] rpc: Show scanning details in getwalletinfo
      [backport#15730 2/4] wallet: Track current scanning progress
      [backport#15730 1/4] wallet: Track scanning duration
      [backport#16786] test: add unit test for wallet watch-only methods involving PubKeys
      Remove BIP9 miner fund.
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [refactor] factor CBlockIndex out of chain.h
      [backport#16361] Remove redundant pre-TopUpKeypool check
      [backport#16753] wallet: extract PubKey from P2PK script with Solver
      Fix OSX SDK caching in Gitian builds
      [GITIAN] Use docker for the build on CI
      [LINTER] Fix syntax error in the cppcheck linter
      Add build config for documentation, including RPC docs
      [refactor] remove global mapBlockIndex access from unparkblock RPC
      [GITIAN] Add documentation for building using Docker
      Renaming bswap_tests test case so that the name does not collide with the test suite name
      [refactor] remove global mapBlockIndex access from parkblock RPC
      [CI] Store functional tests duration with ms resolution in Junit
      [GITIAN] Remove vagrant support documentation for the gitian builds
      [GITIAN] Fix instructions for extracting the OSX SDK
      [GITIAN] Update scripts and docs to use the local gitian version
      [CMAKE] Don't distribute gitian as part of our sources package
      [GITIAN] Don't ignore target-bin/
      [GITIAN] Pull gitian sources in our repo
      Generate RPC docs using regtest
      [lint] update Avalanche change to circular dep status
      [refactor] access mapBlockIndex via function in Avalanche code
      [backport#17357 2/2] tests: Add fuzzing harness for Bech32 encoding/decoding
      [backport#17357 1/2] tests: Move CaseInsensitiveEqual to test/util/str
      Pass chain params down to GetNextWorkRequired
      Move CustomArgumentsFixture to an apropriate place
      Fix providing an explicit branch/ref to land-via-bot
      [backport#17051] tests: Add deserialization fuzzing harnesses
      Initialize nVersionDummy in txdb.cpp
      [avalanche] Add a ValidationState for Proof
      [CMAKE] Optionally install bitcoin-bench
      Move difficulty adjustement realted stuff in the pow folder
      [backport#15040] Add workaround for QProgressDialog bug on macOS
      Merge #16302: test: Add missing syncwithvalidationinterfacequeue to wallet_balance test
      Merge #15866: test: Add missing syncwithvalidationinterfacequeue to wallet_import_rescan
      Fix some functional test executable flags
      [CI] Allow for referencing multiple templates in a build configuration
      [CI] Rename environment configuration to env
      [CI] Run gitian builds through the build configuration
      [CI] Don't create the unused sanitizer log directory
      [backport#17136] tests: Add fuzzing harness for various PSBT related functions
      [CI] Fix wrong key for the environment variables in build configuration
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      Add Doxyfile to build-master artifacts
      Install base-devel by default for the example Arch Linux build in build-unix.md
      Use ninja for the debian packages build
      Disable optional Jemalloc for the example Arch Linux build in build-unix.md
      Add --distro flag to PPA release script
      [backport#10953] [Refactor] Combine scriptPubKey and amount as CTxOut in CScriptCheck
      [GITIAN] Make the setup feature from gitian-build.py create a debian VM
      Use cat to output help text in PPA release script
      Refactor FormatStateMessage() to better match Core
      Bump version to 0.21.12
      Merge #15921: validation: Tidy up ValidationState interface
      Merge #17746: refactor: rpc: Remove vector copy from listtransactions
      Fix uninitialized variable caught by cppcheck
      [refactor] refactor FinalizeBlockAndInvalidate
      [land-bot] Pass committer name/email to land bot endpoint
      [validation] Remove fMissingInputs from AcceptToMemoryPool()
      [avalanche] Add buildavalanchproof RPC call
      [backport#17291] tests: Add fuzzing harness for ISO-8601 related functions
      [backport#17083] tests: Add fuzzing harness for various CScript related functions
      [avalanche] Do not add invalid proof in PeerManager
      [avalanche] Generate proof that verify
      [avalanche] Move makeRandomProof to the test framework
      [avalanche] Change the addavalanchenode RPC so that it return if the operation succeeded
      Merge #14524: Trivial: fix typo
      Use RPCTypeCheck in avalanche's RPC
      [backports] tests: Skip unnecessary fuzzer initialisation. Hold ECCVerifyHandle only when needed.
      [correction] fix comment style for D6878
      [validation] Remove useless ret parameter from Invalid()
      [avalanche] Create addavalanchenode RPC call
      [backport#17018] tests: Add Parse(...) (descriptor) fuzzing harness
      [backport#17113] tests: Add fuzzing harness for descriptor Span-parsing helpers
      [backport#16887 3/3] test: add unit tests for Span-parsing helpers
      [backport#16887 2/3] Add documenting comments to spanparsing.h
      [backport#16887 1/3] Abstract out some of the descriptor Span-parsing helpers
      [avalanche] Add capability to verify proofs
      [validation] Remove unused first_invalid parameter from ProcessNewBlockHeaders()
      [validation] Remove error() calls from Invalid() calls
      [backport#17080] consensus: Explain why fCheckDuplicateInputs can not be skipped and remove it
      [Automated] Update manpages
      Ignore cppcheck error in EraseOrphanTx()
      [avalanche] First iteration on Proof
      [CI] Bump timeouts for cross builds
      Improve banman behavior comments
      Update -banscore and -bantime help text
      Remove confusing statement from setban RPC help text
      Clarify connection dropped message
      [secp256k1] Do not use unitialized multiset in multiset benchmark
      Report most cppcheck errors
      arc lint everything
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      [avalanche] Remove Peer::score
      [avalanche] Remove ability to rescore a peer
      [avalanche] Use Proof in Processor
      [backport#13910] Log progress while verifying blocks at level 4
      [SECP256K1] Fix the Travis build wrong targets names
      [backport#19188] test: Avoid overwriting the NodeContext member of the testing setup [-Wshadow-field]
      [avalanche] Kill PeerManager::addPeer
      [avalanche] Remove PeerManager::addNodeToPeer
      [avalanche] Attach Proof to each Peer
      [avalanche] Use ProofId instead of PeerId
      Put CConnmanTest in an anonymous namespace
      [backport#15245] remove deprecated mentions of signrawtransaction from fundraw help
      [backport#13310] Report progress in ReplayBlocks while rolling forward
      Merge #14734: fix an undefined behavior in uint::SetHex
      [avalanche] Rename getSuitableNodeToQuery => selectNode
      [avalanche] More namespace instead of smurfnaming
      [avalanche] Use namespace instead of smurfnaming
      Factor out SaltedUint256Hasher
      [avalanche] Use PerrManager instead of ad hoc logic in AvalancheProcessor
      [refactor] add const CChainParams& m_params to interface::ChainImpl
      [CI] Add a runOnDiff flag to the build configurations
      [avalanche] Factor AvalancheNode from PeerManager
      [avalanche] Move AvalancheNode to its own file
      [avalanche] Remove getPubkey API
      [avalanche] Add a facility to update node's timeout
      [avalanche] Add facilities to delete nodes from the PeerManager
      [avalanche] Add node related functions to the peermanager
      [avalanche] Add the notion of Peer to the PeerManager
      [avalanche] Use an hash_unique key for the node/round index in QuerySet
      Move NO_NODE to net.h
      [backport#15267] doc: explain AcceptToMemoryPoolWorker's coins_to_uncache
      [avalanche] Use std::chrono for time constants
      [avalanche] Use constexpr for global constants
      [CI] Improve teamcity error reporting by setting the failure message
      [avalanche] Bail when selecting a peer fails too many times
      [avalanche] Implement compaction for the PeerManager
      Cleanup leftover phononactivationtime option
      [backport#16415] Get rid of PendingWalletTx class.
      [backport#16208 2/2] Restrict lifetime of ReserveDestination to CWallet::CreateTransaction
      [backport#16208 1/2] CreateTransaction calls KeepDestination on ReserveDestination before success
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      [backport#17009] tests: Add EvalScript(...) fuzzing harness
      [linter] add exceptions to include guards and file name linters
      [backport#17076] tests: Add fuzzing harness for CheckTransaction(...), IsStandardTx(...) and other CTransaction related functions
      Cleanup unused --with-phononactivation from test_framework
      Cleanup phonon activation in functional tests
      [backport#16237 3/3] Add GetNewChangeDestination for getting new change Destinations
      [backport#16237 2/3] Replace CReserveKey with ReserveDestinatoin
      [backport#16237 1/3] Add GetNewDestination to CWallet to fetch new destinations
      [backport#16542 4/4] Additional tests for other failure cases
      [backport#16542 3/4] Check error messages in descriptor tests
      [backport#pr16542 2/4] Give more errors for specific failure conditions
      [backport#16542 1/4] Return an error from descriptor Parse that gives more information about what failed
      [avalanche] Manipulate peers via PeerId
      [avalanche] Make peer score an uint32_t instead of an uint64_t
      [backport#15639 3/3] bitcoin-wallet tool: Drop libbitcoin_server.a dependency
      [backport#15492 2/2] [cleanup] Remove unused CReserveKey
      [backport#15492 1/2] [rpc] simplify generate RPC
      [fix] replace leftover dustRelayFee symbols in wallet/wallet.cpp
      [fix] actually move tx_check.cpp from libserver to libbitcoin consensus
      [backport#15492] [rpc] remove deprecated generate method
      [avalanche] Build Slot using start and score rather than start/stop
      [avalanche] Early bail when no matchign slot exists
      [avalanche] Track slots as start+score rather than start/stop
      Add an exception for the TSAN lock-order-inversion on reverselock_tests
      Add an exception for UBSAN vptr on boost::unit_test::decorator::timeout
      [avalanche] Abstract the Slot infos so that layout can be changed easily
      [avalanche] separate avalanche.{h|cpp} into more relevent files
      Enable Axion upgrades in functional tests
      Enable Axion upgrades in unit tests
      Add facility and test for checking if Axion upgrade is enabled
      [avalanche] Use utility method to find slots
      [avalanche] Add the ability to remove and rescore peers
      [avalanche] Introduce a datastructure to keep track of peers
      Merge #14543: [QA] minor p2p_sendheaders fix of height in coinbase
      [CI] Log sanitizers output to stdout instead of log files
      Move NodeContext from TestingSetup to BasicTestingSetup
      test: Check that wait_until returns if time point is in the past
      refactor: Make scheduler methods type safe
      [CI] Run the check target instead of a list of subtargets
      [seeder] Use testutil library in seeder tests
      Move avalanche in its own folder
      Move minimum boost version to 1.59
      build: Create test utility library from src/test/util/
      Remove dead checkpoint test
      Bump version to 0.21.11
      [CI] Increase stream buffer limit for build subprocess
      [CI] Fix wrong configuration when there is no template
      Add missing fi in secp256k1's travis script
      [CI] Refactor the build by making it a class
      [CI] Make the build configuration a class
      [CI] Filter what is printed to the console and log it to files
      [CI] Manage the build artifacts from the configuration
      [backport#17069] tests: Pass fuzzing inputs as constant references
      [Automated] Update manpages
      [CI] Ensure llvm-symbolizer is available in PATH
      Separate reason for premature spends (coinbase/locktime)
      Assert validation reasons are contextually correct
      [avalanche] Add test for the parking scenario
      [refactor] Update some comments in validation.cpp as we arent doing DoS there
      [refactor] Drop unused state.DoS(), state.GetDoS(), state.CorruptionPossible()
      [CI] Add a templating system to the configuration
      [CI] Include the teamcity-messages library and display the build name
      CorruptionPossible -> TX_WITNESS_MUTATED
      Fix build path in build-make-generator
      Use reason for checking for sigcheck in txvalidationcache
      [CI] Cleanup unused build-configurations wrapper
      [CI] Make sure every build directory is under a common 'build' directory
      scripted-diff: Remove DoS calls to CValidationState
      [refactor] Prep for scripted-diff by removing some \ns which annoy sed.
      Allow use of state.Invalid() for all reasons
      Fix handling of invalid headers
      [CI] Allow for setting environment variables from the configuration file
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Use BlockHash in net_processing.cpp
      [refactor] Use Reasons directly instead of DoS codes
      CorruptionPossible -> BLOCK_MUTATED
      [CI] Set build timeout in the configuration file
      [LINTER] Enforce shellcheck >= 0.7.0
      Merge #14051: [Tests] Make combine_logs.py handle multi-line logs
      Merge #14816: Add CScriptNum decode python implementation in functional suite
      Merge #14658: qa: Add test to ensure node can generate all rpc help texts at runtime
      [CI] Fix a bug where CI scripts are not able to build in directories that are not one level below the project root
      [CI] Convert the build-configurations.sh script to python, read config
      [CI] Split build-configurations.sh into individual scripts
      [CI] Extract the CI build facitilties to it's own script and source it
      [land-bot] Skip sanity check during arc patch
      Merge #18412: script: fix SCRIPT_ERR_SIG_PUSHONLY error string
      Merge #11418: Add error string for CLEANSTACK script violation
      Add jemalloc as a dependency on osx
      Update .arcconfig phabricator URI to the new syntax
      LookupBlockIndex -> CACHED_INVALID
      [refactor] Drop redundant nDoS, corruptionPossible, SetCorruptionPossible
      [refactor] Add useful-for-dos "reason" field to CValidationState
      [CI] Wrap build_depends.sh on ibd.sh into functions
      [CI] Setup don't need to be a function
      Do not resuse state in checkpoints_tests.cpp
      Make sigcheck violation report invaid rather than non standard
      [backport#15751] Speed up deriveaddresses for large ranges
      [backport#16326] [RPC] add new utxoupdatepsbt arguments to the CRPCCommand and CPRCConvertParam tables
      [backport#15427 3/3] Add support for descriptors to utxoupdatepsbt
      [backport#15427 2/3] Abstract out UpdatePSBTOutput from FillPSBT
      [backport#15427 1/3] Abstract out EvalDescriptorStringOrObject from scantxoutset
      [backport#16512] rpc: Shuffle inputs and outputs after joining psbts
      [backport#10574] Remove includes in .cpp files for things the corresponding .h file already included
      [CI] Remove duplicated `cd` to the build dir with build-make-generator
      Clean up banning levels
      [refactor] drop IsInvalid(nDoSOut)
      [CI] Wrap the build_autotools.sh script in a function
      [refactor] Refactor misbehavior ban decisions to MaybePunishNode()
      [CI] Make the build_autotools.sh script take a list of targets
      [CI] Wrap the build_cmake.sh environment and script path into a function
      [CI] Add an option to select the compiler to build_cmake.sh
      Remove double if in tx_verify.cpp
      test: Add basic test for BIP34
      [CI] Make build-configurations.sh take the build name as an argument
      [CI] Don't print an error if there is no sanitizer log files
      [CI] Add a build plan to run clang-tidy on the changed files
      Revert "[CI] Install the latest wine version from the winehq repository"
      test: add invalid tx templates for use in functional tests
      [refactor] rename stateDummy -> orphan_state
      Merge #13418: Docs: More precise explanation of parameter onlynet
      Merge #13457: tests: Drop variadic macro
      [backport#16322] wallet: Fix -maxtxfee check by moving it to CWallet::CreateTransaction
      [backport#14935] tests: Test for expected return values when calling functions returning a success code
      [backport#16079] wallet_balance.py: Prevent edge cases
      [backport#14818] Bugfix: test/functional/rpc_psbt: Remove check for specific error message that depends on uncertain assumptions
      [avalanche] Increase quorum size in the test.
      [avalanche] Remove blocks not worth pollling from the vote reccords rather than just ignore them
      [CI] Install the latest wine version from the winehq repository
      Merge #18563: test: Fix unregister_all_during_call cleanup
      Merge #18551: Do not clear validationinterface entries being executed
      Merge #18524: refactor: drop boost::signals2 in validationinterface
      Merge #16688: log: Add validation interface logging
      Merge #15999: init: Remove dead code in LoadChainTip
      [backport#15559] doc: correct analysepsbt rpc doc
      Merge #18338: Fix wallet unload race condition
      Add cppcheck to base image setup
      Merge #13577: logging: avoid nStart may be used uninitialized in AppInitMain warning
      Merge #12401: Reset pblocktree before deleting LevelDB file
      Merge #15486: [addrman, net] Ensure tried collisions resolve, and allow feeler connections to existing outbound netgroups
      Merge #15824: docs: Improve netbase comments
      Merge #16412: net: Make poll in InterruptibleRecv only filter for POLLIN events.
      Fix missing braces
      Merge #16355: refactor: move CCoinsViewErrorCatcher out of init.cpp
      [backport#15986] Add unmodified-but-with-checksum to getdescriptorinfo
      [backport#15986] Factor out checksum checking from descriptor parsing
      [CI] Temporarly fix adoptopenjdk8 failure
      tests: Make coins_tests/updatecoins_simulation_test deterministic
      tests: Make updatecoins_simulation_test deterministic
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      Merge #16188: net: Document what happens to getdata of unknown type
      [backport#15744] refactor: Extract ParseDescriptorRange
      [backport#15497] Make deriveaddresses use stop/[start,stop] notation for ranges
      [backport#15497] Use stop/[start,stop] notation in importmulti desc range
      [backport#15497] Add support for stop/[start,stop] ranges to scantxoutset
      [backport#15497] Support ranges arguments in RPC help
      [backport#15497] Add ParseRange function to parse args of the form int/[int,int]
      [backport#15368] Add checksums to descriptors.md
      [backport#15368] Make descriptor checksums mandatory in deriveaddresses and importmulti
      [backport#15368] Add getdescriptorinfo to compute checksum
      generatetoaddress should mention ABC wallet not Core
      Merge #14632: Tests: Fix a comment
      Leftovers from PR14119
      [backport#15368] Descriptor checksum
      [CI] Make the coverage report available to Teamcity
      [CI] Add a build-coverage target
      [Qt] Fix deprecated QButtonGroup::buttonClicked event
      [Qt] Fix deprecated QString::SplitBehavior (now Qt::SplitBehavior)
      [Qt] Fix deprecated QSignalMapper::mapped event
      [avalanche] Poll all candidate tips
      Always enable sigcheck in the mempool admission
      Use the new Check facility in wallet/rpcwallet.cpp
      [backport#12727] Remove unreachable help conditions
      [RPC docs] Fix build paths
      [backport#15337] rpc: Fix for segfault if combinepsbt called with empty inputs
      [backport#11590] [Wallet] always show help-line of wallet encryption calls
      [backport#17362] test: speed up wallet_avoidreuse, add logging
      [backport#16917] tests: Move common function assert_approx() into util.py
      [backport#16659] refactoring: Remove unused includes
      Use the new Check facility in rpc/rawtransaction.cpp
      Bump version to 0.21.10
      Use the new Check facility in rpc/misc.cpp
      Use the new Check facility is rpc/blockchain.cpp
      Use the new Check facility in rpc/net.cpp
      Use the new Check facility is wallet/rpcdump.cpp
      [Qt] Fix deprecated QDateTime(const QDate &)
      [Qt] Fix deprecated pixmap() return by pointer with Qt 5.15
      Use the new Check facility in rpc/mining.cpp
      Merge #14719: qa: Check specific reject reasons in feature_block
      Use Check facility is ZeroMQ RPC
      Use the new Check facility in rpc/server.cpp
      Update Bitcoin ABC RPC to use the Check facility
      [backport#16227 8/8] Move WatchOnly stuff from SigningProvider to CWallet
      Remove activation logic for chained transaction limit
      [avalanche] Process AvalancheResponse and act based on the result.
      [Qt] Remove unused WindowFlags parameters
      Consistently use the QT_VERSION_CHECK macro
      Cleanup Qt branches on old unsupported version
      [CI] Fix wrong path to bitcoind binary on build-ibd-*
      Make CLIENT_VERSION constexpr
      Continue relaying transactions after they expire from mapRelay
      Always repport proper sigcheck count
      CBlockTreeDB::ReadReindexing => CBlockTreeDB::IsReindexing
      Update avalanche integration test to use a quorum
      [backport#16227 7/8] Remove CCryptoKeyStore and move all of it's functionality into CWallet
      [backport#16227 6/8] Move various SigningProviders to signingprovider.{cpp,h}
      [backport#16227 5/8] Move KeyOriginInfo to its own header file
      [backport#16227 4/8] scripted-diff: rename CBasicKeyStore to FillableSigningProvider
      [backport#16227 3/8] Move HaveKey static function from keystore to rpcwallet where it is used
      [backport#16227 2/8] Remove CKeyStore and squash into CBasicKeyStore
      [backport#16227 1/8]Add HaveKey and HaveCScript to SigningProvider
      [CI] Prevent ccache crosstalk by building in separate directories
      Merge #14700: qa: Avoid race in p2p_invalid_block by waiting for the block request
      [backport#16026] Ensure that uncompressed public keys in a multisig always returns a legacy address
      [backport#15831] test: Add test that addmultisigaddress fails for watchonly addresses
      [LINTER] Check for missing explicit
      [CMAKE] Fail early if a lib header is missing, remove garbage in version
      [CMAKE] Don't require jemalloc for systems where it's the default
      Fix single parameter constructors not marked "explicit"
      Lint everything
      [Automated] Update manpages
      Add a lock on cs_main while modifying the config in setexcessiveblock
      txmempool: Remove unused default value MemPoolRemovalReason::UNKNOWN
      Merge #16092: Don't use global (external) symbols for symbols that are used in only one translation unit
      Merge #15622: Remove global symbols: Avoid using the global namespace if possible
      Merge #12980: Allow quicker shutdowns during LoadBlockIndex()
      Replace automatic bans with discouragement filter
      [CI] Do not enable debug for TSAN builds
      [backport#13531] doc: Clarify that mempool txiter is const_iterator
      [backport#16908] txmempool: Make entry time type-safe (std::chrono)
      [backport#16908] util: Add count_seconds time helper
      [backport#16908] test: mempool entry time is persisted
      [backport#14931] test: mempool_persist: Verify prioritization is dumped correctly
      [backport#14704]: doc: add detached release notes for #14060
      Drop unused reverselock.h
      scheduler: switch from boost to std
      sync.h: add REVERSE_LOCK
      scheduler: don't rely on boost interrupt on shutdown
      UninterruptibleSleep in avalanche test
      test: Fix bug in blockfilter_index_tests.
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [backport#16117] util: Remove unused MilliSleep
      [backport#16117] scripted-diff: Replace MilliSleep with UninterruptibleSleep
      [backport#16117] util: Add UnintrruptibleSleep
      [backport#17091] tests: Add test for loadblock option and linearize scripts
      Use std::condition_variable and sync.h instead of boost in scheduler_tests.cpp
      [backport#15305] [validation] Crash if disconnecting a block fails
      [rpc] expose ability to mock scheduler via the rpc
      Make the RPCHelpMan aware of JSONRPCRequest and add Check() helper
      rpc: migrate JSONRPCRequest functionality into request.cpp
      Merge #14626: Select orphan transaction uniformly for eviction
      Add username and ip logging for RPC method requests
      [lib] add scheduler to node context
      [test] add chainparams property to indicate chain allows time mocking
      [test] unit test for new MockForward scheduler method
      [util] allow scheduler to be mocked
      Move jsonrpcrequest to request
      [backport#16239] docs: release note wording
      [backport#16239] wallet/rpc: use static help text
      [backport#16239] wallet/rpc/getbalances: add entry for 'mine.used' balance in results
      Increase maxconnections limit when using poll.
      Implement poll() on systems which support it properly.
      Move SocketEvents logic to private method.
      Move GenerateSelectSet logic to private method.
      Introduce and use constant SELECT_TIMEOUT_MILLISECONDS.
      Merge #15597: net: Generate log entry when blocks messages are received unexpectedly
      Merge #15718: docs: Improve netaddress comments
      [backport#16898] test: Remove unused connect_nodes_bi
      [backport#16898] scripted-diff: test: Replace connect_nodes_bi with connect_nodes
      [backport#16898] test: Use connect_nodes when connecting nodes in the test_framework
      Merge #13503: Document FreeBSD quirk. Fix FreeBSD build: Use std::min<int>(...) to allow for compilation under certain FreeBSD versions.
      Merge #16073: refactor: Improve CRollingBloomFilter::reset by using std::fill
      Merge #15343: [doc] netaddress: Make IPv4 loopback comment more descriptive
      Merge #15254: Trivial: fixup a few doxygen comments
      Merge #15194: Add comment describing fDisconnect behavior
      Merge #15078: rpc: Document bytessent_per_msg and bytesrecv_per_msg
      Merge #14436: doc: add comment explaining recentRejects-DoS behavior
      Merge #14054: p2p: Disable BIP 61 by default
      [backport#14060] ZMQ: add options to configure outbound message high water mark, aka SNDHWM
      [backport#13756] bitcoind: update -avoidpartialspends description to account for auto-enable for avoid_reuse wallets
      [backport#13756] doc: release notes for avoid_reuse
      [backport#13756] wallet: enable avoid_partial_spends by default if avoid_reuse is set
      [backport#13756] test: add test for avoidreuse feature
      [backport#13756] Wallet/rpc: add 'avoid_reuse' option to RPC commands
      [backport#13756] wallet/rpc: add setwalletflag RPC and MUTABLE_WALLET_FLAGS
      [backport#13756] wallet: enable avoid_reuse feature
      Merge #13096: [Policy] Fix MAX_STANDARD_TX_WEIGHT check
      [backport#15930] rpc: Deprecate getunconfirmedbalance and getwalletinfo balances
      [backport#15930] rpc: Add getbalances RPC
      [backport#15930] rpcwallet: Make helper methods const on CWallet
      [backport#15930] wallet: Use IsValidNumArgs in getwalletinfo rpc
      [backport#15758] test: Add reorg test to wallet_balance
      [backport#15758] test: Check that wallet txs not in the mempool are untrusted
      [backport#15758] test: Add getunconfirmedbalance test with conflicts
      [backport#15758] test: Add wallet_balance test for watchonly
      [bugfix] prevent nodes from banning other nodes in ABC tests
      Merge #17931: test: Fix p2p_invalid_messages failing in Python 3.8 because of warning
      Don't relay addr messages to block-relay-only peers
      Add 2 outbound block-relay-only connections
      [backport#18247] test: Remove redundant sync_with_ping after add_p2p_connection
      [backport#18247] test: Wait for both veracks in add_p2p_connection
      Merge #15697: qa: Make swap_magic_bytes in p2p_invalid_messages atomic
      Merge #15330: test: Fix race in p2p_invalid_messages
      [land-bot] Point land bot at bitcoinabc.org
      Skip stale tip checking if outbound connections are off or if reindexing.
      Fire TransactionRemovedFromMempool from mempool
      scripted-diff: Replace ::mempool with m_node.mempool in tests
      Explicitely pass the mempool down in some test
      Have importwallet use ImportPrivKeys and ImportScripts
      Optionally allow ImportScripts to set script creation timestamp
      Disconnect peers violating blocks-only mode
      doc: improve comments relating to block-relay-only peers
      test: Replace recursive lock with locking annotations
      node: Add reference to mempool in NodeContext
      Check that tx_relay is initialized before access
      Add comment explaining intended use of m_tx_relay
      Add tests and documentation for blocksonly
      Have importaddress use ImportScripts and ImportScriptPubKeys
      Have importpubkey use CWallet's ImportScriptPubKeys and ImportPubKeys functions
      Have importprivkey use CWallet's ImportPrivKeys, ImportScripts, and ImportScriptPubKeys
      [backport#16551] test: Test that low difficulty chain fork is rejected
      [backport#16551] test: Pass down correct chain name in tests
      Change ImportScriptPubKeys' internal to apply_label
      [backport#16839] Avoid using g_rpc_node global in wallet code
      [backport#16244] Move wallet creation out of the createwallet rpc into its own function
      [backport#15006] Add option to create an encrypted wallet
      [backport#15713] Tidy up BroadcastTransaction()
      Log when an import is being skipped because we already have it
      [backport#15713 4/5] Remove unused submitToMemoryPool and relayTransactions Chain interfaces
      [backport#15713 3/5] Remove duplicate checks in SubmitMemoryPoolAndRelay
      [backport#15713 2/5] Introduce CWalletTx::SubmitMemoryPoolAndRelay
      [backport#16839] scripted-diff: Remove g_connman, g_banman globals
      net: Remove unused unsanitized user agent string CNode::strSubVer
      [refactor] Change tx_relay structure to be unique_ptr
      [refactor] Move tx relay state to separate structure
      [backport#16839] Pass NodeContext, ConnMan, BanMan references more places
      Change ismine to take a CWallet instead of CKeyStore
      [backport#15728] [wallet] Refactor CWalletTx::RelayWalletTransaction()
      [backport#15452] GetKeyBirthTimes should return key ids, not destinations
      Move ismine to wallet module
      [backport#14678] [wallet] remove redundant KeyOriginInfo access, already done in CreateSig
      [backport#15452] Replace CScriptID and CKeyID in CTxDestination with dedicated types
      [backport#14821] Replace CAffectedKeysVisitor with descriptor based logic
      Remove unused variable
      Use BlockHash for vInventoryBlockToSend
      [backport#15750] [rpc] Remove the addresses field from the getaddressinfo return object
      Merge #15246: qa: Add tests for invalid message headers
      Simplify install instructions for linter dependencies
      Add remaining linter dependencies to CI base image
      [land-bot] Clarify review status error message
      Batch write imported stuff in importmulti
      [devtools] Use -daemon instead of backgrounding bitcoind
      [backport#16898] test: Reformat python imports to aid scripted diff
      Fix: importmulti only imports origin info for PKH outputs
      [backport#13756] wallet: avoid reuse flags
      Use a single wallet batch for UpgradeKeyMetadata
      Add facility to generate RPC docs
      Improve handling of INVALID in IsMine
      Fix UniValue .write() changes for C++98
      [devtools] Use a trap to cleanup bitcoind instead of a background process
      util: Add Join helper to join a list of strings
      util: refactor upper/lowercase functions
      [CI] Disable jemalloc for running the tests with wine
      [CI] Let build_cmake.sh take a list of targets and use it
      [backport#13756] wallet: make IsWalletFlagSet() const
      [backport#13756] wallet: rename g_known_wallet_flags constant to KNOWN_WALLET_FLAGS
      [refactor] add const CCoinControl& param to SendMoney
      [backport#15777] [docs] Add doxygen comment for CKeyPool
      Import watch only pubkeys to the keypool if private keys are disabled
      blockfilter: Update BIP 158 test vectors.
      rpc: Add getblockfilter RPC method.
      Update multiset hash benchmark to use get_iters
      init: Add CLI option to enable block filter index.
      Merge #13047: [trivial] Tidy blocktools.py
      Merge #12856: Tests: Add Metaclass for BitcoinTestFramework
      [CI] Disable jemalloc for running the JNI bindings tests
      Context isn't freed in the ECDH benchmark
      Use jemalloc as a default
      [CMAKE] Prevent using jemalloc with the sanitizers
      Suppress a harmless variable-time optimization by clang in memczero
      Remove symbols exported by jemalloc from the symbols check
      Fix the build with Qt 5.15
      Remove memcpy compatibility for glibc < 2.14
      test: Create new test library
      test: Add RegTestingSetup to setup_common
      test: move-only ComputeFilter to src/test/lib/blockfilter
      Remove fdelt_chk back-compat code and sanity check
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      [backport#15777] [wallet] move-only: move CReserveKey to be next to CKeyPool
      [backport#15780] wallet: add cachable amounts for caching credit/debit values
      Rename Seeder's CAddrInfo -> CSeederAddrInfo
      index: Access functions for global block filter indexes.
      gui: Fix shutdown order
      Add arcanist land bot workflow
      [CMAKE] Fix a -Wpointer-to-int-cast when searching Jemalloc
      test: Unit test for block filter index reorg handling.
      test: Unit tests for block index filter.
      index: Implement lookup methods on block filter index.
      index: Implement block filter index with write operations.
      blockfilter: Functions to translate filter types to/from names.
      index: Ensure block locator is not stale after chain reorg.
      index: Allow atomic commits of index state to be extended.
      Use BlockHash in BlockFilter
      Fix Debian package script in case signer has multiple user IDs
      Merge #14426: utils: Fix broken Windows filelock
      Use BlockHash and TxId in zmq
      [backport#16129] Include core_io.h from core_read.cpp
      [backport#16129] Make reasoning about dependencies easier by not including unused dependencies
      [backport#15139] util: Make ToLower and ToUpper take a char
      [backport#14599] Use functions guaranteed to be locale independent
      [test] add a couple test cases to uint256_tests.cpp
      Fix a comment in validation.cpp
      [CMAKE] Remove the ENABLE_WERROR option
      [CI] Use proper argument handling in build_cmake.sh
      [CI] Enable -Werror where possible
      [backport#14802] rpc: faster getblockstats using BlockUndo data
      [CI] Install Clang 10 and use it for the werror build
      Allow overriding default flags
      [CMAKE] Rename secp256k1 test targets
      Merge #13160: wallet: Unlock spent outputs
      Merge #13507: RPC: Fix parameter count check for importpubkey
      Merge #13535: [qa] wallet_basic: Specify minimum required amount for listunspent
      Merge #13545: tests: Fix test case streams_serializedata_xor. Remove Boost dependency.
      Merge #13116: Add Clang thread safety annotations for variables guarded by cs_{rpcWarmup,nTimeOffset,warnings}
      Merge #16481: Trivial: add missing space
      Merge #12330: Reduce scope of cs_wallet locks in listtransactions
      Add land bot dependencies to base image setup script
      Generate assumed blockchain and chainstate disk sizes when updating chainparams
      [CMAKE] BOOST_TEST_DYN_LINK is defined twice
      [CMAKE] Add a facility to add flag groups and use it for -Wformat-*
      [CMAKE] Allow for checking support for several flags at the same time
      [backport#15623] refactor: Expose UndoReadFromDisk in header
      [backport#15932] rpc: Add lock annotations to block{,header}ToJSON
      [backport#15932] rpc: Serialize in getblock without cs_main
      [backport#15932] rpc: Use IsValidNumArgs in getblock
      Make nChainTx private, ass facility to update it
      Wrap nChainTx into GetChainTxCount
      Make env data logging optional
      Merge #15345: net: Correct comparison of addr count
      Remove CBlockIndex::SetNull
      [wallet] abort when attempting to fund a transaction above maxtxfee
      Pass Consensus::Params to load block index family of functions
      Fix WSL file locking by using flock instead of fcntl
      Bump version to 0.21.9
      [backport#15139] util: remove unused [U](BEGIN|END) macros
      [backport#15139] Replace use of BEGIN and END macros on uint256
      [backport#14518] rpc: Always throw in getblockstats if -txindex is required
      [backport#15458] refactor: Drop redundant wallet reference
      Prevent -Wcast-align in sha256_shani.cpp
      Disable some more leveldb warnings
      The -Wredundant-move warning is C++ only
      Fix unused -pie flag for libs
      [Automated] Update manpages
      Fixup release notes
      [backport#15365] wallet: Add lock annotation for mapAddressBook
      Avoid non-trivial global constants in SHA-NI code
      Fix deprecated ByteSize() for protobuf >= 3.1
      [backport#15713] Add BroadcastTransaction utility usage in Chain interface
      [backport#16452] refactor : use RelayTransaction in BroadcastTransaction utility
      [backport#16034] scripted-diff: Rename LockAnnotation to LockAssertion
      [backport#15435] Merge #15435: rpc: Add missing #include
      refactor: Fix implicit value conversion in formatPingTime
      Fix deprecated copy warning for PrecomputedTransactionData
      Fix shadow warning
      [Automated] Update timing.json
      [Automated] Update seeds
      [Automated] Update chainparams
      [backport#16034] Move LockAnnotation from threadsafety.h (imported code) to sync.h (our code)
      [backport#15855] [refactor] interfaces: Add missing LockAnnotation for cs_main
      Revert "[backport#15639] bitcoin-wallet tool: Drop libbitcoin_server.a dependency"
      [wallet] Move maxTxFee to wallet
      rpc: Uncouple non-wallet rpcs from maxTxFee global
      [backport#15842] refactor: replace isPotentialtip/waitForNotifications by higher method
      [backport#15784] rpc: Remove dependency on interfaces::Chain in SignTransaction
      [backport#15670] refactor: combine Chain::findFirstBlockWithTime/findFirstBlockWithTimeAndHeight
      [backport#15639] bitcoin-wallet tool: Drop libbitcoin_server.a dependency
      [backport#15639] Remove access to node globals from wallet-linked code
      wallet/rpc: sendrawtransaction maxfeerate
      [BUILD] Search and include OpenSSL only where required
      Fix CPUID subleaf iteration
      fix wrong include prior to backporting 15639
      [CMAKE] Move the OpenSSL symbol detection to Qt rather than config
      [refactor] make ArgsManager& parameter in IsDeprecatedRPCEnabled const
      [backport#14453] rpc: Fix wallet unload during walletpassphrase timeout
      [backport#15652] qa: Check unconfirmed balance after loadwallet
      [backport#15652] wallet: Update transactions with current mempool after load
      [backport#15652] interfaces: Add Chain::requestMempoolTransactions
      [backport#15652] wallet: Move CWallet::ReacceptWalletTransactions locks to callers
      [autotools-build] Disable _FORTIFY_SOURCE when enable-debug
      random: Remove remaining OpenSSL calls and locking infrastructure
      random: stop retrieving random bytes from OpenSSL
      random: stop feeding RNG output back into OpenSSL
      [build] set _FORTIFY_SOURCE=2 for -O* builds only
      refactor: Add handleNotifications method to wallet
      bench: Add wallet_balance benchmarks
      Fix up release notes
      [backport#15644] Interrupt orphan processing after every transaction
      [backport#15644] [MOVEONLY] Move processing of orphan queue to ProcessOrphanTx
      Test importing descriptors with key origin information and add release notes
      Import KeyOriginData when importing descriptors
      Implement a function to add KeyOriginInfo to a wallet
      Store key origin info in key metadata
      Add a method to CWallet to write just CKeyMetadata
      [backport#15644] Simplify orphan processing in preparation for interruptibility
      [backport#15639] bitcoin-wallet tool: Drop MakeChain calls
      Remove hdmasterkeyid
      Add WriteHDKeypath function and move *HDKeypath to util/bip32.{h,cpp}
      Refactor keymetadata writing to a separate method
      Merge #15746: rpc: RPCHelpMan: Always name dictionary keys
      Merge #14417: Fix listreceivedbyaddress not taking address as a string
      Merge #14129: Trivial: update clang thread-safety docs url
      wallet: Get all balances in one call
      doc: Add release notes for 15596
      wallet: Remove unused GetLegacyBalance
      scripted-diff: wallet: Rename pcoin to wtx
      rpc: Document that minconf is an ignored dummy value
      [refactor] change orphan txs std::map member to use TxId instead of uint256
      rpc: Actually throw help when passed invalid number of params
      RPCHelpMan: Check default values are given at compile-time
      rpc: Document default values for optional arguments
      random: mark RandAddPeriodic and SeedPeriodic as noexcept
      Report amount of data gathered from environment
      Use thread-safe atomic in perfmon seeder
      Run background seeding periodically instead of unpredictably
      Add information gathered through getauxval()
      Feed CPUID data into RNG
      [rpc] util: add deriveaddresses method
      Fix code alignement in rpc/misc.cpp
      [backport#15617] Do not relay banned IP addresses
      netaddress: Update CNetAddr for ORCHIDv2
      Use sysctl for seeding on MacOS/BSD
      [CI] Disable unsupported qemu feature by bypassing the configuration
      Merge #17469: test: Remove fragile assert_memory_usage_stable
      Descriptor expansions only need pubkey entries for PKH/WPKH
      Merge #14522: tests: add invalid P2P message tests
      [land-bot] Improve error reporting when fetching revision status fails
      Add comments to descriptor tests
      Add descriptor expansion cache
      [refactor] Combine the ToString and ToPrivateString implementations
      [refactor] Use DescriptorImpl internally, permitting access to new methods
      decremented EXPECTED_VIOLATION_COUNT
      [refactor] Add a base DescriptorImpl with most common logic
      Add release notes for importmulti descriptor support
      Add test for importing via descriptor
      [wallet] Allow descriptor imports with importmulti
      Gather additional entropy from the environment
      [MOVEONLY] Move cpuid code from random & sha256 to compat/cpuid
      Seed randomness with process id / thread id / various clocks
      [tests] move wallet util functions to wallet_util.py
      [tests] tidy up wallet_importmulti.py
      [wallet] Refactor ProcessImport() to call ProcessImportLegacy()
      [wallet] Add ProcessImportLegacy()
      [MOVEONLY] Move perfmon data gathering to new randomenv module
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      rpc: creates possibility to preserve labels on importprivkey
      [backport#16349] Remove redundant WalletController::addWallet slot
      [backport#16348] qt: Assert QMetaObject::invokeMethod result
      [backport#16348] gui: Fix missing qRegisterMetaType(WalletModel*)
      [backport#15091] Qt: Fix update headers-count
      [backport#15091] Qt: update header count regardless of update delay
      [backport#15462] gui: Fix async open wallet call order
      [backport#16106] refactor: Rename getWallets to getOpenWallets in WalletController
      [backport#16106] gui: Sort wallets in open wallet menu
      [backport#16231] gui: Fix open wallet menu initialization order
      [backport#16118] gui: Enable open wallet menu on setWalletController
      Replace message type literals with protocol.h constants
      [backport#15195] gui: Add close wallet action
      [backport#15195] gui: Add closeWallet to WalletController
      [backport#16995] refactor: Work around GCC 9 `-Wredundant-move` warning
      Recategorize seeder connections as not manual
      Removed activation logic for OP_REVERSEBYTES to pretend it was always enabled
      [backport#16728] move-only: move coins statistics utils out of RPC
      [backport#16995] net: Fail instead of truncate command name in CMessageHeader
      [backport#15195] interfaces: Add remove to Wallet
      [backport#15957] Show loaded wallets as disabled in open menu instead of nothing
      [backport#15308] Piecewise construct to avoid invalid construction
      [backport#14820] test: Fix descriptor_tests not checking ToString output of public descriptors
      doc: minor corrections in random.cpp
      random: remove call to RAND_screen() (Windows only)
      gui: remove OpenSSL PRNG seeding (Windows, Qt only)
      doc: correct function name in ReportHardwareRand()
      net: Use mockable time for tx download
      scripted-diff: use self.sync_* methods
      test: Add BitcoinTestFramework::sync_* methods
      test: Pass at most one node group to sync_all
      scripted-diff: Rename sync_blocks to send_blocks
      qa: Run more tests with wallet disabled
      Reconsider checkpointed block at startup.
      [wallet] Refactor ProcessImport()
      [backport#15153] gui: Show indeterminate progress dialog while opening wallet
      Overhaul importmulti logic
      Release notes for PR14477
      [backport#15153] gui: Add OpenWalletActivity
      [backport#15153] Interfaces: Avoid interface instance if wallet is null
      [backport#15153] gui: Add thread to run background activity in WalletController
      [backport#15153] gui: Add Open Wallet menu
      Add RNG strengthening (10ms once every minute)
      Add release notes
      Add matching descriptors to scantxoutset output + tests
      Add descriptors to listunspent and getaddressinfo + tests
      [backport#16033] Hold cs_main when reading chainActive via getTipLocator(). Remove assumeLocked()
      Switch memory_cleanse implementation to BoringSSL's
      [testonly] [wallet] use P2WPKH change output if any destination is P2WPKH or P2WSH
      Add address_types test
      Add tests for InferDescriptor and Descriptor::IsSolvable
      Add support for inferring descriptors from scripts
      Add Descriptor::IsSolvable() to distinguish addr/raw from others
      Fix dbcrash spurious failures
      Fix Flake8 E741 errors
      Lint everything
      [backport#15153] gui: Add openWallet and getWalletsAvailableToOpen to WalletController
      Cleanup useless dependency in setup-debian-buster.sh
      Check for IBD completion based on existing log message instead of 100% progress
      Remove legacy per transaction sigops accounting
      Remove legacy per block sigops accounting
      [fix] unbreak D6110
##### [backport#15153] interfaces: Add loadWallet to Node
      Remove legacy sigops support from miner
      Kill GetSigOpCount
      Do not count sigops at all anymore
      MOVEONLY: Move NodeContext struct to node/context.h
      scripted-diff: Rename InitInterfaces to NodeContext
      [build] enforce exhaustive switch statements in BUILD_WERROR config
      [Automated] Update timing.json
##### [backport#15153] wallet: Factor out LoadWallet
      [backport#15638] [build] Move AnalyzePSBT from psbt.cpp to node/psbt.cpp
      [backport#15508] Add documentation of struct PSBTAnalysis et al
      [backport#15508] Refactor analyzepsbt for use outside RPC code
      [backport#15508] Move PSBT decoding functions from core_io to psbt.cpp
      [backport#14906] refactor: Make explicit CMutableTransaction -> CTransaction conversion.
      [backport#13769] Mark single-argument constructors "explicit"
      [backport#15404] Address test todos by removing -txindex to nodes.
      [backport#15247] qa: Use wallet to retrieve raw transactions
      [backport#15159] [RPC] Update getrawtransaction interface
      [backport#13932] Implement analyzepsbt RPC and tests
      Set LEEWAY to 0 in check_script_prefixes
      Rename test to follow the naming convention
      Bump version to 0.21.8
      Allow abc_ as a prefix for test naming convention
      Fix typo related to ZMQ in build docs
      Add OP_REVERSEBYTES test case
      Report updated sigops count in mining RPC
      Add checkpoints for phonon activation
      Add release notes
      [Automated] Update manpages
      [tests] add test_address method to wallet_import.py
      [tests] add test_importmulti method to wallet_import.py
      [tests] add get_multisig function to wallet_importmulti.py
      [tests] add get_key function to wallet_importmulti.py
      Partial Merge #14454: ProcessImport() cleanup (excluding witness)
      Update chainparams to a post-upgrade block
      Fix incorrect mocktime set in miner fund test
      Remove 0.20.x nodes from makeseeds
      [tests] tidy up imports in wallet_importmulti.py
      Remove white space between list in v0.21.5 release notes
      Merge #9332: Let wallet importmulti RPC accept labels for standard scriptPubKeys
      Bump PORT_MIN in test framework to not collide with testnet
      Merge #16918: test: Make PORT_MIN in test runner configurable
      Remove Core release note file, update previous release notes, and fix generate() deprecation message
      Merge #18641: test: Create cached blocks not in the future
      [Automated] Update timing.json
      [Automated] Update seeds
      Reduce memory allocations in getblocktemplate
      Fix string layout in rpc/rawtransaction.cpp
      [CMAKE] Improve the FindJemalloc module
      Bump misbehaving factor for unexpected version message behavior
      Update univalue to 1.1.1
      [tests] Give a useful error message when assert_debug_log is called with empty expected messages
      [doc] since D5764, regtest requires standard txns by default
      [DEPENDS] Remove the facilities for building win32
      [backport#13932] Figure out what is missing during signing
      [DEPENDS] Add jemalloc to the depends
      [backport#13932] Move PSBT UTXO fetching to a separate method
      [backport#13932] Implement joinpsbts RPC and tests
      [backport#13932] Implement utxoupdatepsbt RPC and tests
      refactor: Cleanup walletinitinterface.h
      scripted-diff: Make translation bilingual
      Add bilingual message type
      Refactor out translation.h
      Remove leftover from debuging
      [land-bot] Ensure changes to the land bot script do not modify its execution in-flight
      [land-bot] Only operate on trusted patches
      Merge #14150: Add key origin support to descriptors
      Use HTTPS for LLVM repository
      Add missing cs_main lock
      [net] Ignore unlikely timestamps in version messages
      Fix potential timedata overflow
      refactoring: IsInitialBlockDownload -> CChainState
      [tests] Remove ctime() call which may be unreliable on some systems
      [CMAKE] Use jemalloc as an allocator
      Bump version to 0.21.7
      [backport#15632] [wallet] Remove unnecessary Chain::Lock parameter from ResendWalletTransactions
      [backport#15632] [wallet] Schedule tx rebroadcasts in wallet
      Restrict setmocktime to non-negative integers
      [backport#14690] Throw error if CPubKey is invalid during PSBT keypath serialization
      [backport#14689] Require a public key to be retrieved when signing a P2PKH input
      [backport#14424] Stop requiring imported pubkey to sign non-PKH schemes
      [backport#15408] Remove unused TransactionError constants
      [backport#14356] fix converttopsbt permitsigdata arg, add basic test
      [backport#15638] [docs] Document src subdirectories and different libraries
      [backport#15638] [build] Move wallet load functions to wallet/load unit
      Properly handle LONG_MIN in timedata.cpp
      Remove last vestige of the alert system
      Merge #12764: doc: Remove field in getblocktemplate help that has never been used.
      [Automated] Update manpages
      Merge #14984: rpc: Speedup getrawmempool when verbose=true
      Merge #15463: rpc: Speedup getaddressesbylabel
      refactoring: FlushStateToDisk -> CChainState
      Added some release notes
      refactoring: introduce ChainstateActive()
      [backport#15638] [build] Add several util units
      move-only: make the CChainState interface public
      UniValue performance speedups for .write()
      Move DisconnectResult in its own header and make it an enum class
      test: Bump MAX_NODES to 12
      Remove dead code in core_memusage.h
      More include fixes
      Add missing includes
      [backport#15638][build] Move several units into common libraries
      correct forward declaration in rawtransaction_util.h
      prevector: avoid misaligned member accesses
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      qa: Always refresh cache to be out of ibd
      [backport#15638] [build] Move rpc rawtransaction util functions to rpc/rawtransaction_util.cpp
      [backport#15638] [build] Move rpc utility methods to rpc/util
      Merge #16267: bench: Benchmark blockToJSON
      Merge #16299: bench: Move generated data to a dedicated translation unit
      [backport#15680] [wallet] Remove ResendWalletTransactionsBefore
      [tests] Move deterministic address import to setup_nodes
      [rpc] add 'getnewaddress' hint to 'generatetoaddress' help text.
      [wallet] Deprecate the generate RPC method
      [tests] Add generate method to TestNode
      [tests] Small fixups before deprecating generate
      Delete globals.h and globals.cpp
      [trivial] turn test runner cli output into Bitcoin Cash
      [backport#15638] [build] Move policy settings to new src/policy/settings unit
      tests: write the notification to different files to avoid race condition
      Pure python EC
      tests: Make it possible to run functional tests on Windows
      Make sure we read the command line inputs using utf-8 decoding in python
      [lint-circular-dependencies] changed expected dep list to establish baseline
      [backport#13695] lint: Add linter for circular dependencies
      [backport#15638] [build] Move CheckTransaction from lib_server to lib_consensus
      qa: Make extended tests pass on native Windows
      qa: Fix some tests to work on native windows
      qa: Add emojis to test_runner path and wallet filename
      [backport#15680] [rpc] remove resendwallettransactions RPC
      wallet: Fixup rescanblockchain result doc
      [trivial] comment correction on wallet_balance.py
      Suggested wallet code cleanups from #14711
      [backport#15632] [wallet] Keep track of the best block time in the wallet
      [backport#14845] [tests] Add wallet_balance.py
      [backport#15646] [tests] Add test for wallet rebroadcasts
      Finish PR14987 and clean up some functions
      Pass some of wallet/rpcwallet RPC results and examples to RPCHelpMan
      [land-bot] Fix a bug where the unencrypted CONDUIT_TOKEN could be logged by subshells
      Clear mock time between tests
      [rpc] mining: Omit uninitialized currentblockweight, currentblocktx
      Update confusing names in rpc_blockchain.py
      [backport#15288] moved remaining g_mempool references out of wallet.cpp
      test: Adapt test framework for chains other than "regtest"
      [backport#10973] Remove remaining wallet accesses to node globals
      [Fix] The default whitelistrelay should be true
      test: Remove incorrect and unused try-block in assert_debug_log
      on startup, write config options to debug.log
      QA: fix rpc_setban.py race
      [Fix] Allow connection of a noban banned peer
      Add functional tests for flexible whitebind/list
      Replace the use of fWhitelisted by permission checks
      Do not disconnect peer for asking mempool if it has NO_BAN permission
      Make whitebind/whitelist permissions more flexible
      Correction of unaddressed nit in previous revision
      [backport#10973] Remove use of CCoinsViewMemPool::GetCoin in wallet code
      Finish passing rpcwallet RPCs Results and Examples to RPCHelpMan
      Pass some more (3/4) rpcwallet RPCs Results and Examples to RPCHelpMan
      Remove config managed RPC user/pass
      Pass more (2/4) rpcwallet RPCs Results and Examples to RPCHelpMan
      [backport#10973] Remove use of CRPCTable::appendCommand in wallet code
      Pass pruneblockchain RPC Results and Examples to RPCHelpMan
      Pass rpc/avalanche Results and Examples to RPCHelpMan
      Pass rpc/misc RPC Results and Exmaples to RPCHelpMan
      Revert ContextFreeRPCCommand nonsense
      Fix error messages in noui.cpp
      [CI] Add a sepc256k1 specific build plan
      refactor: Settings code cleanups
      [CI] Run the tests for ARM
      [CI] Run the tests for AArch64
      test: Add ASSERT_DEBUG_LOG to unit test framework
      refactor: Remove redundant c_str() calls in formatting
      Add settings_tests
      Deduplicate settings merge code
      Remove includeconf nested scope
      Preparations for more testchains
      [CMAKE] Fix missing linker wrap for fcntl64
      build: remove linking librt for backwards compatibility
      [CI] Build the OSX DMG
      [CI] Build the windows installer
      Run tool_wallet.py with an emulator as needed
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Finish Passing rpc/blockchain RPC results and examples to RPCHelpMan
      Link univalue in the seeder
      gui: Drop boost::scoped_array and use wchar_t API explicitly on Windows
      gui: Fix for Incorrect application name when passing -regtest
      Reorder univalue include
      Pass some rpc/blockchain RPC results and examples to RPCHelpMan
      Finish Passing rpc/rawtransaction RPC results and examples to RPCHelpMan
      Add util::Settings struct and helper functions.
      Rename includeconf variables for clarity
      Clarify emptyIncludeConf logic
      [backport#14532] net: Always default rpcbind to localhost, never "all interfaces"
      Fix the emulator with autotools
      [CMAKE] Increase the unit test verbosity to test_suite
      Make bitcoin-util use the emulator as needed
      Allow for using an emulator for the functional test framework
      [CMAKE] Use the crosscompiling emulator to run the tests
      [CMAKE] Propagate the LFS support flags to the libraries
      Pass wallet/rpcdump RPC results and examples to RPCHelpMan
      Add a land bot script for running smoke tests before landing patches
      [backport#10973] Remove use CValidationInterface in wallet code
      [backport#15531] Merge #15531: Suggested interfaces::Chain cleanups from #15288
      [backport#15288] Remove use of IsInitialBlockDownload in wallet code
##### [backport#15288] Remove use of uiInterface.LoadWallet in wallet code
      [backport#15288] circular-dependencies: Avoid treating some .h/.cpp files as a unit
      [backport#15288] Remove use of AcceptToMemoryPool in wallet code
      Bump tool_wallet timeout
      [CMAKE] Fix the build with ZMQ disabled
      Add test for ArgsManager::GetChainName
      Fix -Wdeprecated-copy warning with GCC >= 9
      [backport#15288] Remove uses of InitMessage/Warning/Error in wallet code
      Reorder various univalue include orders
      Make sure we're using the same version of clang tools for everything
      Merge #15947: Install bitcoin-wallet manpage
      Merge #15354: doc: Add missing bitcoin-wallet tool manpages
      [CI] Cross build and run tests for Linux 64 bits
      Pass rpc/rawtransaction RPC results and examples to RPCHelpMan
      [CI] Cross build for Linux ARM and AArch64
      [CI] Cross build for OSX
      Pass rpc/mining RPC results and examples to RPCHelpMan
      Pass rpc/net RPC Results and Examples to RPCHelpMan
      [backport#15288] Remove uses of GetAdjustedTime in wallet code
      [backport#15288] Remove use of g_connman / PushInventory in wallet code
      Remove unnecessary --force-yes from installation script
      [backport#15288] Remove uses of g_connman in wallet code
      Backport leftovers from 15788
      bench: Add block assemble benchmark
      util: make ScheduleBatchPriority advisory only
      Add test for AddTimeData
      Add settings merge test to prevent regresssions
      Use BlockHash for CheckProofOfWork
      Reduce the use of ClearArg and only guarantee that we clear forced args.
      [backport#15288] Remove uses of fPruneMode in wallet code
      [backport#15288] Remove use of CalculateMemPoolAncestors in wallet code
      Use std::thread::hardware_concurrency, instead of Boost, to determine available cores
      refactor: consolidate PASTE macros
      Fix declaration order in util/system.h
      [backport#15288] Remove use of GetTransactionAncestry in wallet code
      scripted-diff: Replace CCriticalSection with RecursiveMutex
      [CI] Build for windows 64 and run some unit tests
      Merge #15069: test: Fix rpc_net.py "pong" race condition
      Merge #15013: test: Avoid race in p2p_timeouts
      Rename IsGood() to IsReliable()
      Merge #14733: P2P: Make peer timeout configurable, speed up very slow test and ensure correct code path tested.
      Add a deprecation notice for the autotools build system
      Bump version to 0.21.6
      Merge #14456: test: forward timeouts properly in send_blocks_and_test
      [backport#15288] Remove use of GetCountWithDescendants in wallet code
      [backport#15288] Remove use of IsRBFOptIn in wallet code
      Pass zmq RPC results and examples to RPCHelpMan
      [backport#15288] Remove uses of CheckFinalTx in wallet code
      [Automated] Update manpages
      Add RPC Whitelist Feature from #12248
      Pass rpc/server RPC results and examples to RPCHelpMan
      Finish up release notes
      Pass rpc/abc RPC Results and Examples to RPCHelpMan
      Add default THREADS to build_autotools.sh
      Make sure build_* devtools scripts' default build directories always exist
      Build the deb package with cmake
      [backport#15864] Fix datadir handling
      Update the PPA to support Ubuntu 20.04, drop 16.04
      [BENCH] Add an option to output the result as a Junit report
      Remove CheckFinalTx
      [CMAKE] Add a translate target
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Only log "Using PATH_TO_bitcoin.conf" message on startup if conf file exists.
      [backport#16366] init: Use InitError for all errors in bitcoind/qt
      Remove TestNode()
      [build-configurations] Add build-without-cli
      Merge #17497: test: skip tests when utils haven't been compiled
      refactor: test/bench: dedup Build{Crediting,Spending}Transaction()
      [backport#14783] qt: Call noui_connect to prevent boost::signals2::no_slots_error in early calls to InitWarning
      Merge #14298: [REST] improve performance for JSON calls
      [backport#12916] Introduce BigEndian wrapper and use it for netaddress ports
      Docs: Modify policy to not translate command-line help
      Merge #14097: validation: Log FormatStateMessage on ConnectBlock error in ConnectTip
      gui: Stop translating PACKAGE_NAME
      [autopatch] Do not create a new git branch when fetching upstream
      core -> ABC in extract_strings_qt.py
      Fix @generated marking in Phab for generate-seeds.py
      Stop translating command line options (continuated)
      gui: Fix window title update
      Merge #17068: qt: Always generate `bitcoinstrings.cpp` on `make translate`
      Fix a bug where running test_runner.py --usecli would fail when built without bitcoin-cli
      Merge #14381: test: Add missing call to skip_if_no_cli()
      Refactor: Replace fprintf with tfm::format
      Merge #14885: rpc: Assert named arguments are unique in RPCHelpMan
      Merge #17192: util: Add CHECK_NONFATAL and use it in src/rpc
      [backport#15891] test: Require standard txs in regtest by default
      Merge #13105: [qa] Add --failfast option to functional test runner
      init: Remove deprecated args from hidden args
      test: Make tests arg type specific
      Revamp option negating policy
      Replace IsArgKnown() with FlagsOfKnownArg()
      Use ArgsManager::NETWORK_ONLY flag
      Remove unused m_debug_only member from Arg struct
      scripted-diff: Use ArgsManager::DEBUG_ONLY flag
      scripted-diff: Use Flags enum in AddArg()
      util: Explain why the path is cached
      Enable PID file creation on WIN
      Improve PID file error handling
      Speed up OP_REVERSEBYTES test significantly
      Catch exception by ref in wallettool.cpp
      [cmake] Fix dependencies for functional test targets
      [CMAKE] Use a cmake template for config.ini
      [backport#15629] init: Throw error when network specific config is ignored
      Add Flags enum to ArgsManager
      Refactor InterpretNegatedOption() function
      refactoring: Check IsArgKnown() early
      implements different disk sizes for different networks on intro
      [tools] Add wallet inspection and modification tool
      [tests] Functional test naming convention
      Update univalue to 1.0.5
      [backport#15335] Fix lack of warning of unrecognized section names
      [backport#15087] Error if rpcpassword contains hash in conf sections
      [backport#14708] Warn unrecognised sections in the config file
      Backport Core PR12246
      [DOC] Update the sanitizer documentation
      [DOC] Update developer notes
      [CMAKE] Add support for generating test coverage reports
      [backport#14618] rpc: Make HTTP RPC debug logging more informative
      [backport#14628] Rename misleading 'defaultPort' to 'http_port'
      Merge #15943: tests: Fail if RPC has been added without tests
      Factor out combine / finalize / extract PSBT helpers
      Switch away from exceptions in refactored tx code
      Remove op== on PSBTs; check compatibility in Merge
      Split DecodePSBT into Base64 and Raw versions
      Add pf_invalid arg to std::string DecodeBase{32,64}
      Simplify Base32 and Base64 conversions
      Allow ConvertBits() to succeed on unpadded zeros
      qt: Set AA_EnableHighDpiScaling attribute early
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Complete PR14796 by cleaning up some old functions and names
      Finish passing the remainder of wallet/rpcwallet RPC argument descriptions to RPCHelpMan
      Start passing some wallet/rpcwallet RPC argument descriptions to RPCHelpMan
      Pass rpc/mining RPC argument descriptions to RPCHelpMan
      Pass wallet/rpcdump RPC argument descriptions to RPCHelpMan
      Pass rpc/rawtransaction RPC argument descriptions to RPCHelpMan
      Pass rpc/misc RPC argument descriptions to RPCHelpMan
      [avalanche] Process AvalancheResponse
      Move PSBT definitions and code to separate files
      Refactor PSBT signing logic to enforce invariant
      Factor BroadcastTransaction out of sendrawtransaction
      Merge #17121: test: speedup wallet_backup by whitelisting peers (immediate tx relay)
      [release-process] Update Ubuntu PPA instruction
      Pass rpc/blockchain RPC argument descriptions to RPCHelpMan
      Pass rpc/net RPC argument descriptions to RPCHelpMan
      Pass rpc/server RPC argument descriptions to RPCHelpMan
      Extract the event loop management from the avalanche code so it can be reused.
      Pass rpc/avalanche RPC argument descriptions to RPCHelpMan
      Make use of ADDR_SOFT_CAP outside just the seeder test suite
      Extract smoke tests from automated commits
      Merge #13891: [RPC] Remove getinfo deprecation warning
      Add upgraded nodes as seeds
      [avalanche] Modernize the code via using instead of typedef
      Remove win32 from Github release
      [backport] Scripts and tools: Fix BIND_NOW check in security-check.py
      [avalanche] Buffer avapoll and avaresponse
      [backport] Trivial: fix references to share/rpcuser (now share/rpcauth)
      Pass abc RPC argument descriptions to RPCHelpMan
      Use full sanitizer options on CI
      Fix race condition in avalanche test
      nits: use const for loop iterrators in avalanche code
      Remove 'boost::optional'-related gcc warnings
      Allow to extend and override the sanitizers options
      [CMAKE] Set environment variables when running tests with sanitizers
      Bump copyright year to 2020
      Bump version to 0.21.5
      [backport] test: Fix AreInputsStandard test to reference the proper scriptPubKey
      Merge #15201: net: Add missing locking annotation for vNodes. vNodes is guarded by cs_vNodes.
      [Automated] Update manpages
      Fix the avalanche tests
      [Avalanche] Gather INVs before entering critical section for cs_vNodes
      Remove uses of chainActive and mapBlockIndex in wallet code
      tests: Fix fs_tests for unknown locales
      Use a different address in Avalanche test to prevent duplicate blocks
      [backport] RPCAuth Detection in Logs
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Clean cache and tmp directory for instegration tests
      Fix Avalanche functional test when wallet is disabled
      [avalanche] Require that a pubkey is associated with each avalanche peer.
      Use CPubKey::PUBLIC_KEY_SIZE & al when apropriate.
      depends: only use D-Bus with Qt on linux
      [CMAKE] Enable DBus on Linux only
      [avalanche] Poll tip candidate rather than eagerly
      [avalanche] Start polling when we park of block because of reorg
      bump libevent to 2.1.11 in depends
      Pass CChainParams down to DisconnectTip
      Pass CChainParams down to UpdateTip
      depends: fix boost mac cross build with clang 9+
      Reword confusing warning message in RPC linter
      depends: Consistent use of package variable
      depends: don't configure xcb_proto
      build: pass -dead_strip_dylibs to ld on macOS
      build: don't embed a build-id when building libdmg-hfsplus
      depends: add ability to skip building qrencode
      Fix invalid use a memory order relaxed
      Only pass --disable-dependency-tracking to packages that understand it
      depends: qt: Fix C{,XX} pickup
      depends: qt: Fix {C{,XX},LD}FLAGS pickup
      depends: zlib: Move toolchain options to configure
      depends macOS: point --sysroot to SDK
      build: switch to upstream libdmg-hfsplus
      depends: latest config.guess and config.sub
      build: Add variable printing target to Makefiles
      Add OpenSSL termios fix for musl libc
      build: remove redundant sed patching
      [LINTER] Remove trailing whitespaces
      Add setexcessiveblock to vRPCConvertParams
      Remove trailing whitespaces in old release notes
      Remove trailing whitespaces in various files
      Remove trailing whitespaces in cmake files
      Remove trailing whitespaces in markdown files
      Also track dependencies of native targets
      depends: Bump QT to LTS release 5.9.8
      depends: cleanup package configure flags
      build: make building protobuf optional in depends
      build: remove qt libjpeg check from bitcoin_qt.m4
      depends: disable unused Qt features
      Added some factors that affect the dependency list
      depends: Prune X packages
      packages.md: document depends build targets
      build: disable libxcb extensions
      .gitignore: Don't ignore depends patches
      depends: expat 2.2.7
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      Revert "Disable thread_local for i686-mingw"
      Remove Windows 32 bit build
      Use ninja to generate dep files for the native build
      Rebuild native executable when changes to the build system are made
      build: prune dbus from depends
      depends: xtrans: Configure flags cleanup.
      [depends] boost: update to 1.70
      depends: Purge libtool archives
      depends: add ability to skip building zeromq
      Merge #15485: add rpc_misc.py, mv test getmemoryinfo, add test mallocinfo
      Add version number to the seeder
      depends: Make less assumptions about build env
      contrib: Fix test-security-check fail in Ubuntu 18.04
      tests: Add test for 64-bit PE, modify 32-bit test results
      Improve depends debuggability
      Update zmq to 4.3.1
      depends: expat 2.2.6 and qt 5.9.7
      depends: native_protobuf: avoid system zlib
      depends: Enable unicode support on dbd for Windows
      [depends, zmq, doc] upgrade zeromq to 4.2.5 and avoid deprecated zeromq
      build: Remove illegal spacing in darwin.mk
      depends: Add 'make clean' and 'make clean-all' rules
      [tests] Remove rpc_zmq.py
      [CMAKE] Replace the test runner with a test wrapper
      Clean the native directory when using the clean target
      depends: zeromq 4.2.3
      [SECP256K1] Travis Don't run the constant time check with java tests
      [secp256k1] Extend libsecp256k1's ctime test to check schnorr signatures
      Constant-time behaviour test using valgrind memtest.
      Bump version to 0.21.4
      [depends] ZeroMQ 4.2.2
      [CMAKE] Pull zmq windows library dependencies in the find module
      [CMAKE] Pull miniupnpc windows library dependencies in the find module
      [CMAKE] Pass the interface linked libraries to find_component
      Disable thread_local for i686-mingw
      [SECP256K1] Turn off ASM by default on target with no ASM support
      [secp256k1] Install all packages in travis
      [SECP256K1] Fix Travis missing ninja
      [Automated] Update manpages
      [test_runner] Fix result collector variable shadowing
      [test_runner] Use the daemon property directly instead of setDaemon()
      Add missing softforks help text to getblockchaininfo
      Merge #14813: qa: Add wallet_encryption error tests
      Fixup release notes formatting
      Fix usehd release number errors in release notes
      Remove rules argument from getblocktemplate help
      Eliminate harmless non-constant time operations on secret data.
      [depends] Don't build libevent sample code
      [avalanche] sign avaresponse
      wallet: Initialize stop_block to nullptr in ScanForWalletTransactions
      Add stop_block out arg to ScanForWalletTransactions
      Return a status enum from ScanForWalletTransactions
      Make CWallet::ScanForWalletTransactions args and return value const
      wallet: Avoid leaking nLockTime fingerprint when anti-fee-sniping
      build: macOS toolchain update
      tests: remove unused includes in tests
      qt: test: Create at most one testing setup
      test: Log to debug.log in all tests
      test: Add test for unknown args
      Ignore unknown config file options, warn instead of error
      util: Log early messages
      [Automated] Update timing.json
      [Automated] Update chainparams
      [Automated] Update seeds
      [avalanche] Fix test when ran without wallet
      Merge #13061: Make tests pass after 2020
      Partial Merge #14726: Use RPCHelpMan for all RPCs
      Fix setexcessiveblock rpc help text
      [avalanche] Add handling of ava_poll command in the network layer
      [debian release] Fetch signer string from GPG rather than requiring the user to enter a perfectly formatted one
##### Merge #14561: Remove fs::relative call and fix listwalletdir tests
      Fixup paths in wallet_multiwallet
      Don't rename main thread at process level
      util: Make thread names shorter
      Fix portability issue with pthreads
      Thread names in logs and deadlock debug tools
      utils: Add fstream wrapper to allow to pass unicode filename on Windows
      utils: Convert fs error messages from multibyte to utf-8
      [SECP256K1] Disable ASM for native executables
      [schnorr] Add verification routine to the test framework schnorr signature facility
      Add a script for setting up Debian build containers
      Add another UBSAN vptr suppression
      Nits in rwcollection.h
      Update autotools for new seeder tests
      Fix the CI gitian build script when the OSX SDK is should be downloaded
      Fix the OSX gitian build
      Add some unit tests for write_name() for seeder
##### Merge #14291: wallet: Add ListWalletDir utility function
      Some left overs from PR12490
      Merge #14208: [build] Actually remove ENABLE_WALLET
      build: set minimum supported macOS to 10.12
      Remove unused misc.h
      [CMAKE] Fix the bench build for windows
      [Automated] Update timing.json
      Fix missing rename in fuzz test suite
      Merge #14373: Consistency fixes for RPC descriptions
      Merge #14718: Remove unreferenced boost headers
      [cmake] Fix typo in error message
      [CI] Do not re-download the OSX SDK if it's already available
      Fix remaining test_bitcoin.cpp references (should be setup_common.cpp)
      Make the fuzzer test runner compatible with cmake
      Bump copyright headers in tests (part3)
      Rename test_bitcoin to test/setup_common
      test: Use test_bitcoin setup in bench
      fuzz: Link BasicTestingSetup (shared with unit tests)
      fuzz: Script validation flags
      fuzz: Move deserialize tests to test/fuzz/deserialize.cpp
      qa: Add test/fuzz/test_runner.py
      depends: switch to secure download of all dependencies
      Delete README_osx.md and move its contents into build-osx.md
      [depends] expat 2.2.5, miniupnpc 2.0.20180203
      depends: Remove ccache
      [depends] expat 2.2.1
      [SECP256K1] CMake: Build the ARM ASM field implementation
      [CMAKE] Improve the toolchain files
      [Automated] Update seeds
      [Automated] Update chainparams
      Release notes for D5507
      Merge #14411: [wallet] Restore ability to list incoming transactions by label
      Bump copyright headers in tests (part2)
      Bump copyright headers in tests (part1)
      Bump copyright headers in bench
      Merge #14244: amount: Move CAmount CENT to unit test header
      Merge #14282: [wallet] Remove -usehd
      Merge #14215: [qa] Use correct python index slices in example test
      Merge #14207: doc: `-help-debug` implies `-help`
      Merge #14013: [doc] Add new regtest ports in man following #10825 ports reattributions
      test: Remove useless test_bitcoin_main.cpp
      Bump version to 0.21.3
      Add enum for parse_name() return value
      Use a sane default version for PPA releases
      Add constants to dns.h
      Don't use gold for the Gitian builds
      Add Ubuntu PPA to release process
      [Automated] Update manpages
      Add << operator overload for PeerMessagingState
      Update seeders list
      Add some release notes
      Update seeds
      Added a script for building and deploying Debian packages to launchpad.net
      Adds PeerMessagingState enum to seeder/bitcoin.*
      Various nits in arith_uint256.h
      Update dependencies in debian/control
      Bump debian package compat level to 9
      Update package maintainers in debian/control
      Use const in COutPoint class
      [CMAKE] Use gold as a linker when available
      [Automated] Update chainparams
      Simplify max query name length check in parse_name()
      Update package name in debian/control to bitcoinabc
      [CMAKE] Improve FindZeroMQ
      [CMAKE] Improve FindSHLWAPI
      [CMAKE] Make the FindRapicheck module consistent with the other modules
      [CMAKE] Improve FindQREncode
      [CMAKE] Improve FindMiniUPnPc
      [CMAKE] Improve FindGMP
      Nits to streams.h
      Rename DecodeDumpTime to ParseISO8601DateTime and move to time.cpp
      Misuse of the Visual Studio version preprocessor macro
      [CMAKE] Silent git error output when running from cmake
      [CMAKE] Fix FindBerkeleyDB suffix paths
      util: Add type safe GetTime
      [CMAKE] Improve FindEvent
      [CMAKE] Improve FindBerkeleyDB
      Move PackageOptions out of cmake/modules
      [cmake] check-symbols => symbol-check
      [wallet] Remove CAccount and Update zapwallettxes comment
      Don't enable the secp256k1 multiset module when building bitcoin abc
      [wallet] Remove strFromAccount and strSentAccount
      [wallet] Remove fromAccount argument from CommitTransaction()
      [wallet] Delete unused account functions
      [wallet] Remove CAccountingEntry class
      [cmake] check-security => security-check
      [wallet] Remove ListAccountCreditDebit()
      [SECP256K1] Fix issue where travis does not show the logs
      [SECP256K1] Add valgrind check to travis
      [SECP256K1] Request --enable-experimental for the multiset module
      [SECP256K1] Enable the OpenSSL tests (and benchmark)
      [SECP256K1] Fix a valgrind issue in multisets
      [SECP256K1] Fix the kitware PPA timeouts issues on Travis
      [DOC] Update the depends README with dependencies and cmake instructions
      [wallet] Don't read acentry key-values from wallet on load.
      [wallet] Don't rewrite accounting entries when reordering wallet transactions and remove WriteAccountingEntry()
      [wallet] Remove AddAccountingEntry()
      [wallet] Remove CWallet::ListAccountCreditDebit() and GetAccountCreditDebit()
      [wallet] Remove AccountMove()
      [wallet] Remove 'account' argument from GetLegacyBalance()
      Merge #13265: wallet: Exit SyncMetaData if there are no transactions to sync
      Merge #11269: [Mempool] CTxMemPoolEntry::UpdateAncestorState: modifySiagOps param type
      Merge #14023: Remove accounts rpcs
      Merge #13264: [qt] Satoshi unit
      [Automated] Update chainparams
      [DOC] Update the subtree section from the developer notes
      Move code in seeder_test.cpp close to where it is used
      [backport] trivial: Mark overrides as such. #13282
      [SECP256K1] Fix Travis failures due to APT addon
      [SECP256K1] Fix travis failure on ECMULT_GEN_PRECISION
      [avalanche] Start event loop at node startup
      Rename seeder_tests to p2p_messaging_tests
      Make parse_name() fail when passed buffer size = 0
      Bump version to 0.21.2
      Fix UAHF references in dnsseed-policy.md
      Spoof DISPLAY on headless build servers when generating manpages for bitcoin-qt
      [CI] Run the cmake build with make as a generator
      [DOC] Various updates to cmake/ninja
      Enforce maximum name length for parse_name() and add unit tests
      Label length unit tests for parse_name()
      Add simple unit tests for parse_name()
      scripted-diff: replace chainActive -> ::ChainActive()
      refactoring: introduce unused ChainActive()
      [CMAKE] Fix the check-bitcoin-* targets when running with Xcode
      Remove secret-dependant non-constant time operation in ecmult_const.
      Preventing compiler optimizations in benchmarks without a memory fence
      README: add a section for test coverage
      Overhaul README.md
      Convert bench.h to fixed-point math
      Add SECURITY.md
      Clarify that a secp256k1_ecdh_hash_function must return 0 or 1
      doc: document the length requirements of output parameter.
      variable signing precompute table
      Docstrings
      [CMAKE] Fix the build with Xcode as a generator
      revert to deprecated protobuf ByteSize() due to compatibility
      Stop using the deprecated google::protobuf::MessageLite::ByteSize()
      [backport] qt: Remove obsolete QModelIndex::child() #16707
      [backport] qt: Replace obsolete functions of QSslSocket #16708
      [validation.cpp] update 'cousins' during UpdateFlags
      rename: CChainState.chainActive -> m_chain
      Fix CPack NSIS homepage
      Make CPack email available for all generators
      [backport] qt: Replace functions deprecated in Qt 5.13 #16701
      Fix make dist by finishing RPM cleanup
      [cmake] Add comment on libsecp256k1 benchamrks
      Don't park blocks when there is no actual reorg
      [automated-commits] Add update-timings
      Make gen-manpages.sh return non-zero if the script fails at any point
      Increase robustness against UB in secp256k1_scalar_cadd_bit
      Remove mention of ec_privkey_export because it doesn't exist
      Remove note about heap allocation in secp256k1_ecmult_odd_multiples_table_storage_var
      Make no-float policy explicit
      Fix ASM setting in travis
      Move lcov-filter.py to cmake/utils
      [CMAKE] Fix build with make as a generator
      Fix the benchmark build when wallet is disabled
      JNI: fix use sig array
      [cmake] Build bench by default
      [CMAKE] Make the test python scripts depend on targets and not on files
      Remove GotVersion()
      Avoid calling secp256k1_*_is_zero when secp256k1_*_set_b32 fails.
      Add a descriptive comment for secp256k1_ecmult_const.
      secp256k1/src/tests.c: Properly handle sscanf return value
      Fix typo
      Fix missing update to bitcoin-qt manpages
      Fix typo in secp256k1_preallocated.h
      Make ./configure string consistent
      [seeder] Various nits in the cmake build
      Fix a nit in the recovery tests
      typo in comment for secp256k1_ec_pubkey_tweak_mul ()
      scalar_impl.h: fix includes
      Moved a dereference so the null check will be before the dereferencing
      Fix typo in docs for _context_set_illegal_callback
      [Automated] Update manpages
      [backport] Qt: Replace remaining 0 with nullptr #15114
      Add unit tests for CSeederNode::ProcessMessage()
      Add update-manpages to automated-commits
      Update seeds
      build: update RapidCheck Makefile
      build: dont compile rapidcheck with -Wall
      depends: latest rapidcheck, use INSTALL_ALL_EXTRAS
      Integration of property based testing into Bitcoin ABC
      [Automated] Update chainparams
      Add update-seeds to automated-commits
      [automated-commits] Make sure BUILD_DIR exists and is exported
      Bump version to 0.21.1
      Make all automated commits run smoke tests prior to pushing
      [CI] Run leveldb tests independently of other builds and tests
      Move check-seeds from CI to general seed tools
      Don't use std::quick_exit() as it is poorly supported
      [secp256k1] Allow to use external default callbacks
      [secp256k1] Remove a warning in multiset test
      Mute self assign warning in uint256_tests.cpp
      
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
####  Merge #13030: [bugfix] [wallet] Fix zapwallettxes/multiwallet interaction.
####  Merge #10816: Properly forbid -salvagewallet and -zapwallettxes for multi wallet.
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
####   Merge #15203: Fix issue #9683 "gui, wallet: random abort (segmentation fault)"
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
#     rpc: Make unloadwallet wait for complete wallet unload
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
#     Remove direct node->wallet calls in init.cpp
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
#     Pass chain and client variables where needed
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
#     [wallet] Support creating a blank wallet
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
##   [wallet] Add wallet name to log messages
     [tests] Fix race in rpc_deprecated.py
     add SCRIPT_VERIFY_MINIMALDATA to mandatory flags
     Add braces to support/lockedpool.cpp
#    Abstract EraseBlockData out of RewindBlockIndex
     [CI] Print the sanitizer logs
     [CI] Move the sanitizer log directory to /tmp
     [CMAKE] Add an option to promote some warnings to errors
#    Remove Unused CTransaction tx in wallet.cpp
#    Privatize CWallet::AddToWalletIfInvolvingMe
#    Extract CWallet::MarkInputsDirty
     [CMAKE] Silent the Qt translation files generation
     allow cuckoocache to function as a map
#    Merge #12944: [wallet] ScanforWalletTransactions should mark input txns as dirty
     Log debug build status and warn when running benchmarks
     bench_bitcoin: Avoid read/write to default datadir
     test_bitcoin: Avoid read/write to default datadir
     Merge #13074: [trivial] Correct help text for `importaddress` RPC
#    Merge #13500: [wallet] Decouple wallet version from client version
     partial revert of tx decode sanity check backport
     fix incomplete txvalidationcache_tests
     Fix cuckoocache_tests -Wcast-align warnings
     [cuckoocache] Use getKey and KeyType for contains
##   Merge #13627: Free keystore.h from file scope level type aliases
##   Merge #13603: bitcoin-tx: Stricter check for valid integers
     [move only] Move BIP70 code together in preparation to backport PR14451 BIP70 changes
##   Sanity-check script sizes in bitcoin-tx
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
xx   refactoring: add a method for determining if a block is pruned or not
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
xx   refactor: Add and use HaveTxsDownloaded() where appropriate
     Added autopatch script for patching and rebasing phabricator diffs
xx   Make TxIndex::FindTx use BlockHash
     Update GetTransaction's parameters
xx   Update mempool and compact block logic to use TxHash
xx   tx pool: Use class methods to hide raw map iterator impl details
xx   Update mempool's mapDelta to use TxId
xx   Update mempool's mapTx to index from TxId.
xx   Use TxId in setInventoryTxToSend
     refactor: Drop boost::this_thread::interruption_point and boost::thread_interrupted in main thread
xx   Use BlockHash in BlockTransactionsRequest
     [cmake] link test runners by default
     Drop minor GetSerializeSize template
     Avoid creating a temporary vector for size-prefixed elements
#    Drop unused GetType() from CSizeComputer
     validation: assert that pindexPrev is non-null when required
     [CMAKE] Avoid rebuilding sec256k1
     [CMAKE] Fix scope issue in the remove_<lang>_compiler_flags() functions
#    Rationalize lock anotation in validation code
     tests: Add missing cs_main locks required when accessing pcoinsdbview, pcoinsTip or pblocktree
##   Introduce BlockHash to represent a block hash
     Add braces in block.h
     Use size_t where apropriate in skiplist_tests.cpp
##   Add Benchmark to test input de-duplication worst case
##   Add const modifier to HTTPRequest methods
     Add braces in txdb.cpp
     Move pindexFinalized in CChainState
#    Explain GetAncestor check for m_failed_blocks in AcceptBlockHeader
#    Remove unnecessary const_cast
     Fix activation_tests
     Add fuzz testing for BlockTransactions and BlockTransactionsRequest
     [test] Speed up fuzzing by ~200x when using afl-fuzz
     [CMAKE] Build test_bitcoin_fuzzy
     drop 'check3' upgrade-conditional-script-failure for Schnorr multisig
##   Nit in net_processing.cpp
###  Backport PR14897, PR15834 and PR16196
###  Merge #15149: gui: Show current wallet name in window title
     Update timings.json
     Various nits in net_processing.cpp
#    p2p: Clarify control flow in ProcessMessage()
#    Backport of Core PR14728: fix uninitialized read when stringifying an addrLocal
     previous link was dead
###  Merge #14784: qt: Use WalletModel* instead of the wallet name as map key
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
###  Merge #12639: Reduce cs_main lock and avoid extra lookups of mapAddressBook in listunspent RPC
###  [wallet] [rpc] Remove getlabeladdress RPC
     Fix wrong version in clang-format error message and update the doc
     Merge #13138: [tests] Remove accounts from wallet_importprunedfunds.py
###  Merge #13437: wallet: Erase wtxOrderd wtx pointer on removeprunedfunds
     Add test coverage for messages requesting invalid blocks
#### Drop IsLimited in favor of IsReachable
#    Remove undue lock assertion in GuessVerificationProgess
#### Revert use of size_t in ParseParameters
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
#### Merge #13055: qt: Don't log to console by default
     [cmake] Add comments to express what tests do.
     [cmake] Remove useless copy of create_cache.py
     [tests] Remove 'account' API from wallet_listreceivedby
#### Split out key-value parsing in ArgsManager into its own function
#### IsReachable is the inverse of IsLimited (DRY). Includes unit tests
     Bump version to 0.20.8
     [cmake] Use terminal when runnign integration tests
     Rename GetLogger() to LogInstance()
     Add missing parts from PR12954
     Use RdSeed when available, and reduce RdRand load
     Print to console by default when not run with -daemon
     Improve formatting in rpcwallet.cpp
#    Leftover from PR13423
#### Stop translating command line options
#### minor refactor to use ranged_for, auto and const-ness
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
xx    [MOVEONLY] Move ParseHDKeypath to utilstrencodings
      Introduce KeyOriginInfo for fingerprint + path
#     Merge #9662: Add createwallet "disableprivatekeys" option: a sane mode for watchonly-wallets
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
xx    RPC: Add new getzmqnotifications method.
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
##### Deprecate wallet 'account' API
##### [wallet] Deprecate account RPC methods
##### [wallet] [rpc] Remove duplicate entries in rpcwallet.cpp's CRPCCommand table
      [tests] Rename rpc_listtransactions.py to wallet_listtransactions.py
####  Merge #12892: [wallet] [rpc] introduce 'label' API for wallet
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
xx    Merge #13481: doc: Rewrite some validation docs as lock annotations
20.5      
      Updated manpages for 0.20.5 release
      Updated chainparams
      Update seeds
      bench: Benchmark MempoolToJSON
#     Merge #14444: Add compile time checking for cs_main locks which we assert at run time
xx    Merge #13114: wallet/keystore: Add Clang thread safety annotations for variables guarded by cs_KeyStore
      Add test_runner flag to suppress ASAN errors from wallet_multiwallet.py
      [CMAKE] Move package name and copyright to the top level
      Added build-werror config to error on build warnings
      Merge #13248: [gui] Make proxy icon from statusbar clickable
xx    Merge #13043: [qt] OptionsDialog: add prune setting
      Fix -Wrange-loop-analysis warnings
      Fix -Wthread-safety-analysis warnings
      [CMAKE] Use CPack to build source packages
      [CMAKE] Use CPack to build packages
#     mempool: remove unused magic number from consistency check
#     Merge #13258: uint256: Remove unnecessary crypto/common.h dependency
      Merge #11491: [gui] Add proxy icon in statusbar
XXXXX bugfix: Remove dangling wallet env instance and Delete walletView in WalletFrame::removeWallet
#     ui: Support wallets unloaded dynamically
#     rpc: Add unloadwallet RPC, release notes, and tests
#     rpc: Extract GetWalletNameFromJSONRPCRequest from GetWalletForJSONRPCRequest
#     [mempool] Mark mempool import fails that were found in mempool as 'already there'
      [CMAKE] Propagate requirements for cmake >= 3.12
xx    Merge #11050: Avoid treating null RPC arguments different from missing arguments
xx    Merge #11191: RPC: Improve help text and behavior of RPC-logging.
xx    Merge #11626: rpc: Make `logging` RPC public
xx    [rpc] Add logging RPC
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
#     Merge #13275: Qt: use [default wallet] as name for wallet with no name
#     Merge #13506: Qt: load wallet in UI after possible init aborts
#     Merge #13564: [wallet] loadwallet shouldn't create new wallets.
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
#     Merge #13058: [wallet] `createwallet` RPC - create new wallet at runtime
      Update Seeder to use fsbridge::fopen() instead of fopen()
XXXXX Make objects in range declarations immutable by default. Avoid unnecessary copying of objects in range declarations.
#     cli: Ignore libevent warnings
#     Merge #13252: Wallet: Refactor ReserveKeyFromKeyPool for safety
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
XX    Add native support for serializing char arrays without FLATDATA
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
xx    Add static_assert to prevent VARINT(<signed value>)
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
xx    Abstract out BlockAssembler options
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
#     wallet: Use shared pointer to retain wallet instance
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
##    debug log number of unknown wallet records on load
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
##    Remove TestBlockValidity's dependency on Config
##    Remove ConnectBlock's dependency on Config
##    Remove CheckBlock's dependency on Config
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
###   Remove ContextualCheckBlock's dependency on Config
###   Remove CheckBlockHeader's dependency on Config
      [CMAKE] Fix Linux64 toolchain name
#     Source the Excessive block size from BlockValidationOptions
#     Do not construct out-of-bound pointers in SHA2 code
      Avoid triggering undefined behaviour (std::memset(nullptr, 0, 0)) if an invalid string is passed to DecodeSecret(...)
#     Pull leveldb subtree
      [CMAKE] Move version to the top level CMakeLists.txt
      [CMAKE] Rename the top-level and `src/` cmake projects
      Generalized ibd.sh to provide a logging tool for running similar types of tests
###   Modify ContextualCheckBlockHeader to accept a CChainParam rather than a Config
###   Remove ReadBlockFromDisk's dependency on Config
###   Remove dependency on Config from the PoW code
###   Use Consensus::Params in ContextualCheckTransaction and variations instead of Config
###   Activate consensus rule based on consensus params rather than config
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
#     wallet: Catch filesystem_error and raise InitError
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
##    Merge #12431: Only call NotifyBlockTip when chainActive changes
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
##    scripted-diff: Rename CChainState::g_failed_blocks to m_failed_blocks
#     Merge #11714: [tests] Test that mempool rejects coinbase transactions
#     Merge #11133: Document assumptions that are beoing made to avoid division by zero
##    [Part 5 of 5] Add a CChainState class to clarify internal interfaces
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
#     wallet: Make vpwallets usage thread safe
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
#     wallet: Add HasWallets
      qa: Prepare functional tests for Windows
      Fix a-vs-an typos
#     wallet: Add AddWallet, RemoveWallet, GetWallet and GetWallets
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
#     wallet: Add input bytes to CInputCoin
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
####  [rpc] Move DescribeAddressVisitor to rpc/util
####  [rpc] split wallet and non-wallet parts of DescribeAddressVisitor
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
#     wallet: Refactor g_wallet_init_interface to const reference
#     wallet: Make WalletInitInterface members const
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
####  scripted-diff: Replace NET_TOR with NET_ONION
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
##      [Part 4 of 5] Add a CChainState class to clarify internal interfaces
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
