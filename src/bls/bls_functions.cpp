// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock

#include <bls/bls_functions.h>
#include "bls/bls.hpp"
#include "bls/privatekey.hpp"
#include "bls/signature.hpp"
#include <cstring>
#include "util/strencodings.h"
namespace bls {

bool SignBLS(const CKey &key, const uint256 &hash, std::vector<uint8_t> &vchSig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  bls::Signature sig = PK.SignPrehashed(hash.begin());
  uint8_t sigBytes[bls::Signature::SIGNATURE_SIZE]; // 96 byte array
  sig.Serialize(sigBytes);
  vchSig.resize(bls::Signature::SIGNATURE_SIZE);
  for (size_t i = 0; i < bls::Signature::SIGNATURE_SIZE; i++)
    vchSig[i] = sigBytes[i];
  // Then Verify
  return sig.Verify();
}

auto SignBLS(const CKey &key, const uint256 &hash) -> std::optional<std::vector<uint8_t>>  {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  bls::Signature sig = PK.SignPrehashed(hash.begin());
  uint8_t sigBytes[bls::Signature::SIGNATURE_SIZE]; // 96 byte array
  sig.Serialize(sigBytes);
  bool ok = sig.Verify();
  if (!ok) return std::nullopt;
  std::vector<uint8_t> vchSig(bls::Signature::SIGNATURE_SIZE);
  for (size_t i = 0; i < bls::Signature::SIGNATURE_SIZE; i++) vchSig[i] = sigBytes[i];
  return vchSig;
}

bool SignBLS(const CKey &key, const uint256 &hash, Signature& sig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  sig = PK.SignPrehashed(hash.begin());
  // Then Verify
  return sig.Verify();
}
/*
bool SignBLSMessage(const CKey &key, const std::vector<uint8_t> &message, Signature& sig) {
  auto PK = bls::PrivateKey::FromSeed(key.begin(), PrivateKey::PRIVATE_KEY_SIZE);
  sig = PK.Sign(message.data(),message.size());
  // Then Verify
  return sig.Verify();
}
*/
bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t *vch) {

  bls::Signature sig = bls::Signature::FromBytes(vchSig.data());
  // Add information required for verification, to sig object

  // Unserialize from CPubKey back to PublicKey
  bls::PublicKey Pk = bls::PublicKey::FromBytes(vch);

  sig.SetAggregationInfo(bls::AggregationInfo::FromMsgHash(Pk, hash.begin()));

  bool ok = sig.Verify();
  return ok;
}

bool VerifyBLS(const uint256 &hash, Signature& sig, const uint8_t *vch) {

  // Unserialize from CPubKey back to PublicKey
  bls::PublicKey Pk = bls::PublicKey::FromBytes(vch);
  sig.SetAggregationInfo(bls::AggregationInfo::FromMsgHash(Pk, hash.begin()));
  bool ok = sig.Verify();
  return ok;
}

    
// aggregate and convert to std::vector<uint8_t>
std::vector<uint8_t> AggregateSig(std::vector<CKey>& keys, const uint256& hash) {
  bool sigsok = true;
  std::vector<Signature> sigs;
  std::vector<uint8_t> aggSigsAsBytes;
  for (size_t i=0;i<keys.size();i++) {
      bls::Signature Sig;
      if (SignBLS(keys[i], hash, Sig)) sigs.push_back(Sig);
      else sigsok = false;
  }
  if (!sigsok) return std::vector<uint8_t>();
  try {
      Signature aggSig = Signature::Aggregate(sigs);
      aggSigsAsBytes = aggSig.Serialize();
  } catch (...) { throw std::runtime_error("Problem aggregating signatures"); }
  return aggSigsAsBytes;
}

   
bool VerifyAggregate(const uint256& hash,
                     const std::vector<uint8_t> &aggSig, 
                     const std::vector<uint8_t> &aggPubKey) {
  // Unserialize from CPubKey back to PublicKey
  bls::PublicKey Pk = bls::PublicKey::FromBytes(aggPubKey.data());
  bls::Signature Sig = bls::Signature::FromBytes(aggSig.data());
  Sig.SetAggregationInfo(bls::AggregationInfo::FromMsgHash(Pk, hash.begin()));
  bool ok = Sig.Verify();
  return ok;
}

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

//--------------------------------------------------------------------------------------------------

// aggregate and convert to std::vector<uint8_t>
std::vector<uint8_t> AggregateSigForMessages(std::vector<CKey>& keys, const std::vector<uint256>& hash) {
  bool sigsok = true;
  assert(keys.size() == hash.size());
  std::vector<Signature> sigs;
  std::vector<uint8_t> aggSigsAsBytes;
  for (size_t i=0;i<keys.size();i++) {
      bls::Signature Sig;
      if (SignBLS(keys[i], hash[i], Sig)) sigs.push_back(Sig);
      else sigsok = false;
  }
  if (!sigsok) return std::vector<uint8_t>();
  try {
      Signature aggSig = Signature::Aggregate(sigs); 
      aggSigsAsBytes = aggSig.Serialize();
  } catch (...) { throw std::runtime_error("Problem aggregating signatures"); }
  return aggSigsAsBytes;
}
    
// aggregate and convert to std::vector<uint8_t>
std::vector<uint8_t> AggregateSigForMessages(std::map<uint256,CKey>& keys_plus_hash) {
  bool sigsok = true;
  std::vector<Signature> sigs;
  std::vector<uint8_t> aggSigsAsBytes;
  for (const auto& kph : keys_plus_hash) {
      bls::Signature Sig;
      if (SignBLS(kph.second, kph.first, Sig)) sigs.push_back(Sig);
      else sigsok = false;
  }
  if (!sigsok) return std::vector<uint8_t>();
  try {
      Signature aggSig = Signature::Aggregate(sigs); 
      aggSigsAsBytes = aggSig.Serialize();
  } catch (...) { throw std::runtime_error("Problem aggregating signatures"); }
  return aggSigsAsBytes;
}
 
bool VerifySigForMessages(const std::vector<uint256>& msgs,
                          const std::vector<uint8_t> &aggSigs, 
                          const std::vector<std::vector<uint8_t>> &pubkeys) {

    auto aggSigFinal = bls::Signature::FromBytes(aggSigs.data());

    std::vector<bls::AggregationInfo> infos;
    int i=0;
    for (const auto& msg : msgs) {
        bls::PublicKey Pk = bls::PublicKey::FromBytes(pubkeys[i++].data());
        infos.push_back(bls::AggregationInfo::FromMsgHash(Pk, msg.begin()));
    }

    auto aFinal = bls::AggregationInfo::MergeInfos(infos);
    aggSigFinal.SetAggregationInfo(aFinal);
    bool ok = aggSigFinal.Verify();
    return ok;
}

    
 

    
} // namespace bls

