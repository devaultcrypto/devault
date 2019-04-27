// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "devault/rewards.h"
#include "amount.h"
#include "chain.h"
#include "chainparams.h"
#include "config.h"
#include "consensus/consensus.h"
#include "devault/rewards_calculation.h"
#include "init.h" // for Shutdown
#include "logging.h"
#include "script/standard.h"
#include "validation.h"

using namespace std;

CCriticalSection cs_rewardsdb;

// Probably should be a shared function
bool IsSuperBlock(int nBlockHeight) {
  int64_t nBlocksPerPeriod = (GetConfig().GetChainParams().GetConsensus().nBlocksPerYear / 12);
  return (nBlockHeight % nBlocksPerPeriod == 0);
}

//

CColdRewards::CColdRewards(const Consensus::Params &consensusParams, CRewardsViewDB *prdb) : pdb(prdb) {
  LOCK(cs_rewardsdb);
  Setup(consensusParams);
  // viable_utxos = 0;
}

void CColdRewards::Setup(const Consensus::Params &consensusParams) {
  nMinBlocks = consensusParams.nMinRewardBlocks;
  nMinBalance = consensusParams.nMinRewardBalance;
  nMaxReward = consensusParams.nMaxReward;
}

bool CColdRewards::UpdateWithBlock(const Config &config, CBlockIndex *pindexNew) {

  const Consensus::Params consensusParams = config.GetChainParams().GetConsensus();
  bool db_change = false;
  CBlock block;
  int nHeight = pindexNew->nHeight;
  ReadBlockFromDisk(block, pindexNew, config);

  // Loop through block
  std::vector<std::pair<COutPoint, CRewardValue>> rewardAdditions;
  std::vector<std::pair<COutPoint, CRewardValue>> rewardErasures;

  for (auto &tx : block.vtx) {
    // Will will ignore coinbase rewards. i.e. miner rewards and cold rewards as basis for rewards
    if (!tx->IsCoinBase()) {
      auto TxId = tx->GetId();
      int n = 0;
      // Loop through outputs
      for (const CTxOut &out : tx->vout) {
        // Add a new entry for each output into database with current height, etc if value > min
        Amount balance = out.nValue;
        // LogPrintf("Found spend to %d COINS at height %d\n", balance/COIN, nHeight);
        COutPoint outpoint(TxId, n); // Unique
        if (balance >= nMinBalance) {
          LogPrint(BCLog::COLD, "%s : Writing to Rewards db, value of %d at Height %d\n", __func__, balance / COIN,
                   nHeight);
          CRewardValue e(out, nHeight, nHeight, nHeight);
          rewardAdditions.emplace_back(outpoint, e);
          db_change = true;
        }
        n++;
      }
    }
  }

  for (auto &tx : block.vtx) {
    if (!tx->IsCoinBase()) {
      // Loop through inputs
      for (const CTxIn &in : tx->vin) {
        // Since input is a previous output, delete it from the database if it's in there
        // could be if > nMinBalance, but don't need to check
        COutPoint outpoint(in.prevout);
        if (pdb->HaveCoin(outpoint)) {
          // will erase
          // first get Value for now
          CRewardValue coinr; // Don't need this expect for log/checking
          if (!pdb->GetCoinWithHeight(outpoint, coinr)) {
              LogPrint(BCLog::COLD, "%s : Problem getting coin from Rewards db at Height %d, value %d\n", __func__,
                       nHeight, coinr.GetValue() / COIN);
          }
          rewardErasures.emplace_back(outpoint, coinr);
          db_change = true;
          // viable_utxos--;
        }
        
        // If Input is an output in this same block, remove it from Additions array also
        for (auto it = rewardAdditions.begin(); it != rewardAdditions.end(); ) {
          if (it->first == outpoint) {
            rewardAdditions.erase(it);
            break;
          }
          it++;
        }
      }
    }
  }
  
  // Batch Write/Erase
  if (rewardAdditions.size() > 0) pdb->Add(rewardAdditions);
  if (rewardErasures.size() > 0) pdb->InActivate(rewardErasures);

  return true;
}

