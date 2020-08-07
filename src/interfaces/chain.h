// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CHAIN_H
#define BITCOIN_INTERFACES_CHAIN_H

#include <memory>
#include <string>
#include <vector>

namespace interfaces {

//! Interface for giving wallet processes access to blockchain state.
class Chain {
public:
    virtual ~Chain() {}
};

//! Interface to let node manage chain clients (wallets, or maybe tools for
//! monitoring and analysis in the future).
class ChainClient {
public:
    virtual ~ChainClient() {}
};

//! Return implementation of Chain interface.
std::unique_ptr<Chain> MakeChain();

//! Return implementation of ChainClient interface for a wallet client. This
//! function will be undefined in builds where ENABLE_WALLET is false.
//!
//! Currently, wallets are the only chain clients. But in the future, other
//! types of chain clients could be added, such as tools for monitoring,
//! analysis, or fee estimation. These clients need to expose their own
//! MakeXXXClient functions returning their implementations of the ChainClient
//! interface.
std::unique_ptr<ChainClient>
MakeWalletClient(Chain &chain, std::vector<std::string> wallet_filenames);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_CHAIN_H
