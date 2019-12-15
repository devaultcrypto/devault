## Exchange FAQ for DeVault
---------------------

Exchange Critical Information - please read carefully
-----------------

- It is a clone of Bitcoin Cash and uses Bech32 style addresses exclusively
- Transaction amounts can vary between 2 billion and 0.001 DVT. i.e currently only 3 decimal places are supported
- Transaction with more than 3 decimal place precision will be rejected by network/client
- Password setup can be bypassed by using the -bypasspassword command line option to the client. This must be used the first time the wallet is setup and encrypts the wallet with a empty string.  The wallet will always be encrypted. Subsequent uses of the client should then also pass the -bypassspassword option to have the wallet unlocked
- Otherwise, for typical use, passwords must be setup when 1st running the wallet - either devaultd or DeVault-Core and subsequently used to unlock the wallet.
- Default # of keys generated in wallet is 200 in total, use -keypool for other amounts

Other important info for exchanges
-----------------

- UTXOs in excess of 4000 DVT will typically generate "Cold Rewards" every 21915 blocks corresponding to a specified interest rate, interest will be generated for amounts 1000-4000 less frequently and nothing will be generated for < 1000 DVT.
- The Cold Rewards are generated as part of the Coinbase transaction (supported by getblocktemplate for miner use) with at most 1 reward per block.
- The Mining Block Reward is not similar to bitcoin and increases gradually before hitting a peak about 1 1/2 years from launch and then declining
- Superblocks are generated every 21915 blocks and provide additional rewards to DAO addresses


## Specifications

| Specification         | Descriptor                              |
|-----------------------|-----------------------------------------|
| Ticker                | DVT                                     |
| Algorithm             | SHA256d Proof of Work                   |
| RPC Port              | 3339                                    |
| P2P Port              | 33039                                   |
| Block Spacing         | 120 Seconds                             |
| Difficulty Algorithm  | LWMA                                    |
| Block Size            | 32MB                                    |
| Protocol Support      | IPV4, IPV6 & TOR                        |

## ColdReward Requirements

| Requirement   | Details                                            |
|---------------|----------------------------------------------------|
| Confirmations | 21915 Blocks                                       |
| Amount        | 1000+ DVT (Per Input)                              |
| Interest Rate | 15% for year 1, then declining - subject to change |

