// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkcoins.h>
#include <devault/rewards.h>
#include <chainparams.h>
#include <dstencode.h>

const double minThresholdPercent = 10.1; // 10%

std::string checkcoins(const interfaces::Wallet::CoinsList &coinList) {

    // List of non-valid coldrewards
    std::vector<COutPoint> invalid_cold;
    std::vector<COutPoint> valid_but_young_cold;
    Amount invalid_sum;
    Amount valid_sum;

    for (const auto &coins : coinList) {
        auto sWalletAddress = EncodeDestination(coins.first);
        Amount nSum = Amount::zero();

        for (const auto &outpair : coins.second) {
            const COutPoint &output = std::get<0>(outpair);
            const interfaces::WalletTxOut &out = std::get<1>(outpair);
            nSum += out.txout.nValue;

            // address
            CTxDestination outputAddress;
            if (ExtractDestination(out.txout.scriptPubKey, outputAddress)) {
                auto sAddress = EncodeDestination(outputAddress);
                // amount
                // BitcoinUnits::format(nDisplayUnit, out.txout.nValue));

                CRewardValue rewardval;
                if (prewardsdb->GetReward(output, rewardval)) {
                    ///
                    auto Height = chainActive.Tip()->nHeight;
                    auto nMinBlock = Params().GetConsensus().nMinRewardBlocks;
                    auto payAge = 0.1 * int((1000.0 * (Height - rewardval.GetHeight())) /nMinBlock);
                    if (payAge < minThresholdPercent) {
                      valid_but_young_cold.push_back(output);
                      valid_sum += out.txout.nValue;
                    }
                } else {
                    invalid_cold.push_back(output);
                    invalid_sum += out.txout.nValue;
                }
            }
        }
    }

    std::stringstream s;
    auto Height = chainActive.Tip()->nHeight;
    Amount total_sum = invalid_sum + valid_sum;
    if (total_sum > Params().GetConsensus().getMinRewardBalance(Height)) {
      if (invalid_sum > Amount(0)) {
        s << "unused sum = " << invalid_sum.ToString() << " in " << invalid_cold.size() << " utxos ";
      }
      if (valid_sum > Amount(0)) {
        s << " and valid sums of " << valid_sum.ToString() << " in " << valid_but_young_cold.size() << " utxos "
          << " that are less than " << 0.01*minThresholdPercent*30 << " days old \n";
      }
    } else {
      s << "Total amount that could be moved is less than the minimum reward\n";
    }
    return s.str();
}
