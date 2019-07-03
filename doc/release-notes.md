This release includes the following features and fixes since 1.0.4 release:

TBD

Upstream Bitcoin-ABC updates

- Merge #12630: Provide useful error message if datadir is not writable
 - Using addresses in createmultisig is now deprectated. Use -deprecatedrpc=createmultisig to get the old behavior.
 - The `createrawtransaction` RPC will now accept an array or dictionary (kept for compatibility) for the `outputs` parameter. This means the order of transaction outputs can be specified by the client.
 - The new RPC `testmempoolaccept` can be used to test acceptance of a transaction to the mempool without adding it.
 - An `initialblockdownload` boolean has been added to the `getblockchaininfo` RPC to indicate whether the node is currently in IBD or not.

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
