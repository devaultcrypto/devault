// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// No longer dependent on txdb.h, but some stuff copied/used

#pragma once
#include <primitives/transaction.h>

// For ToString()
#include <tinyformat.h>
#include <dstencode.h>

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
    TxId id;
    s >> id;
    uint32_t n = 0;
    s >> VARINT(n);
    *outpoint = COutPoint(id, n);
  }
};

// This is the Value used for CColdReward DB
struct CRewardValue {
  CTxOut txout;
  uint32_t creationHeight;
  uint32_t OldHeight;
  uint32_t height;
  uint32_t payCount;
  uint8_t version = 1;
  bool active;

  CScript scriptPubKey() const { return txout.scriptPubKey; }
  CTxOut &GetTxOut() { return txout; }
  Amount GetValue() const { return txout.nValue; }
  uint32_t GetCreationHeight() const { return creationHeight; }
  uint32_t GetOldHeight() const { return OldHeight; }
  uint32_t GetHeight() const { return height; }
  uint32_t GetPayCount() { return payCount; }
  uint8_t GetVersion() const { return version; }
  void SetVersion(const uint8_t nVersion) { version = nVersion; }
  void SetHeight(uint32_t h) { height = h; }
  void SetOldHeight(uint32_t h) { OldHeight = h; }
  bool was_paid() const { return (GetHeight() != GetOldHeight()); }
  bool IsActive() const { return active; }
  void SetActive(bool a) { active = a;}
  CRewardValue() : creationHeight(0), OldHeight(0), height(0), payCount(0), active(false) {}
  explicit CRewardValue(const CTxOut &ptr, uint32_t cH, uint32_t OldH, uint32_t NewH)
      : txout(ptr), creationHeight(cH), OldHeight(OldH), height(NewH), payCount(0), active(true) {}

  template <typename Stream> void Serialize(Stream &s) const {
    s << version;
    s << txout;
    s << creationHeight;
    s << OldHeight;
    s << height;
    s << active;
    s << payCount;
  }

  template <typename Stream> void Unserialize(Stream &s) {
    s >> version;
    s >> txout;
    s >> creationHeight;
    s >> OldHeight;
    s >> height;
    s >> active;
    s >> payCount;
  }
  std::string ToString() {
        return strprintf("CR(Addr : %s, Value %d, Creation : %d, Height : %d, OldHeight %d, PayCount % d, Active : %d)", 
                         GetAddrFromTxOut(GetTxOut()),
                         GetValue().ToString(),
                         GetCreationHeight(),
                         GetHeight(),
                         GetOldHeight(),
                         GetPayCount(),
                         IsActive());
  }
 
};
