DeVault version 1.0.0 is now available from:

  <https://github.com/devaultcrypto/devault/>

This release includes the following features and fixes since forking from Bitcoin-ABC 19.0 :
- Cold Rewards Code 
- Budget Rewards 
- `Shark` block inflation for a fairer initial distribution
- Removed BIP70 & protobuf dependency
- Updated CMake config
- Redesigned QT Wallet GUI
- Uses C++17 for builds
- Remove Base58 address support
- Transition to BIP 32/39/44 HD Wallet support only - Uses 12 word phrase setup
- Remove some Boost dependencies
- Each run of devaultd/devault-qt will create a new debug.log file and rename older files based on last accessed time
  Use -keeplogfiles=<days> to specify how long to keep in days (default is 7)
- Change coin display precision and current network precision to 3 decimal points instead of 8
- Remove BIP9 code
- Prompt user for Password on 1st run so that wallet will always be encrypted
- Exclusive use of Bech32 style addresses
- Replace OpenSSL dependency with Libsodium
- Replace Difficulty with LWMA difficulty calculation
- Code can be built with either AutoTools or CMake
- Reorg Depth set at 30 blocks
- Upgraded or added dependencies, QT 5.9.7, libsodium, libgmp, boost 1.69
- Default # of keys generated is 200 in total, miners may want to use -keypool at 1st startup for additional keys

Bitcoin-ABC 19.x backports:
 
 - Add `signrawtransactionwithkey` and `signrawtransactionwithwallet` RPCs.
   These are specialized subsets of the `signrawtransaction` RPC.
 - Deprecate `nblocks` parameter in `estimatefee`.  See `bitcoin-cli help estimatefee` for more info. Use `-deprecatedrpc=estimatefee`
      to temporarily re-enable the old behavior while you migrate.
 - Minor bug fixes and wallet UI cleanup
 - Removed `txconfirmtarget` option from bitcoind
 - Added parameter `include_removed` to `listsinceblock` for better tracking of transactions during a reorg. See `bitcoin-cli help listsinceblock` for more details.
 - `listsinceblock` will now throw an error if an unknown `blockhash` argument value is passed, instead of returning a list of all wallet transactions since
   the genesis block.
 - Various minor fixes to RPC parameter validation
 - Minor wallet performance improvements
 - `errors` in getmininginfo rpc commmand has been deprecated.  Use `warnings` now instead.
 - Added optional `blockhash` parameter to `getrawtransaction` to narrowly
   search for a transaction within a given block. New returned field
   `in_active_chain` will indicate if that block is part of the active chain.
 - `signrawtransaction` RPC is now deprecated. The new RPCs 
   `signrawtransactionwithkey` and `signrawtransactionwithwallet` should 
   be used instead.
 - Added to `getblockchaininfo` `size_on_disk` and, when the prune option is enabled, `prune_height`, `automatic_pruning`, and `prune_target_size`.
    - The help message also reflects this.
 - Remove `depends` from transaction objects provided by `getblocktemplate`.
