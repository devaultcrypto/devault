// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_TEST_FIXTURE_H
#define BITCOIN_WALLET_TEST_FIXTURE_H

#include <catch_tests/test_bitcoin.h>
#include <wallet/wallet.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>

#include <test/test_bitcoin.h>

#include <memory>

/**
 * Testing setup and teardown for wallet.
 */
struct WalletTestingSetup : public TestingSetup {
    explicit WalletTestingSetup(
        const std::string &chainName = CBaseChainParams::MAIN);
    ~WalletTestingSetup();

    std::unique_ptr<interfaces::Chain> m_chain = interfaces::MakeChain();
    CWallet m_wallet;
};

#endif
