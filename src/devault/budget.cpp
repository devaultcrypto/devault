// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/budget.h>
#include <amount.h>
#include <config.h>
#include <dstencode.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <consensus/validation.h>

#include <memory>

    
// Group Address with % for safety/clarity
struct BudgetStruct {
  std::string MainNetAddress;
  std::string TestNetAddress;
  std::string Purpose;
  int64_t percent;
};

struct BudgetPayouts {
    int32_t changeSuperBlock; // Where to switch between old DAO addresses and new DAO addresses
    std::vector<BudgetStruct> budget; 
};

// Reward Address & Percentage
const BudgetPayouts Payouts[] =
    {
     {
      5, // change on 5th Superblock
      {{"devault:pp2ghv9ya7fs98rvz3gzuqmen608dh6g2y5d5dxrtp", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "Community",15},
       {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "CoreDevs", 10},
       {"devault:pqws2sgc2y22x2gkcnmw72edpa0u0kscdsqp29e530", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "WebDevs", 5},
       {"devault:pzgux6zlzpw45hm45fwcj5d7mf5fn7pa2ydjzx5nxw", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "BusDevs",5},
       {"devault:prutq74qks5aez2a4mhrcm26t0r3pjzpxyapq0kdjk", "dvtest:qqltstfypfuftlgrvqdfml4js0y3yxdpgya2krkhk5", "Marketing",5},
       {"devault:prcljsfamr0hsc2jn4mr5et9xx5u9lm8rvl976n0zm", "dvtest:qpw43t63nxmufurn5qn9jyurc27xntvdz5n7ujhdaq", "Support", 5}},
     },
     {
      15,
      {{"devault:pp2ghv9ya7fs98rvz3gzuqmen608dh6g2y5d5dxrtp", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "Community",15},
       {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "CoreDevs", 10},
       {"devault:pqws2sgc2y22x2gkcnmw72edpa0u0kscdsqp29e530", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "WebDevs/Support", 10},
       {"devault:pzgux6zlzpw45hm45fwcj5d7mf5fn7pa2ydjzx5nxw", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "BusDev/Marketing",10}}
     },
     {
      std::numeric_limits<int32_t>::max(), // change if we add more to superblock change
      {{"devault:pqqqaf843zj992fkqr483zptyp0r8kfg7g5enjt05d", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "Community",15},
       {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "CoreDevs", 10},
       {"devault:prgtl5yust3v2m76c3t4vsnuwuufaht0euqm2smja2", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "WebDevs/Support", 10},
       {"devault:pz3htku32554kntjvzpwp8nuhmksvp73f5wwmxts8h", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "BusDev/Marketing",10}}
     }
     };
     
// Get Array Size at Compile time for Loops
const int ChangeSize = sizeof(Payouts) / sizeof(Payouts[0]);

// Reward Address & Percentage
int getPayoutIndexFromHeight(int SuperBlockNumber) {

    int i=0;
    for (i=0;i<ChangeSize;i++) {
        if (Payouts[i].changeSuperBlock > SuperBlockNumber) break;
    }
    return i;
}
       

bool CBudget::Validate(const CBlock &block, int nHeight, const Amount &BlockReward, Amount& nSumReward) {

  if (!IsSuperBlock(nHeight)) return true;

  Amount refRewards = CalculateSuperBlockRewards(nHeight, BlockReward);
  int Index = getPayoutIndexFromHeight(nHeight/nBlocksPerPeriod);
  
  // Just Log during Validation
  for (int i = 0; i < BudgetSize; i++) {
    if (fTestNet) {
      LogPrintf("%s: budget payment to %s (%s) for %s COINs\n", __func__, Payouts[Index].budget[i].Purpose,
                Payouts[Index].budget[i].TestNetAddress, nPayment[i].ToString() );
    } else {
      LogPrintf("%s: budget payment to %s (%s) for %s COINs\n", __func__, Payouts[Index].budget[i].Purpose,
                Payouts[Index].budget[i].MainNetAddress, nPayment[i].ToString() );
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
  int Index = getPayoutIndexFromHeight(nHeight/nBlocksPerPeriod);
  BudgetSize = Payouts[Index].budget.size();
  int PerCentSum = 0;
  for (const auto& p : Payouts[Index].budget) PerCentSum += p.percent;
  int ScaleFactor = (100-PerCentSum);
  
  // Overall Reward should be an integer - so divide by COIN and then re-mulitply
  Amount sumRewards;
  for (int i = 0; i < BudgetSize; i++) {
    nPayment[i] = (((Payouts[Index].budget[i].percent * nBlocksPerPeriod * nOverallReward.toInt()) / (ScaleFactor *  COIN.toInt())) * COIN);
    sumRewards += nPayment[i];
  }
  return sumRewards;
}

CBudget::CBudget(const Config &config) {
  pChainparams = &config.GetChainParams();
  nBlocksPerPeriod = (pChainparams->GetConsensus().nBlocksPerYear / 12);
  fTestNet = (pChainparams->NetworkIDString() != "main");
}

bool CBudget::IsSuperBlock(int nBlockHeight) {
  if (pChainparams->GetConsensus().IsSuperBlock(nBlockHeight)) {
    SetupForHeight(nBlockHeight);
    return true;
  } else {
    return false;
  }
}

void CBudget::SetupForHeight(int nHeight) {
  int Index = getPayoutIndexFromHeight(nHeight/nBlocksPerPeriod);
  BudgetSize = Payouts[Index].budget.size();
  nPayment = std::make_unique<Amount[]>(BudgetSize);
  Scripts = std::make_unique<CScript[]>(BudgetSize);
  for (int i = 0; i < BudgetSize; i++) {
      if (fTestNet) {
          Scripts[i] = GetScriptForDestination(DecodeDestination(Payouts[Index].budget[i].TestNetAddress, *pChainparams));
      } else {
          Scripts[i] = GetScriptForDestination(DecodeDestination(Payouts[Index].budget[i].MainNetAddress, *pChainparams));
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
