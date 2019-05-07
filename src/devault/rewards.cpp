// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "devault/rewards.h"
#include "amount.h"
#include "chain.h"
#include "cashaddrenc.h" // GetAddrFromTxOut for debug
#include "chainparams.h"
#include "config.h"
#include "consensus/consensus.h"
#include "devault/rewards_calculation.h"
#include "init.h" // for Shutdown
#include "logging.h"
#include "script/standard.h"
#include "validation.h"

#include "fs.h" // for Dump debug stuff
#include <fstream>

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
  nMinReward = consensusParams.nMinReward;
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
          LogPrint(BCLog::COLD, "CR: %s : Writing to Rewards db, addr %s, Value of %d at Height %d\n", __func__,
                   GetAddrFromTxOut(out), balance / COIN, nHeight);
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
        if (pdb->HaveReward(outpoint)) {
          // will erase
          // first get Value for now
          CRewardValue coinr; // Don't need this expect for log/checking
          if (!pdb->GetReward(outpoint, coinr)) {
              LogPrint(BCLog::COLD, "CR: %s : Problem getting coin from Rewards db at Height %d, value %d\n", __func__,
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
        if (prewardsdb->HaveReward(out)) {
          CRewardValue val;
          prewardsdb->GetReward(out, val);
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
        // LogPrintf("Found spendcOINS at height %d\n", balance/COIN, nHeight);
        COutPoint outpoint(TxId, n); // Unique

        // 2. means possibly new candidates that should be removed by DB
        if (prewardsdb->HaveReward(outpoint)) {
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
    if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "CR: %s: cannot parse CCoins record", __func__); }

    int nHeight = the_reward.GetHeight();

    // get Height (last reward)
    if (the_reward.IsActive()) {
      if (nHeight <= minHeight) { // same Height OK to check for bigger rewards
        HeightDiff = Height - nHeight;
        if (HeightDiff > nMinBlocks) { 
          balance = the_reward.GetValue();
          Amount reward = CalculateReward(consensusParams, Height, HeightDiff, balance);
          //LogPrint(BCLog::COLD, "%s: Got coin from Height %d, with balance = %d COINS, Height Diff = %d(%d), reward = %d\n",
            //       __func__, nHeight, the_reward.GetValue() / COIN, HeightDiff, Height, reward / COIN);
          // Check reward amount to make sure it's > min
          if (reward >= nMinReward) {
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
                //LogPrint(BCLog::COLD, "CR: %s : Candidate : %s, Reward %d\n", __func__, coinreward.ToString(), reward);
            } else if (minHReward == selAmount) {
                // Same reward at Same Height => Select the 'smallest' key
                if (key < minKey) {
                    selAmount = minHReward;
                    minHeight = nHeight;
                    minKey = key;
                    sel_reward = the_reward;
                    //LogPrint(BCLog::COLD, "CR: %s : Candidate : %s, Reward %d\n", __func__, coinreward.ToString(), reward);
                }
            }
            found = true;
          }
        }
      }
    } else {
      // Not Active and Sufficiently old not to be re-org back,
      // therefore delete from dB
      if ((Height - nHeight) > maxreorgdepth) {
        pdb->EraseReward(key);
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
    if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "CR: %s: cannot parse CCoins record", __func__); }

    int nHeight = the_reward.GetHeight();
    if (nHeight == Height) { // Bingo!
      // Restore previous height
      the_reward.SetHeight(the_reward.GetOldHeight());
      the_reward.payCount--;
      pdb->PutReward(key, the_reward);
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
  pdb->GetReward(rewardKey, coinreward);
  // Should have erased coin with balance/height
  LogPrint(BCLog::COLD, "CR: %s : %s\n", __func__, coinreward.ToString());
  pdb->EraseReward(rewardKey);
  // Get Height of last payment and shift into Coin
  CRewardValue newReward(coinreward);
  newReward.SetOldHeight(newReward.GetHeight()); // Move Height of creation Height or last payment to OldHeight
  newReward.SetHeight(nNewHeight);
  newReward.payCount++;
  pdb->PutReward(rewardKey, newReward);
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
      if (!valid) LogPrintf("ERROR: Cold Reward invalid since TxOut doesn't match,\n coinbase(%s : %d)\n reward  (%s : %d)\n", GetAddrFromTxOut(coinbase_reward),coinbase_reward.nValue, GetAddrFromTxOut(out),out.nValue);
      return valid;
    } else {
      LogPrintf("ERROR: Cold Reward invalid coinbase size ! > 1, while reward = %d\n\n", reward/COIN);
      // Coinbase has Reward but FindReward can't find it
      return false;
    }
    // Validate didn't find reward, so Coinbase size should be 1
  } else {
    reward = Amount();
    bool valid = (size == 1);
    if (!valid) LogPrintf("ERROR: Cold Reward invalid since no Reward found but size != 1 (reward in coinbase)\n");
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
      if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "CR: %s: cannot parse CCoins record", __func__); }
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
      if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "CR: %s: cannot parse CCoins record", __func__); }
      vals.push_back(the_reward);
      pcursor->Next();
  }
  
  return vals;
}

void CColdRewards::DumpOrderedRewards(const std::string& filename) {

    fs::path filepath;
    if (filename == "") {
        std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        filepath = fs::absolute("rewarddb-"+FormatDebugLogDateTime(cftime)+".log");
    } else {
        filepath = fs::absolute(filename);
    }
    std::ofstream file;
    file.open(filepath.string().c_str());

    CRewardValue val;
    COutPoint key;
  
    std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
    while (pcursor->Valid()) {
        if (!pcursor->GetKey(key)) { break; }
        if (!pcursor->GetValue(val)) { LogPrint(BCLog::COLD, "CR: %s: cannot parse CCoins record", __func__); }        
        file << "Height " << val.GetHeight() << " ";
        file << "payCount " << val.GetPayCount() << " ";
        file << "active "<<  val.IsActive() << " ";
        file << "creationHeight " << val.GetCreationHeight() << " ";
        file << "OldHeight " << val.GetOldHeight() << " ";
        file << "Addr: " << GetAddrFromTxOut(val.GetTxOut()) << " ";
        file << "n " << key.GetN() << " ";
        file << "Value " << val.GetValue()/COIN << "\n";
        pcursor->Next();
    }
  
    file.close();
}
