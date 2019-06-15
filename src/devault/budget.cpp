// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "budget.h"
#include "amount.h"
#include "chainparams.h"
#include "config.h"
#include "dstencode.h"
#include "logging.h"
#include "primitives/transaction.h"
#include <consensus/validation.h>

#include <memory>

// Group Address with % for safety/clarity
struct BudgetStruct {
  std::string MainNetAddress;
  std::string TestNetAddress;
  std::string Purpose;
  int64_t percent;
};

// Consts until change is required
// Reward Address & Percentage
const BudgetStruct Payouts[] = {
    {"devault:pp2ghv9ya7fs98rvz3gzuqmen608dh6g2y5d5dxrtp", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "Community",15},
    {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "CoreDevs", 10},
    {"devault:pqws2sgc2y22x2gkcnmw72edpa0u0kscdsqp29e530", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "WebDevs", 5},
    {"devault:pzgux6zlzpw45hm45fwcj5d7mf5fn7pa2ydjzx5nxw", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "BusDevs",5},
    {"devault:prutq74qks5aez2a4mhrcm26t0r3pjzpxyapq0kdjk", "dvtest:qqltstfypfuftlgrvqdfml4js0y3yxdpgya2krkhk5", "Marketing",5},
    {"devault:prcljsfamr0hsc2jn4mr5et9xx5u9lm8rvl976n0zm", "dvtest:qpw43t63nxmufurn5qn9jyurc27xntvdz5n7ujhdaq", "Support", 5}};

// Get Array Size at Compile time for Loops
const int BudgetSize = sizeof(Payouts) / sizeof(BudgetStruct);

bool CBudget::Validate(const CBlock &block, int nHeight, const Amount &BlockReward, Amount& nSumReward) {

  if (!IsSuperBlock(nHeight)) return true;

  Amount refRewards = CalculateSuperBlockRewards(nHeight, BlockReward);
  
  // Just Log during Validation
  for (int i = 0; i < BudgetSize; i++) {
    if (fTestNet) {
      LogPrintf("%s: budget payment to %s (%s) for %d COINs\n", __func__, Payouts[i].Purpose, Payouts[i].TestNetAddress, nPayment[i] );
    } else {
      LogPrintf("%s: budget payment to %s (%s) for %d COINs\n", __func__, Payouts[i].Purpose, Payouts[i].MainNetAddress, nPayment[i] );
    }
  }


  bool fPaymentOK = true;
  auto txCoinbase = block.vtx[0];

  // Verify that the superblock rewards are being payed out to the correct addresses with the correct amounts
  // by going through the coinbase rewards
  nSumReward = Amount();
  for (auto &out : txCoinbase->vout) {
    for (int i = 0; i < BudgetSize; i++) {
      if (out.scriptPubKey == Scripts[i]) {
        if (out.nValue != nPayment[i]) {
          fPaymentOK = false;
        } else {
          nSumReward += nPayment[i];
        }
      }
    }
  }
  if (nSumReward != refRewards) fPaymentOK = false;
  if (!fPaymentOK) LogPrintf("%s: Problem with budget payment in coinbase transaction\n", __func__);
  return fPaymentOK;
}

Amount CBudget::CalculateSuperBlockRewards(int nHeight, const Amount &nOverallReward) {
  if (!IsSuperBlock(nHeight)) return Amount();
  
  // Get Sum of the %s to get a gain factor since we must include block rewards
  // and scale appropriately
  int PerCentSum = 0;
  for (const auto& p : Payouts) PerCentSum += p.percent;
  int ScaleFactor = (100-PerCentSum);
  
  // Overall Reward should be an integer - so divide by COIN and then re-mulitply
  Amount sumRewards;
  for (int i = 0; i < BudgetSize; i++) {
    nPayment[i] = (((Payouts[i].percent * nBlocksPerPeriod * nOverallReward.toInt()) / (ScaleFactor *  COIN.toInt())) * COIN);
    sumRewards += nPayment[i];
  }
  return sumRewards;
}

CBudget::CBudget(const Config &config) {
  const CChainParams &chainparams = config.GetChainParams();

  nPayment = std::make_unique<Amount[]>(BudgetSize);
  Scripts = std::make_unique<CScript[]>(BudgetSize);

  nBlocksPerPeriod = (chainparams.GetConsensus().nBlocksPerYear / 12);
  
  fTestNet = (chainparams.NetworkIDString() != "main");
  
  for (int i = 0; i < BudgetSize; i++) {
      if (fTestNet) {
          Scripts[i] = GetScriptForDestination(DecodeDestination(Payouts[i].TestNetAddress, chainparams));
      } else {
          Scripts[i] = GetScriptForDestination(DecodeDestination(Payouts[i].MainNetAddress, chainparams));
      }
      nPayment[i] = Amount(); // start at 0
  }
}
bool CBudget::FillPayments(CMutableTransaction &txNew, int nHeight, const Amount &nOverallReward) {

  if (!IsSuperBlock(nHeight)) return false;

  CalculateSuperBlockRewards(nHeight, nOverallReward);

  for (int i = 0; i < BudgetSize; i++) {
    CTxOut p1 = CTxOut(nPayment[i], Scripts[i]);
    txNew.vout.push_back(p1);
  }
  return true;
}
