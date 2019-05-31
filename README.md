![DeVault](./share/pixmaps/dvt-logo.png)

This is the working repository for [DeVault](https://devault.cc).

[![Build Status](https://travis-ci.com/devaultcrypto/devault.svg?branch=master)](https://travis-ci.com/devaultcrypto/devault) [![GitHub version](https://badge.fury.io/gh/devaultcrypto%2Fdevault.svg)](https://badge.fury.io/gh/devaultcrypto%2Fdevault)  
=====

The goal of DeVault is to create a decentralized economy that is usable by everyone 
in the world. We believe this is a civilization-changing technology which can
dramatically improve the lives of all. 

What is DeVault?
---------------------

[DeVault](https://www.devault.cc/) is a peer-to-peer digital payment platform built 
with the goal of harnessing the collective knowledge and intrinsic value of the p2p economy. 
Creating an ever growing Multi-DAO system geared towards educating, on-boarding 
and supporting the crypto-currency adopters of today and tomorrow.

What is DeVault Core?
--------------------

DeVault Core is the name of open-source software which enables the use of
DeVault. It is a fork of the [Bitcoin ABC](https://bitcoinabc.org)
software project.

License
-------

DeVault is released under the terms of the MIT license. See
[COPYING](COPYING) for more information or see
https://opensource.org/licenses/MIT.

Development Process
-------------------

If you would like to contribute, please read [CONTRIBUTING](CONTRIBUTING.md).

Disclosure Policy
-----------------

See [DISCLOSURE_POLICY](DISCLOSURE_POLICY.md)

Upgrades/Changes in this code fork
-----------------

- Cold Rewards Code (details here : [Cold Reward](COLDREWARDS.md))
- Budget Rewards (detail here : [Budget](BUDGET.md))
- `Shark` block inflation for a fairer initial distribution - details here : [Inflation](INFLATION.md) ![Shark](./share/pixmaps/Shark.png)
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

## Specifications

| Specification         | Descriptor                              |
|-----------------------|-----------------------------------------|
| Ticker                | DVT                                     |
| Algorithm             | SHA256d                                 |
| RPC Port              | 3339                                    |
| P2P Port              | 33039                                   |
| Block Spacing         | 120 Seconds                             |
| Difficulty Algorithm  | LWMA                                    |
| Block Size            | 32MB                                    |
| Protocol Support      | IPV4, IPV6 & TOR                        |

## ColdReward Requirements

| Requirement   | Details              |
|---------------|----------------------|
| Confirmations | 21915 Blocks         |
| Amount        | 1000+ DVT (Per Input)|

