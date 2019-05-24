This release includes the following features and fixes since 1.0.2 release:

Upstream Bitcoin-ABC updates

- Remove Safe Mode
- [schnorr] Refactor the signature process in reusable component
- Merge #12630: Provide useful error message if datadir is not writable
 - Using addresses in createmultisig is now deprectated. Use -deprecatedrpc=createmultisig to get the old behavior.
 - The `createrawtransaction` RPC will now accept an array or dictionary (kept for compatibility) for the `outputs` parameter. This means the order of transaction outputs can be specified by the client.
 - The new RPC `testmempoolaccept` can be used to test acceptance of a transaction to the mempool without adding it.
 - An `initialblockdownload` boolean has been added to the `getblockchaininfo` RPC to indicate whether the node is currently in IBD or not.