bool CColdRewards::UndoBlock(const CBlock &block, const CBlockIndex *pindex, bool undoReward) {

  int nHeight = pindex->nHeight;

  // Loop through block
  std::vector<std::pair<COutPoint, CRewardValue>> rewardUpdates;
  std::vector<COutPoint> rewardRemovals;
  
  for (auto &tx : block.vtx) {
    if (!tx->IsCoinBase()) {
      // Loop through inputs
      for (const CTxIn &in : tx->vin) {
        // Since input is a previous output, delete it from the database if it's in there
        // could be if > nMinBalance, but don't need to check
        COutPoint out(in.prevout);
        // Inputs for Rewards means that they'd become invalid due to being spent
        // so we must revert this 
        if (prewardsdb->HaveCoin(out)) {
          CRewardValue val;
          prewardsdb->GetCoinWithHeight(out, val);
          // if inactive, make inactive
          if (!val.IsActive()) {
            val.SetActive(true);
          } else {
            // if active, restore height
            val.SetHeight(val.GetOldHeight());
          }
          rewardUpdates.emplace_back(out,val);
        }
      }
    }
  }
  

  for (auto &tx : block.vtx) {
    // Will will ignore coinbase rewards. i.e. miner rewards and cold rewards as basis for rewards
    if (!tx->IsCoinBase()) {
      auto TxId = tx->GetId();
      int n = 0;
      // Loop through outputs
      
      for (const CTxOut &out : tx->vout) {
        // Add a new entry for each output into database with current height, etc if value > min
        Amount balance = out.nValue;
        // LogPrintf("Found spend to %d COINS at height %d\n", balance/COIN, nHeight);
        COutPoint outpoint(TxId, n); // Unique

        // 2. means possibly new candidates that should be removed by DB
        if (prewardsdb->HaveCoin(outpoint)) {
          rewardRemovals.push_back(outpoint);
        }            
        n++;
      }
    } else {
      // Coinbase. Size > 1 => Cold Reward if not Superblock
      if ((tx->vout.size() > 1) && !IsSuperBlock(nHeight) && undoReward) {
          // coinbase, but need to rewind the last paid height
          bool ok = RestoreRewardAtHeight(nHeight);
          if (!ok) {
            // Never found in DB!!!!
          }
        }
    }
  }
  
  // Batch Write/Erase
  if (rewardUpdates.size() > 0) pdb->Add(rewardUpdates);
  if (rewardRemovals.size() > 0) pdb->Erase(rewardRemovals);

  return true;
}

// Fill the Coinbase Tx
//
void CColdRewards::FillPayments(const Consensus::Params &consensusParams, CMutableTransaction &txNew, int nHeight) {
  CTxOut out;
  if (FindReward(consensusParams, nHeight, out)) txNew.vout.push_back(out);
}

// Determine which coin gets reward and how much
//
bool CColdRewards::FindReward(const Consensus::Params &consensusParams, int Height, CTxOut &rewardPayment) {
  CRewardValue the_reward;
  CRewardValue sel_reward;
  // int64_t count = 0;
  COutPoint key;
  COutPoint minKey;
  int minHeight = Height;
  int HeightDiff = 0;
  Amount balance;
  Amount selAmount;
  Amount minHReward;
  bool found = false;
  const int32_t maxreorgdepth = gArgs.GetArg("-maxreorgdepth", DEFAULT_MAX_REORG_DEPTH);
  
  std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
  while (pcursor->Valid()) {
    interruption_point(ShutdownRequested());
    if (!pcursor->GetKey(key)) { break; }
    if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "%s: cannot parse CCoins record", __func__); }

    int nHeight = the_reward.GetHeight();

    // get Height (last reward)
    if (the_reward.IsActive()) {
      if (nHeight < minHeight) { // Later change to :  
        //if (nHeight <= minHeight) { // same Height OK to check for bigger rewards
        HeightDiff = Height - nHeight;
        if (HeightDiff > nMinBlocks) { 
          balance = the_reward.GetValue();
          Amount reward = CalculateReward(consensusParams, Height, HeightDiff, balance);
          LogPrint(BCLog::COLD, "%s: Got coin from Height %d, with balance = %d COINS, Height Diff = %d(%d), reward = %d\n",
                   __func__, nHeight, the_reward.GetValue() / COIN, HeightDiff, Height, reward / COIN);
          // Check reward amount to make sure it's > min
          if (reward > Amount()) {
            // This is one of the oldest unrewarded UTXO with a + reward value
            // But there could be other valid UTXOs at same Height and same RewardValue
            if (reward < nMaxReward)
              minHReward = reward;
            else
              minHReward = nMaxReward;

            // New cases
            // Heigher reward at same Height => choose it
            if (minHReward > selAmount) {
                selAmount = minHReward;
                minHeight = nHeight;
                minKey = key;
                sel_reward = the_reward;
            } else if (minHReward == selAmount) {
                // Same reward at Same Height => Select the 'smallest' key
                if (key < minKey) {
                    selAmount = minHReward;
                    minHeight = nHeight;
                    minKey = key;
                    sel_reward = the_reward;
                }
            }
            found = true;
            LogPrint(BCLog::COLD, "%s : *** Reward candidate for Height %d, bal = %d, HeightDiff = %d(%d), Reward = %d\n",
                     __func__, nHeight, the_reward.GetValue() / COIN, HeightDiff, Height, reward / COIN);
          }
        }
      }
    } else {
      // Not Active and Sufficiently old not to be re-org back,
      // therefore delete from dB
      if ((Height - nHeight) > maxreorgdepth) {
        pdb->EraseCoin(key);
      }
    }
    //
    pcursor->Next();
  }

  if (found) {
    // Use this coin
    rewardPayment = GetPayment(sel_reward, selAmount);
    rewardKey = minKey;
  }
  return found;
}

