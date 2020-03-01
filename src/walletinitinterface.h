// Copyright (c) 2017 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <chainparams.h>
#include <support/allocators/secure.h>
#include <string>

class CScheduler;
class CRPCTable;
struct InitInterfaces;

namespace interfaces {
class Chain;
} // namespace interfaces

class WalletInitInterface {
public:
     /** Is the wallet component enabled */
    virtual bool HasWalletSupport() const = 0;
    /** Get wallet help string */
    virtual void AddWalletOptions() const = 0;
    //virtual std::string GetHelpString(bool showDebug) const = 0;
    /** Check wallet parameter interaction */
    virtual bool ParameterInteraction() const = 0;
    /** Check if wallet exists already */
    virtual bool CheckIfWalletExists(const CChainParams &chainParams) const = 0;
    /** Add wallets that should be opened to list of init interfaces. */
    virtual void Construct(InitInterfaces &interfaces) const = 0;
 
    virtual ~WalletInitInterface() = default;
};

extern const WalletInitInterface& g_wallet_init_interface;
