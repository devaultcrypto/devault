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

  for (auto &tx : block.vtx) {

    // Will will ignore coinbase rewards. i.e. miner rewards and cold rewards as basis for rewards

    if (!tx->IsCoinBase()) {
      // Loop through inputs
      auto TxId = tx->GetId();

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
                     nHeight, coinr.coin.GetTxOut().nValue / COIN);
          }
          if (!pdb->EraseCoin(outpoint)) {
            LogPrint(BCLog::COLD, "%s : Problem erasing from Rewards db at Height %d, value %d\n", __func__, nHeight,
                     coinr.coin.GetTxOut().nValue / COIN);
          }
          db_change = true;
          // viable_utxos--;
        }
      }

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
          CRewardValue e(out, nHeight, nHeight);
          if (!pdb->PutCoinWithHeight(outpoint, e)) {
            LogPrint(BCLog::COLD, "%s : Problem Writing to Rewards db, value of %d at Height %d\n", __func__,
                     balance / COIN, nHeight);
          }
          db_change = true;
        }
        n++;
      }
    }
  }

  if (db_change) pdb->Flush();

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
  CRewardValue new_reward;
  // int64_t count = 0;
  COutPoint key;
  COutPoint minKey;
  int minHeight = Height;
  int HeightDiff = 0;
  Amount balance;
  Amount minReward;
  bool found = false;
  std::unique_ptr<CRewardsViewDBCursor> pcursor(pdb->Cursor());
  while (pcursor->Valid()) {
    interruption_point(ShutdownRequested());
    if (!pcursor->GetKey(key)) { break; }
    if (!pcursor->GetValue(the_reward)) { LogPrint(BCLog::COLD, "%s: cannot parse CCoins record", __func__); }

    int nHeight = the_reward.GetHeight();
    //
    // LogPrint(BCLog::COLD, " Got coin # %d, with balance = %d COINS\n", count++, the_reward.GetTxOut().nValue / COIN);

    // get Height (last reward)
    if (nHeight < minHeight) {
      HeightDiff = Height - nHeight;
      if (HeightDiff > nMinBlocks) {
        balance = the_reward.coin.GetTxOut().nValue;
        Amount reward = CalculateReward(consensusParams, Height, HeightDiff, balance);
        LogPrint(BCLog::COLD, "%s: Got coin from Height %d, with balance = %d COINS, Height Diff = %d, reward = %d\n",
                 __func__, nHeight, the_reward.coin.GetTxOut().nValue / COIN, HeightDiff, reward / COIN);
        // Check reward amount to make sure it's > min
        if (reward > Amount()) {
          // This is the oldest unrewarded UTXO with a + reward value
          minHeight = nHeight;
          minKey = key;
          if (reward < nMaxReward)
            minReward = reward;
          else
            minReward = nMaxReward;
          new_reward = the_reward;
          found = true;
          LogPrint(BCLog::COLD, "%s : *** Reward candidate for Height %d, bal = %d, HeightDiff = %d, Reward = %d\n",
                   __func__, nHeight, the_reward.coin.GetTxOut().nValue / COIN, HeightDiff, reward / COIN);
        }
      }
    }
    //
    pcursor->Next();
  }

  if (found) {
    // Use this coin
    rewardPayment = GetPayment(new_reward, minReward);
    rewardKey = minKey;
  }
  return found;
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
           coinreward.coin.GetTxOut().nValue / COIN, coinreward.coin.GetHeight());
  pdb->EraseCoin(rewardKey);
  LogPrint(BCLog::COLD, "%s : Putting Coin with new Height = %d\n", __func__, nNewHeight);
  // Get Height of last payment and shift into Coin
  CRewardValue newReward(coinreward.coin.GetTxOut(), coinreward.GetHeight(), nNewHeight);
  pdb->PutCoinWithHeight(rewardKey, newReward);
}

// Create CTxOut based on coin and reward
//
CTxOut CColdRewards::GetPayment(const CRewardValue &coinreward, Amount reward) {
  CTxOut out = CTxOut(reward, coinreward.coin.GetTxOut().scriptPubKey);
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
      CTxOut coinbase_reward = txCoinbase->vout[1]; // 1???
      return (out == coinbase_reward);
    } else {
      // Coinbase has Reward but FindReward can't find it
      return false;
    }
    // Validate didn't find reward, so Coinbase size should be 1
  } else {
    reward = Amount();
    return (size == 1);
  }
}
