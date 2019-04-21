// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// No longer dependent on txdb.h, but some stuff copied/used

#pragma once
#include "coins.h"

static const char DB_REWARD = 'R';

// This is the Key for CColdReward DB iterator
struct CRewardKey {
  COutPoint *outpoint;
  char key;
  explicit CRewardKey(const COutPoint *ptr) : outpoint(const_cast<COutPoint *>(ptr)), key(DB_REWARD) {}

  template <typename Stream> void Serialize(Stream &s) const {
    s << key;
    s << outpoint->GetTxId();
    s << VARINT(outpoint->GetN());
  }

  template <typename Stream> void Unserialize(Stream &s) {
    s >> key;
    uint256 id;
    s >> id;
    uint32_t n = 0;
    s >> VARINT(n);
    *outpoint = COutPoint(id, n);
  }
};


// This is the Value used for CColdReward DB 
struct CRewardValue {
  Coin coin;
  uint32_t height;
  char key;

  uint64_t GetHeight() { return height;}
  void SetHeight(uint32_t h) { height = h;}
  bool was_paid() { return (GetHeight() != coin.GetHeight()); }
  CRewardValue() : height(0), key(DB_REWARD) {}
  explicit CRewardValue(const CTxOut& ptr, uint32_t OldHeight, uint32_t NewHeight) : coin(ptr, OldHeight, false), height(NewHeight), key(DB_REWARD) {}

  template <typename Stream> void Serialize(Stream &s) const {
    s << key;
    s << coin;
    s << height;
  }

  template <typename Stream> void Unserialize(Stream &s) {
    s >> key;
    s >> coin;
    s >> height;
  }
};

