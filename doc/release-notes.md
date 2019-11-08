This release includes the following features and fixes since 1.1.0 release:
  - Add the `getblockbynumber` RPC to avoid need to use getblockhash followed by getblock
  - Add copyaddress for RewardsControl
  - Add RPC (`sweeprivkey`) function + GUI function to sweep private key funds into wallet
  - Add RPC `getutxobalance` to check balance without need to use -addressindex option
  - Remove RPC functions `importprivkey`, `importmulti`, add `getutxtobalance`
  - Remove RPC functions `getaddressesbyaccount, `getaccountaddress`, `getaccount`, `setaccount`
  - Add RPC functions `getaddressesbylabels`
  
  