// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <amount.h>
#include <chain.h>
#include <config/bitcoin-config.h>
#include <devault/rewardsview.h>
// for now
#include <validation.h>

extern CCriticalSection cs_rewardsdb;

class CColdRewards {

  public:
  CColdRewards(const Consensus::Params &consensusParams, CRewardsViewDB *prdb);

  private:
  CRewardsViewDB *pdb;
  COutPoint rewardKey;
  int64_t nMinBlocks;
  Amount nMaxReward;
  Amount nMinReward;
  int32_t nNumCandidates = 0; // num of reward candidates (that are active)
  bool fMainNet;
  std::map<COutPoint, int> cachedInactives; // cache map on Inactive rewards that are still needed in case of re-org

  public:
  bool UpdateWithBlock(const Config &config, CBlockIndex *pindexNew);
  void Setup(const Consensus::Params &consensusParams);

  CTxOut GetPayment(const CRewardValue &coin, Amount reward);
  bool FindReward(const Consensus::Params &consensusParams, int Height, CTxOut &out);
  void FillPayments(const Consensus::Params &consensusParams, CMutableTransaction &txNew, int nHeight);
  bool FullValidate(const Consensus::Params &consensusParams, const CBlock &block, int nHeight, Amount &reward, bool fJustCheck=0);
  bool QuickValidate(const Consensus::Params &consensusParams, const CBlock &block, int nHeight, Amount& reward, bool fJustCheck=0);
  bool CheckReward(const Consensus::Params &consensusParams, int Height, CTxOut &rewardPayment);
  void UpdateRewardsDB(int nNewHeight);
  bool UndoBlock(const CBlock &block, const CBlockIndex *pindex, bool undoReward = true);
  void ClearLowRewards(const Consensus::Params &consensusParams, const Amount min);
  
  bool RestoreRewardAtHeight(int Height);
  std::map<COutPoint, CRewardValue> GetRewards();
  std::vector<CRewardValue> GetOrderedRewards();
  void DumpOrderedRewards(const std::string &filename = "");
  void RemoveOlderDumpFile();

  int32_t GetNumberOfCandidates() { return nNumCandidates; }
  void GetInActivesFromDB(int Height);
  bool CheckReward(const std::string& r);
    
};
