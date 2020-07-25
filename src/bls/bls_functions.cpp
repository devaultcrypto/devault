// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bls/bls.hpp"
#include "bls/privatekey.hpp"
#include "bls/schemes.hpp"
#include "bls/hdkeys.hpp"
#include <bls/bls_functions.h>
#include "util/strencodings.h"
#include <cstring>
//#include <dstencode.h>
//#include <script/script.h>

namespace bls {

CKey GetBLSPrivateKey(const uint8_t *seed, size_t seedLen, uint32_t childIndex) {
  PrivateKey master = PrivateKey::FromSeed(seed, seedLen);
  PrivateKey child = HDKeys::DeriveChildSk(master, childIndex);
  auto calculatedChild = child.Serialize();
  CKey k;
  k.Set(calculatedChild.begin(),calculatedChild.end());
  return k;
}
  
CKey GetBLSChild(const CKey& key, uint32_t childIndex) {
  PrivateKey master = PrivateKey::FromBytes(key.begin());
  PrivateKey child = HDKeys::DeriveChildSk(master, childIndex);
  auto calculatedChild = child.Serialize();
  CKey k;
  k.Set(calculatedChild.begin(),calculatedChild.end());
  return k;
}
  
CKey GetBLSMasterKey(const uint8_t *seed, size_t seedLen) {
  PrivateKey master = PrivateKey::FromSeed(seed, seedLen);
  auto bytes = master.Serialize();
  CKey k;
  k.Set(bytes.begin(),bytes.end());
  return k;
}

bool SignBLS(const CKey &key, const uint256 &hash, std::vector<uint8_t> &vchSig) {
  auto PK = bls::PrivateKey::FromBytes(key.begin());
  std::vector<uint8_t> message(hash.begin(),hash.end());
  vchSig = AugSchemeMPL::Sign(PK, message);
  // Then Verify
  return true; // for now True - sig.Verify();
}
bool SignBLS(const CKey& key, const std::vector<uint8_t> &message, std::vector<uint8_t> &vchSig) {
  auto PK = bls::PrivateKey::FromBytes(key.begin());
  vchSig = AugSchemeMPL::Sign(PK, message);
  // Then Verify
  return true; // for now True - sig.Verify();
}

auto SignBLS(const CKey &key, const uint256 &hash) -> std::optional<std::vector<uint8_t>> {
  auto PK = bls::PrivateKey::FromBytes(key.begin());
  std::vector<uint8_t> message(hash.begin(),hash.end());
  auto vchSig = AugSchemeMPL::Sign(PK, message);
  return vchSig;
}
  
bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t *vch) {
  std::vector<uint8_t> v(bls::PublicKey::PUBLIC_KEY_SIZE);
  for (size_t i=0;i<v.size();i++) v[i] = vch[i];
  std::vector<uint8_t> message(hash.begin(),hash.end());
  return AugSchemeMPL::Verify(v,message,vchSig);
}

CPubKey GetBLSPublicKey(const CKey &key) {
  bls::BLS::AssertInitialized();
  bls::PrivateKey priv;
  try {
    priv = bls::PrivateKey::FromBytes(key.begin());
  } catch (...) { throw std::runtime_error("Problem creating bls private key"); }
  try {
    // Get PublicKey and then Serialize bytes to CPubKey
    auto pub = AugSchemeMPL::SkToPk(priv);
    CPubKey k(pub);
    return k;
  } catch (...) { throw std::runtime_error("Problem creating bls public key"); }
  return (CPubKey());
}

std::vector<uint8_t> Aggregate(std::vector<std::vector<uint8_t>> &vSigs) {
  auto aggSig = AugSchemeMPL::Aggregate(vSigs);
  return aggSig;
}

  
//--------------------------------------------------------------------------------------------------
// Convert from std::vector<uint8_t> , aggregate and convert back
std::vector<uint8_t> AggregatePubKeys(std::vector<std::vector<uint8_t>> &vPubKeys) {
  if (vPubKeys.size() == 1) return vPubKeys[0];
  std::vector<bls::PublicKey> pubkeys;
  for (size_t i = 0; i < vPubKeys.size(); i++) {
    bls::PublicKey p = bls::PublicKey::FromBytes(vPubKeys[i].data()); // check
    pubkeys.push_back(p);
  }

  bls::PublicKey aggPubKey = bls::PublicKey::Aggregate(pubkeys);
  return aggPubKey.Serialize();
}

// aggregate and convert to std::vector<uint8_t>
std::vector<uint8_t> AggregateSigForMessages(std::map<uint256, CKey> &keys_plus_hash) {
  bool sigsok = true;
  std::vector<std::vector<uint8_t>> sigs;
  for (const auto &kph : keys_plus_hash) {
    std::vector<uint8_t> Sig;
    if (SignBLS(kph.second, kph.first, Sig))
      sigs.push_back(Sig);
    else
      sigsok = false;
  }
  if (!sigsok) return std::vector<uint8_t>();

  std::vector<uint8_t> aggSig = AugSchemeMPL::Aggregate(sigs);
  return aggSig;
}

  
bool VerifySigForMessages(const std::vector<uint256> &msgs, const std::vector<uint8_t> &aggSigs,
                          const std::vector<std::vector<uint8_t>> &pubkeys) {

  std::vector<std::vector<uint8_t>> messages;
  for (const auto& m : msgs) {
    std::vector<uint8_t> ms(m.begin(),m.end());
    messages.push_back(ms);
  }
  bool ok = false;
  try {
    ok = AugSchemeMPL::AggregateVerify(pubkeys, messages, aggSigs);
  }
  catch (...) { ; }
  return ok;
}
  
bool VerifySigForMessages(const std::vector<std::vector<uint8_t>> &msgs, const std::vector<uint8_t> &aggSigs,
                         const std::vector<std::vector<uint8_t>> &pubkeys) {

  return AugSchemeMPL::AggregateVerify(pubkeys, msgs, aggSigs);
}

std::vector<uint8_t> MakeAggregateSigsForMessages(const std::vector<uint256> &msgs,
                                                  const std::vector<std::vector<uint8_t>> &aggSigs,
                                                  const std::vector<std::vector<uint8_t>> &pubkeys) {

  auto Agg = AugSchemeMPL::Aggregate(aggSigs);
  return Agg;
}
    
} // namespace bls
