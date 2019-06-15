// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// clang-format off
#include "config/bitcoin-config.h"
#include "chain.h"
#include "rewards_calculation.h"
// clang-format on

Amount CalculateReward(const Consensus::Params &consensusParams, int Height, int HeightDiff, Amount balance) {
  size_t year_number = Height/consensusParams.nBlocksPerYear;
  if (year_number > consensusParams.nPerCentPerYear.size()-1) year_number = consensusParams.nPerCentPerYear.size()-1;
  int64_t percent = consensusParams.nPerCentPerYear[year_number];
  int64_t nRewardRatePerBlockReciprocal = (100*consensusParams.nBlocksPerYear)/percent;
  Amount nMinReward = consensusParams.nMinReward;
  int64_t reward_per_block_x100 = ((100*balance.toInt()) / nRewardRatePerBlockReciprocal);
  int64_t reward_x100 = (HeightDiff * reward_per_block_x100) / Amount::COIN_PRECISION;
  // Quantize reward to 1/100th of a coin
  Amount reward = reward_x100 * (COIN / 100);

  // double debug_reward_div = OverRewardRatePerBlock * HeightDiff;
  if (reward < nMinReward) reward = Amount();
  return reward;
}
