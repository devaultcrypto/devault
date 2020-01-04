# BLS FAQ

# BLS Roll out phases

* Hard fork with new code to allow BLS addresses and transactions
  - Includes ability of users to update wallet so that new addresses are BLS addresses
    - BLS Address will use BIP32/44/39 based on "account" 1, EC Addresses are "account" 0
  - Hard fork is for QT-wallets/daemons
  - Mobile wallets will not be updated and will function as before
  - Exchanges will be asked to change to allow deposits to BLS addresses, withdrawals to BLS addresses
  
* Phase out legacy addresses part 1
  - QT Wallet update
    - Will only allow sends to BLS addresses
    - QT/daemon client will disallow mixing BLS and legacy addresses as inputs in the same transaction 
    - Network still will suport legacy addresses and mobile still unchanged
    - Mining should be to BLS addresses by default
    - New wallets will only generated BLS addresses but recovery will allow recovery of legacy addresses

* Phase out legacy addresses part 2
  - Cold Rewards will only occur for BLS addresses (requiring movement from legacy addresses)
  - Mobile wallets updated to BLS Addresses usage
    - Rather than having mixed schemes, 2 separate wallets will be maintained such that new version is BLS only


* Phase out legacy addresses part 3
  - Only odd number blocks will allow legacy address transactions so that even numbered ones can be optimized
  - BLS Aggregation will take place across transactions in BLS only blocks


* Phase out legacy addresses part 4
  - Legacy addresses only allow on select number of blocks or some form of extension block created to support them











