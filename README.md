![DeVault](./share/pixmaps/dvt-logo.png)

This is the working repository for [DeVault](https://devault.cc)


[![Build Status](https://travis-ci.com/devaultcrypto/devault.svg?branch=master)](https://travis-ci.com/devaultcrypto/devault)


===========

The goal of DeVault is to create sound money that is usable by everyone in
the world. We believe this is a civilization-changing technology which will
dramatically increase human flourishing, freedom, and prosperity. The project
aims to achieve this goal by implementing a series of optimizations and
protocol upgrades that will enable peer-to-peer digital cash to scale many
orders of magnitude beyond current limits.

What is DeVault?
---------------------

[DeVault](https://www.devault.cc/) is an experimental digital
currency that enables instant payments to anyone, anywhere in the world. It
uses peer-to-peer technology to operate with no central authority: managing
transactions and issuing money are carried out collectively by the network.

What is DeVault?
--------------------

DeVault is the name of open-source software which enables the use of
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

- Cold Rewards Code (details here : TBD)
- Budget Rewards (detail here : TBD)
- `Shark` block inflation for a fairer initial distribution (details here : TBD)
- Removed BIP70 & protobuf dependency
- Updated CMake config
- Redesigned QT Wallet GUI
- Uses C++17 for builds
- Remove Base58 address support
- Transition to BIP 32/39/44 HD Wallet support only
- Remove some Boost dependencies
- Each run of devaultd/devault-qt will create a new debug.log file and rename older files based on last accessed time
  Use -keeplogfiles=<days> to specify how long to keep in days (default is 7)
- Change coin display precision and current network precision to 3 decimal points instead of 8
- Remove BIP9 code
- Prompt user for Password on 1st run so that wallet will always be encrypted
- Exclusive use of Bech32 style addresses
- Replace OpenSSL dependency with Libsodium
