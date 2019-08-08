// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <devault/rewards.h>
#include <dstencode.h>
#include <wallet/analyzecoins.h>

std::vector<CInputCoin> analyzecoins(const std::map<CTxDestination, std::vector<COutput>> &coinList, double minPercent) {
  // List of non-valid coldrewards
  std::vector<COutPoint> invalid_cold;
  std::vector<COutPoint> valid_but_young_cold;
  std::vector<CInputCoin> selected_coins;
  Amount invalid_sum;
  Amount valid_sum;

  for (const auto &coins : coinList) {
    auto sWalletAddress = EncodeDestination(coins.first);
    Amount nSum = Amount::zero();

    for (const auto &outpair : coins.second) {
      CInputCoin coin = CInputCoin(outpair.tx, outpair.i);
      COutPoint output = coin.outpoint;
      CTxOut txout = coin.txout;
      nSum += txout.nValue;

      // address
      CTxDestination outputAddress;
      if (ExtractDestination(txout.scriptPubKey, outputAddress)) {
        auto sAddress = EncodeDestination(outputAddress);
        CRewardValue rewardval;
        if (prewardsdb->GetReward(output, rewardval)) {
          ///
          auto Height = chainActive.Tip()->nHeight;
          auto nMinBlock = Params().GetConsensus().nMinRewardBlocks;
          auto payAge = 0.1 * int((1000.0 * (Height - rewardval.GetHeight())) / nMinBlock);
          if (payAge < minPercent) {
            valid_but_young_cold.push_back(output);
            valid_sum += txout.nValue;
            selected_coins.push_back(coin);
          }
        } else {
          selected_coins.push_back(coin);
          invalid_cold.push_back(output);
          invalid_sum += txout.nValue;
        }
      }
    }
  }
  return selected_coins;
}
