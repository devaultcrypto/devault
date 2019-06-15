// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "config.h"

#include "devault/rewards_calculation.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_FIXTURE_TEST_SUITE(reward_tests, BasicTestingSetup)

// For 1st year below 4000 shouldn't payoff before 1 month
BOOST_AUTO_TEST_CASE(zero_reward_checks) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<int64_t> balances = {3999, 1999, 999};
    std::vector<int64_t> height_diff_months = {1, 2, 4};

    const Consensus::Params &params = config.GetChainParams().GetConsensus();
    int month = params.nMinRewardBlocks;

    int i = 0;
    for (auto balance : balances) {
        int height_diff = (height_diff_months[i] * month +
                           1); // 1 extra block because of rounding down
        int height = 0;        // 1st year
        Amount bal(balance * COIN);
        Amount v(CalculateReward(params, height, height_diff, bal));
        BOOST_CHECK_EQUAL(v, Amount::zero());
        i++;
    }
}

// 12000 should payout decreasing amounts down to min of 50 after 5 years
BOOST_AUTO_TEST_CASE(Reward12k) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<int64_t> years = {0, 1, 2, 3, 4, 5};
    std::vector<int64_t> ref_amount = {150, 120, 90, 70, 50, 50};

    const Consensus::Params &params = config.GetChainParams().GetConsensus();
    int month = params.nMinRewardBlocks + 1;

    int i = 0;
    Amount bal(12000 * COIN);
    int height_diff = month;

    for (auto y : years) {
        int height = y * 12 * month;
        Amount r(ref_amount[i] * Amount::COIN_PRECISION);
        Amount v(CalculateReward(params, height, height_diff, bal));
        BOOST_CHECK_EQUAL(v, r);
        i++;
    }
}

// Amount Balances needed to get Minimum Reward Amount
BOOST_AUTO_TEST_CASE(minRewardTests) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<int64_t> balances = {4000, 5000, 6667, 8572, 12000};
    std::vector<int64_t> years = {0, 1, 2, 3, 4, 5};

    const Consensus::Params &params = config.GetChainParams().GetConsensus();
    int month = params.nMinRewardBlocks + 1;
    int height_diff = month;

    int i = 0;
    for (auto balance : balances) {
        int height = years[i] * 12 * month;
        Amount bal(balance * COIN);
        Amount v(CalculateReward(params, height, height_diff, bal));
        // std::cout << " bal = " << bal.ToString() << " @ " << years[i] << "
        // years: " << v.ToString() << "\n";
        BOOST_CHECK_EQUAL(v, params.nMinReward);
        i++;
    }
}

BOOST_AUTO_TEST_SUITE_END()
