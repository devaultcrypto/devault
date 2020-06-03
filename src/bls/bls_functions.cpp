// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock

#include "bls/bls.hpp"
#include "bls/privatekey.hpp"
#include "bls/signature.hpp"
#include "bls/schemes.hpp"
#include "util/strencodings.h"
#include <bls/bls_functions.h>
#include <cstring>
//#include <dstencode.h>
//#include <script/script.h>

namespace bls {

bool SignBLS(const CKey &key, const uint256 &hash, std::vector<uint8_t> &vchSig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  std::vector<uint8_t> message(hash.begin(),hash.end());
  vchSig = AugScheme::Sign(PK, message);
  // Then Verify
  return true; // for now True - sig.Verify();
}
bool SignBLS(const CKey& key, const std::vector<uint8_t> &message, std::vector<uint8_t> &vchSig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  vchSig = AugScheme::Sign(PK, message);
  // Then Verify
  return true; // for now True - sig.Verify();
}

auto SignBLS(const CKey &key, const uint256 &hash) -> std::optional<std::vector<uint8_t>> {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  std::vector<uint8_t> message(hash.begin(),hash.end());
  auto vchSig = AugScheme::Sign(PK, message);
  return vchSig;
}
  
bool SignBLS(const CKey &key, const uint256 &hash, Signature &sig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  std::vector<uint8_t> message(hash.begin(),hash.end());
  auto vchSig = AugScheme::Sign(PK, message);
  sig = bls::Signature::FromBytes(vchSig.data());
  return true; // for now
}
  
bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t *vch) {

  auto pub = bls::PublicKey::FromBytes(vch); //???
  auto pubkey = pub.Serialize();
  std::vector<uint8_t> message(hash.begin(),hash.end());
  return AugScheme::Verify(pubkey,message,vchSig);
  
}

CPubKey GetBLSPublicKey(const CKey &key) {
  bls::BLS::AssertInitialized();
  bls::PrivateKey priv;
  try {
    priv = bls::PrivateKey::FromSeed(key.begin(), bls::PrivateKey::PRIVATE_KEY_SIZE);
  } catch (...) { throw std::runtime_error("Problem creating bls private key"); }
  try {
    // Get PublicKey and then Serialize bytes to CPubKey
    bls::PublicKey pub = priv.GetPublicKey();
    auto b = pub.Serialize();
    CPubKey k(b);
    return k;
  } catch (...) { throw std::runtime_error("Problem creating bls public key"); }
  return (CPubKey());
}

std::vector<uint8_t> Aggregate(std::vector<std::vector<uint8_t>> &vSigs) {
  auto aggSig = AugScheme::Aggregate(vSigs);
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

  std::vector<uint8_t> aggSig = AugScheme::Aggregate(sigs);
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
    ok = AugScheme::AggregateVerify(pubkeys, messages, aggSigs);
  }
  catch (...) { ; }
  return ok;
}
  
bool VerifySigForMessages(const std::vector<std::vector<uint8_t>> &msgs, const std::vector<uint8_t> &aggSigs,
                         const std::vector<std::vector<uint8_t>> &pubkeys) {

  return AugScheme::AggregateVerify(pubkeys, msgs, aggSigs);
}

std::vector<uint8_t> MakeAggregateSigsForMessages(const std::vector<uint256> &msgs,
                                                  const std::vector<std::vector<uint8_t>> &aggSigs,
                                                  const std::vector<std::vector<uint8_t>> &pubkeys) {

  auto Agg = AugScheme::Aggregate(aggSigs);
  return Agg;
}
    
} // namespace bls
