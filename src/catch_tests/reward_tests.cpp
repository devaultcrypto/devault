// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <config.h>

#include <devault/rewards_calculation.h>
#include "catch_unit.h"

#include <vector>

// BOOST_FIXTURE_TEST_SUITE(reward_tests, BasicTestingSetup)

// For 1st year below 4000 shouldn't payoff before 1 month
TEST_CASE("zero_reward_checks") {
  DummyConfig config(CBaseChainParams::MAIN);
  std::vector<int64_t> balances = {3999, 1999, 999};
  std::vector<int64_t> height_diff_months = {1, 2, 4};

  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  int month = params.nMinRewardBlocks;

  int i = 0;
  for (auto balance : balances) {
    int height_diff = (height_diff_months[i] * month + 1); // 1 extra block because of rounding down
    int height = 0;                                        // 1st year
    Amount bal(balance * COIN);
    Amount v(CalculateReward(params, height, height_diff, bal));
    // std::cout << " bal = " << bal.ToString() <<  " reward = " << v.ToString() << "\n";
    BOOST_CHECK_EQUAL(v, Amount::zero());
    i++;
  }
}

// 12000 should payout decreasing amounts down to min of 50 after 5 years
TEST_CASE("Reward12k") {
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
    // std::cout << " bal = " << bal.ToString() <<  " reward = " << v.ToString() << "\n";
    BOOST_CHECK_EQUAL(v, r);
    i++;
  }
}
// 1000 requires longer differential heights as % goes down over time
TEST_CASE("Reward1k") {
  DummyConfig config(CBaseChainParams::MAIN);
  std::vector<int64_t> years = {0, 1, 2, 3, 4};
  std::vector<int64_t> diff = {4, 6, 8, 12, 12};
  std::vector<int64_t> ref_amount = {50, 60, 60, 70, 50};

  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  int month = params.nMinRewardBlocks + 1;

  int i = 0;
  Amount bal(1000 * COIN);

  for (auto y : years) {
    int height = y * 12 * month;
    int height_diff = month * diff[i];
    Amount r(ref_amount[i] * Amount::COIN_PRECISION);
    Amount v(CalculateReward(params, height, height_diff, bal));
    // std::cout << " bal = " << bal.ToString() <<  " height diff = " << diff[i] << "m, reward = " << v.ToString() <<
    // "\n";
    BOOST_CHECK_EQUAL(v, r);
    i++;
  }
}

// Amount Balances needed to get Minimum Reward Amount
TEST_CASE("minRewardTests") {
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
    // std::cout << " bal = " << bal.ToString() << " @ " << years[i] << " years: " << v.ToString() << "\n";
    BOOST_CHECK_EQUAL(v, params.nMinReward);
    i++;
  }
}

// Check for possible overflows and Max out
TEST_CASE("RewardMaxes") {
  DummyConfig config(CBaseChainParams::MAIN);
  std::vector<int64_t> balances = {10000, 100000, 1000000, 10000000, 100000000};
  std::vector<int64_t> ref_amount = {125, 1250, 12500, 125005, 1250057};

  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  int month = params.nMinRewardBlocks + 1;

  int i = 0;

  for (auto balance : balances) {
    Amount bal(balance * COIN);
    Amount r(ref_amount[i] * Amount::COIN_PRECISION);
    Amount v(CalculateReward(params, 0, month, bal));
    // std::cout << " bal = " << bal.ToString() << " reward : " << v.ToString() << "\n";
    BOOST_CHECK_EQUAL(v.toIntCoins(), ref_amount[i]);
    i++;
  }
}

// Check MAX_MONEY is OK on year 1 for a height diff of 1 year without overflow
TEST_CASE("RewardMaxMoney") {
  DummyConfig config(CBaseChainParams::MAIN);

  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  int month = params.nMinRewardBlocks + 1;

  Amount v(CalculateReward(params, 0, 12 * month, MAX_MONEY));
  // std::cout << " MAX_MONEY = " << MAX_MONEY.ToString() << " reward : " << v.ToString() << "\n";
  BOOST_CHECK_EQUAL(v, Amount(30001368925000000));
}

// BOOST_AUTO_TEST_SUITE_END()
