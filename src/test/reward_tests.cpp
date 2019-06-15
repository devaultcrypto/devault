// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "chainparams.h"
#include "config.h"

#include "devault/rewards_calculation.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <array>

BOOST_FIXTURE_TEST_SUITE(reward_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RewardTests) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::array<int64_t, 8> months = {{1,2,6,6,6,24,48, 60}};
    std::array<int64_t, 8> balances = {{4000, 40000, 400000, 4000000, 5000, 10000, 50000, 99999999}};
    std::array<int64_t, 8> min_balances = {{1,1,1,1,1,1,1,1}};
    std::array<int64_t, 8> ref_amount = {{65,0,65,0,65,65,635, 624976}};

    const Consensus::Params &params = config.GetChainParams().GetConsensus();

  
    for (int i=0;i<8;i++) {
      // 1 full month (for now)
      int height_diff = params.nMinRewardBlocks;
      int height = months[i]*params.nMinRewardBlocks;
      Amount bal(balances[i] * COIN);
      Amount r(ref_amount[i]*Amount::COIN_PRECISION);

      Amount v(CalculateReward(params, height, height_diff, bal));
      std::cout << " bal = " << bal.ToString() << " for " << months[i] << "m : " << v.ToString() << "\n";
      // BOOST_CHECK_EQUAL(CalculateReward(params, height_diff, height_diff, bal), r);
    }
}

BOOST_AUTO_TEST_SUITE_END()
