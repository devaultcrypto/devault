This release includes the following features and fixes since 1.0.4 release:

 - Migrate unit tests away from Boost unit test framework and use header-only catch framework instead
 - Update/fix various unit tests
 - Fix consensus issue whereby client halted rather than rejecting invalid incoming blocks and continuing on
 - Upgrade/cleanup wallet processing so that initial wallet setup for new users is much faster than before
 - Add ability to use 24 word phrases for wallet
 - Various compiler/warning fixes and code cleanup

Upstream Bitcoin-ABC/Bitcoin updates

 - Using addresses in createmultisig is now deprectated. Use -deprecatedrpc=createmultisig to get the old behavior.
 - The `createrawtransaction` RPC will now accept an array or dictionary (kept for compatibility) for the `outputs` parameter. This means the order of transaction outputs can be specified by the client.
 - The new RPC `testmempoolaccept` can be used to test acceptance of a transaction to the mempool without adding it.
 - An `initialblockdownload` boolean has been added to the `getblockchaininfo` RPC to indicate whether the node is currently in IBD or not.
 - Add the `minrelaytxfee` output to the `getmempoolinfo` RPC.
 - For full list of Bitcoin/Bitcoin-ABC backports please see the doc/abc_update_logs.md file

Transaction index changes
-------------------------

The transaction index is now built separately from the main node procedure,
meaning the `-txindex` flag can be toggled without a full reindex. If bitcoind
is run with `-txindex` on a node that is already partially or fully synced
without one, the transaction index will be built in the background and become
available once caught up. When switching from running `-txindex` to running
without the flag, the transaction index database will *not* be deleted
automatically, meaning it could be turned back on at a later time without a full
resync.