bool CColdRewards::RestoreRewardAtHeight(int Height) {
  CRewardValue the_reward;
  COutPoint key;
  
  std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
  while (pcursor->Valid()) {
    interruption_point(ShutdownRequested());
    if (!pcursor->GetKey(key)) { break; }
    if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "%s: cannot parse CCoins record", __func__); }

    int nHeight = the_reward.GetHeight();
    if (nHeight == Height) { // Bingo!
      // Restore previous height
      the_reward.SetHeight(the_reward.GetOldHeight());
      the_reward.payCount--;
      pdb->PutCoinWithHeight(key, the_reward);
      return true;
    }
    //
    pcursor->Next();
  }
  return false;
}
//
// Effectively update the "Height" for a coin
//
void CColdRewards::UpdateRewardsDB(int nNewHeight) {
  // Now re-write with new Height
  CRewardValue coinreward;
  pdb->GetCoinWithHeight(rewardKey, coinreward);
  // Should have erased coin with balance/height
  LogPrint(BCLog::COLD, "%s : Attempt to erase Coin with Value %d/Height = %d\n", __func__,
           coinreward.GetValue() / COIN, coinreward.GetOldHeight());
  pdb->EraseCoin(rewardKey);
  LogPrint(BCLog::COLD, "%s : Putting Coin with new Height = %d\n", __func__, nNewHeight);
  // Get Height of last payment and shift into Coin
  //CRewardValue newReward(coinreward.GetTxOut(), coinreward.GetHeight(), nNewHeight);
  CRewardValue newReward(coinreward);
  newReward.SetHeight(nNewHeight);
  newReward.payCount++;
  pdb->PutCoinWithHeight(rewardKey, newReward);
}

// Create CTxOut based on coin and reward
//
CTxOut CColdRewards::GetPayment(const CRewardValue &coinreward, Amount reward) {
    CTxOut out = CTxOut(reward, coinreward.txout.scriptPubKey);
    //CTxOut out = CTxOut(reward, coinreward.scriptPubKey());
    return out;
}

// Validate!

bool CColdRewards::Validate(const Consensus::Params &consensusParams, const CBlock &block, int nHeight,
                            Amount &reward) {
  auto txCoinbase = block.vtx[0];
  int size = txCoinbase->vout.size();
  // Coinbase has Cold Reward
  CTxOut out;
  // Found Reward
  if (FindReward(consensusParams, nHeight, out)) {
    reward = out.nValue;
    if (size > 1) {
      CTxOut coinbase_reward = txCoinbase->vout[1];
      bool valid = (out == coinbase_reward);
      if (!valid) LogPrintf("Cold Reward invalid since TxOut doesn't match");
      return valid;
    } else {
      LogPrintf("Cold Reward invalid coinbase size ! > 1, while reward = %d\n\n", reward/COIN);
      // Coinbase has Reward but FindReward can't find it
      return false;
    }
    // Validate didn't find reward, so Coinbase size should be 1
  } else {
    reward = Amount();
    bool valid = (size == 1);
    if (!valid) LogPrintf("Cold Reward invalid since no Reward found but size != 1 (reward in coinbase)\n");
    return valid;
  }
}
//
std::map<COutPoint, CRewardValue> CColdRewards::GetRewards() {
  std::map<COutPoint, CRewardValue> vals;
  CRewardValue the_reward;
  COutPoint key;
  
  std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
  while (pcursor->Valid()) {
      interruption_point(ShutdownRequested());
      if (!pcursor->GetKey(key)) { break; }
      if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "%s: cannot parse CCoins record", __func__); }
      vals.insert(std::make_pair(key,the_reward));
      pcursor->Next();
  }
  
  return vals;
}
std::vector<CRewardValue> CColdRewards::GetOrderedRewards() {
  std::vector<CRewardValue> vals;
  CRewardValue the_reward;
  COutPoint key;
  
  std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
  while (pcursor->Valid()) {
      interruption_point(ShutdownRequested());
      if (!pcursor->GetKey(key)) { break; }
      if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "%s: cannot parse CCoins record", __func__); }
      vals.push_back(the_reward);
      pcursor->Next();
  }
  
  return vals;
}
