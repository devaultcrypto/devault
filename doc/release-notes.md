This release includes the following features and fixes since 1.0.1 release:

- refactoring of Amount class & related code.
- Discovered and fixed issue that would have led to large % errors in some reward amounts
- Update/fix unit tests
- Fix builds without wallet (ENABLE_WALLET) for non-QT targets
- Fix issue with getmyrewardinfo showing wrong estimated dates


Upstream Bitcoin-ABC updates

- Remove Safe Mode
- [schnorr] Refactor the signature process in reusable component
- Merge #12630: Provide useful error message if datadir is not writable
 - Using addresses in createmultisig is now deprectated. Use -deprecatedrpc=createmultisig to get the old behavior.
- Various other ABC updates, see abc_update_logs.md for merged commits