// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Copy from Bitcoin/txdb.cpp

#include <devault/rewardsview.h>
#include <chainparams.h>
#include <init.h>
#include <random.h>
#include <uint256.h>
#include <fs_util.h>
#include <cstdint>

using namespace std; // for make_pair

CRewardsViewDB::CRewardsViewDB(const std::string &dbname, size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / dbname, nCacheSize, fMemory, fWipe, true) {}

bool CRewardsViewDB::Flush() {
    CDBBatch batch(db);
    bool ret = db.WriteBatch(batch, true);
    return ret;
}


bool CRewardsViewDB::Erase(const std::vector<COutPoint>& vect) {
    CDBBatch batch(db);
    for (const auto& it : vect) {
        batch.Erase(std::make_pair(DB_REWARD, it));
    }
    return db.WriteBatch(batch);
}

bool CRewardsViewDB::InActivate(std::vector<std::pair<COutPoint, CRewardValue> >& vect) {
    CDBBatch batch(db);
    for (auto& it : vect) {
      it.second.SetActive(false);
      batch.Write(std::make_pair(DB_REWARD, it.first), it.second);
    }
    return db.WriteBatch(batch);
}

bool CRewardsViewDB::Add(const std::vector<std::pair<COutPoint, CRewardValue> >& vect) {
    CDBBatch batch(db);
    for (const auto& it : vect) {
        batch.Write(std::make_pair(DB_REWARD, it.first), it.second);
    }
    return db.WriteBatch(batch);
}

CRewardsViewDBCursor *CRewardsViewDB::Cursor() const {
    CRewardsViewDBCursor *i = new CRewardsViewDBCursor(const_cast<CDBWrapper &>(db).NewIterator()); //, GetBestBlock());
  // It seems that there are no "const iterators" for LevelDB. Since we only
  // need read operations on it, use a const-cast to get around that
  // restriction.
  i->pcursor->Seek(DB_REWARD);
  // Cache key of first record
  if (i->pcursor->Valid()) {
    CRewardKey entry(&i->keyTmp.second);
    i->pcursor->GetKey(entry);
    i->keyTmp.first = entry.key;
  } else {
    // Make sure Valid() and GetKey() return false
    i->keyTmp.first = 0;
  }
  return i;
}
size_t CRewardsViewDB::EstimateSize() const { return db.EstimateSize(DB_REWARD, char(DB_REWARD + 1)); }

bool CRewardsViewDBCursor::GetKey(COutPoint &key) const {
  // Return cached key
  if (keyTmp.first == DB_REWARD) {
    key = keyTmp.second;
    return true;
  }
  return false;
}

void CRewardsViewDBCursor::Next() {
  pcursor->Next();
  CRewardKey entry(&keyTmp.second);
  if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
    // Invalidate cached key after last record so that Valid() and GetKey()
    // return false
    keyTmp.first = 0;
  } else {
    keyTmp.first = entry.key;
  }
}
