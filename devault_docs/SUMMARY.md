![DeVault](../share/pixmaps/dvt-logo.png)

This file is an ongoing list of various things done differently in DeVault vs a pure Bitcoin clone

-----------------

- Cold Rewards Code (details here : [Cold Reward](COLDREWARDS.md))
- Check peers to see if new Wallet version is available   
- Budget Rewards (detail here : [Budget](BUDGET.md))
- `Shark` block inflation for a fairer initial distribution : [Inflation](INFLATION.md)
- Removed BIP70 & protobuf dependency
- Remove Base58 address support
- Transition to BIP 32/39/44 HD Wallet support only - Uses 12 word phrase setup
- Each run of devaultd/devault-qt will create a new debug.log file and rename older files based on last accessed time
  Use -keeplogfiles=<days> to specify how long to keep in days (default is 7)
- Change coin display precision and current network precision to 3 decimal points instead of 8
- Remove BIP9 code
- Prompt user for Password on 1st run so that wallet will always be encrypted
- Allow blank bypass password for miners if needed1
- Exclusive use of Bech32 style addresses
- Replace Difficulty with LWMA difficulty calculation
- Reorg Depth set at 30 blocks
- Default # of keys generated is 200 in total, miners may want to use -keypool at 1st startup for additional keys

- GUI Updates/Changes
   * GUI Reskin
   * GUI Reward Tab
   * Add Menu items to Unlock Wallet, Sweep Coins, Show Word Phrase
   * Bip 32/39/44 GUI for Restoring wallets and/or New Wallet Setup
   
- New/added RPC commands
  * Add the `getblockstats` RPC to get statistics on a block or a block range.
  * Add `multisigsignraw` RPC command to make it easier for team to sign multisig transactions
  * `getmyrewardinfo`, `getrewardinfo` rpc commands
  * `consolidaterewards` to move coins for ColdRewards when needed
  * `getblockbynumber` to avoid having to do `getblockhash` followed by `getblock`

- Build/Dev upgrades
  * Updated CMake config
  * Uses C++17 for builds
  * Remove some Boost dependencies - no link dependencies on some platforms
  * Replace OpenSSL dependency with Libsodium
  * Code can be built with either AutoTools or CMake
  * Upgraded or added dependencies, QT 5.9.7, libsodium, libgmp, boost 1.69
  * Use Catch framework for unit tests
  * Newer platforms don't require linking to any Boost libraries, allowing std::filesystem to be used
