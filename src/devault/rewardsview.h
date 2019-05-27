// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// No longer dependent on txdb.h, but some stuff copied/used

#pragma once
#include "coinreward.h"
#include "chain.h"
#include "config/bitcoin-config.h"
#include "dbwrapper.h"
#include "validation.h"

class CRewardsViewDBCursor {
  public:
  bool GetKey(COutPoint &key) const;
  bool GetValue(CRewardValue &coin) const { return pcursor->GetValue(coin); }
  unsigned int GetValueSize() const { return pcursor->GetValueSize(); }

  bool Valid() const { return keyTmp.first == DB_REWARD; }
  void Next();

  private:
  CRewardsViewDBCursor(CDBIterator *pcursorIn) : pcursor(pcursorIn) {}
  std::unique_ptr<CDBIterator> pcursor;
  std::pair<char, COutPoint> keyTmp;

  friend class CRewardsViewDB;
};

/** CCoinsView backed by the coin database (chainstate/) */
class CRewardsViewDB {
  protected:
  CDBWrapper db;

  public:
  explicit CRewardsViewDB(const std::string &dbname, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

  bool HaveReward(const COutPoint &outpoint) const { return db.Exists(std::pair(DB_REWARD, outpoint)); }
  bool EraseReward(const COutPoint &outpoint) { return db.Erase(std::pair(DB_REWARD, outpoint)); }

  // Batch ops
  bool Add(const std::vector<std::pair<COutPoint, CRewardValue> >& vect);
  bool InActivate(std::vector<std::pair<COutPoint, CRewardValue> >& vect, int nHeight);
  bool Erase(const std::vector<COutPoint>& vect);

  // Extra parameter Height
  bool PutReward(const COutPoint &outpoint, const CRewardValue &coin) {    return db.Write(std::pair(DB_REWARD, outpoint), coin);  }
  bool GetReward(const COutPoint &outpoint, CRewardValue &coin) const {
    return db.Read(std::pair(DB_REWARD, outpoint), coin);
  }

  CRewardsViewDBCursor *Cursor() const;

  bool Flush();
  size_t EstimateSize() const;
};
