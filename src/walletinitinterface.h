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

class WalletInitInterface {
public:
     /** Is the wallet component enabled */
    virtual bool HasWalletSupport() const = 0;
    /** Get wallet help string */
    virtual void AddWalletOptions() const = 0;
    /** Check wallet parameter interaction */
    virtual bool ParameterInteraction() const = 0;
    /** Register wallet RPC*/
    virtual void RegisterRPC(CRPCTable &) const = 0;
    /** Verify wallets */
    virtual bool Verify(const CChainParams &chainParams) const = 0;
    /** Check if wallet exists already */
    virtual bool CheckIfWalletExists(const CChainParams &chainParams) const = 0;
    /** Open wallets*/
    virtual bool Open(const CChainParams &chainParams, const SecureString& walletPassphrase,
                      const std::vector<std::string>& words, bool use_bls) const = 0;
    /** Start wallets*/
    virtual void Start(CScheduler &scheduler) const = 0;
    /** Flush Wallets*/
    virtual void Flush() const = 0;
    /** Stop Wallets*/
    virtual void Stop() const = 0;
    /** Close wallets */
    virtual void Close() const = 0;

    virtual ~WalletInitInterface() = default;
};

extern const WalletInitInterface& g_wallet_init_interface;
