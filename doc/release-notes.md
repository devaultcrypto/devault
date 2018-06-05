This release includes the following features and fixes since 1.1.4 release:

This release includes the following features and fixes:
 - Minor bug fixes and improvements.
 - New `fees` field introduced in `getrawmempool`, `getmempoolancestors`,
   `getmempooldescendants` and  `getmempoolentry` when verbosity is set to
   `true` with sub-fields `ancestor`, `base`, `modified` and `descendant`
   denominated in BCH. This new field deprecates previous fee fields, such a
   `fee`, `modifiedfee`, `ancestorfee` and `descendantfee`.

Dynamic creation of wallets
---------------------------------------
  - Previously, wallets could only be loaded or created at startup, by
    specifying `-wallet` parameters on the command line or in the bitcoin.conf
    file. It is now possible to create wallets dynamically at runtime:

  - New wallets can be created (and loaded) by calling the `createwallet` RPC.
    The provided name must not match a wallet file in the `walletdir` directory
    or the name of a wallet that is currently loaded.

  - This feature is currently only available through the RPC interface.

 - Wallets loaded dynamically through the RPC interface may now be displayed in
   the Devault-Core GUI.
 - The default wallet will now be labeled `[default wallet]` in the DeVault-Core
   GUI if no name is provided by the `-wallet` option on start up.
 - It is now possible to unload wallets dynamically at runtime. This feature is
   currently only available through the RPC interface.
 - Wallets dynamically unloaded will now be reflected in the gui.
