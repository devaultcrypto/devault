// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <walletinitinterface.h>

class DummyWalletInit : public WalletInitInterface {
public:
    void AddWalletOptions() const override {}
    bool HasWalletSupport() const override { return false; }
    bool ParameterInteraction() const override { return true; }
    bool CheckIfWalletExists(const CChainParams &chainParams) const override { return false; }
    void Construct(InitInterfaces &interfaces) const override {
        LogPrintf("No wallet support compiled in!\n");
    }
};

void DummyWalletInit::AddWalletOptions() const {
    std::vector<std::string> opts = {
        "-avoidpartialspends", "-disablewallet", "-fallbackfee=<amt>",
        "-keypool=<n>", "-maxtxfee=<amt>", "-mintxfee=<amt>", "-paytxfee=<amt>",
        "-rescan", "-salvagewallet", "-spendzeroconfchange", "-upgradewallet",
        "-wallet=<path>", "-walletbroadcast", "-walletdir=<dir>",
        "-walletnotify=<cmd>", "-zapwallettxes=<mode>",
        // Wallet debug options
        "-dblogsize=<n>", "-flushwallet", "-privdb", "-walletrejectlongchains"};
    gArgs.AddHiddenArgs(opts);
}

const WalletInitInterface &g_wallet_init_interface = DummyWalletInit();

void StopGenerateThread() {}
