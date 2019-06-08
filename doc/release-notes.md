DeVault version 1.0.1 is now available from:

  <https://github.com/devaultcrypto/devault/>

This release includes the following features and fixes since 1.0.0 release:

- Throw error for invalid coin precision when using console/command for sending money
- add antialiasing to net traffic gui
- Fix some potential dereferencing issues
- Re-enable ParseMoney tests in util_tests unit tests
- Fix issue with FormatMoney causing errors in fraction Devault values for rpc outputs
- Remove warning for exceeding threshold of unknown block versions
- update seeder
- Refactoring mnemonic checking, etc for GUI use
- Change netmagic value of 0x3l to 0x03 for clarity (same value)
- Refactor code related to Amount class

Upstream Bitcoin-ABC updates

- Merge #9906: Disallow copy constructor CReserveKeys
- Merge #11744: net: Add missing locks in net.{cpp,h}
- Merge #9539: [net] Avoid initialization to a value that is never read
- Merge #12326: net: initialize socket to avoid closing random fd's
- Merge #11252: [P2P] When clearing addrman clear mapInfo and mapAddr.
- Merge #12448: Interrupt block generation on shutdown request
- Merge #10914: Add missing lock in CScheduler::AreThreadsServicingQueue()
- Merge #11831: Always return true if AppInitMain got to the end
- Merge #10057: [init] Deduplicated sigaction() boilerplate
- Init: Remove redundant exit(EXIT_FAILURE) instances and replace with return false
- Ignore macOS daemon() depracation warning
- Merge #9693: Prevent integer overflow in ReadVarInt.
- Merge #10027: Set to nullptr after delete
- Merge #10029: Fix parameter naming inconsistencies between .h and .cpp files
- Merge #12349: shutdown: fix crash on shutdown with reindex-chainstate
- Merge #12367: Fix two fast-shutdown bugs
- Merge #11238: Add assertions before potential null deferences
- [db] Migration for txindex data to new, separate database.
- [db] Create separate database for txindex.
- Remove obsolete comment from MANDATORY_SCRIPT_VERIFICATION_FLAGS
- Merge #10569: Fix stopatheight
- Merge #11880: Stop special-casing phashBlock handling in validation for TBV
- Do not allow users to get keys from keypool without reserving them
- Merge #9517: [refactor] Switched httpserver.cpp to use RAII wrapped libevents.
- Merge #11012: Make sure to clean up mapBlockSource if we've already seen the block
- serialize: Serialization support for big-endian 32-bit ints.
- [qt] Simplifies boolean expression model && model->haveWatchOnly()
 
