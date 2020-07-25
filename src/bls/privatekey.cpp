// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstring>
#include <string>

#include "bls/bls.hpp"
#include "bls/privatekey.hpp"
#include "bls/util.hpp"
#include <crypto/hmac_sha256.h>


namespace bls {


PrivateKey PrivateKey::FromSeed(const uint8_t *seed, size_t seedLen)
{
  BLS::AssertInitialized();

  // "BLS private key seed" in ascii
  const uint8_t hmacKey[] = {66,  76, 83,  32,  112, 114, 105, 118, 97,  116,
                             101, 32, 107, 101, 121, 32,  115, 101, 101, 100};

  uint8_t *hash = SecAlloc<uint8_t>(PrivateKey::PRIVATE_KEY_SIZE);

  // Hash the seed into sk
  CHMAC_SHA256(hmacKey, sizeof(hmacKey)).Write(seed, seedLen).Finalize(hash);

  bn_t order;
  bn_new(order);
  g1_get_ord(order);
  bn_free(order);

  // Make sure private key is less than the curve order
  bn_t *skBn = SecAlloc<bn_t>(1);
  bn_new(*skBn);
  bn_read_bin(*skBn, hash, PrivateKey::PRIVATE_KEY_SIZE);
  bn_mod_basic(*skBn, *skBn, order);

  PrivateKey k;
  k.AllocateKeyData();
  bn_copy(*k.keydata, *skBn);

  SecFree(skBn);
  SecFree(hash);
  
  return k;
}

// Construct a private key from a bytearray.
PrivateKey PrivateKey::FromBytes(const uint8_t* bytes, bool modOrder) {
    PrivateKey k;
    k.AllocateKeyData();
    bn_read_bin(*k.keydata, bytes, PrivateKey::PRIVATE_KEY_SIZE);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    if (modOrder) {
        bn_mod_basic(*k.keydata, *k.keydata, ord);
    } else {
        if (bn_cmp(*k.keydata, ord) > 0) {
            throw std::invalid_argument("Key data too large, must be smaller than group order");
        }
    }
    return k;
}

PrivateKey PrivateKey::FromBN(bn_t sk) {
    PrivateKey k;
    k.AllocateKeyData();
    bn_copy(*k.keydata, sk);
    return k;
}

// Construct a private key from another private key.
PrivateKey::PrivateKey(const PrivateKey &privateKey) {
    AllocateKeyData();
    bn_copy(*keydata, *privateKey.keydata);
}

PrivateKey::PrivateKey(PrivateKey&& k) {    std::swap(keydata, k.keydata);}

PrivateKey::~PrivateKey() {    SecFree(keydata);}


G1Element PrivateKey::GetG1Element() const
{
    g1_t *p = SecAlloc<g1_t>(1);
    g1_mul_gen(*p, *keydata);

    const G1Element ret = G1Element::FromNative(p);
    SecFree(*p);
    return ret;
}

G2Element PrivateKey::GetG2Element() const
{
    g2_t *q = SecAlloc<g2_t>(1);
    g2_mul_gen(*q, *keydata);

    const G2Element ret = G2Element::FromNative(q);
    SecFree(*q);
    return ret;
}

G1Element &operator*=(G1Element &a, PrivateKey &k)
{
    g1_mul(a.p, a.p, *(k.keydata));
    return a;
}

G1Element &operator*=(PrivateKey &k, G1Element &a)
{
    a *= k;
    return a;
}

G1Element operator*(G1Element &a, PrivateKey &k)
{
    g1_t ans;
    g1_new(ans);
    g1_mul(ans, a.p, *(k.keydata));
    return G1Element::FromNative(&ans);
}

G1Element operator*(PrivateKey &k, G1Element &a) { return a * k; }

G2Element &operator*=(G2Element &a, PrivateKey &k)
{
    g2_mul(a.q, a.q, *(k.keydata));
    return a;
}

G2Element &operator*=(PrivateKey &k, G2Element &a)
{
    a *= k;
    return a;
}

G2Element operator*(G2Element &a, PrivateKey &k)
{
    g2_t ans;
    g2_new(ans);
    g2_mul(ans, a.q, *(k.keydata));
    return G2Element::FromNative(&ans);
}

G2Element operator*(PrivateKey &k, G2Element &a) { return a * k; }

G2Element PrivateKey::GetG2Power(g2_t base) const
{
    g2_t *q = SecAlloc<g2_t>(1);
    g2_mul(*q, base, *keydata);

    const G2Element ret = G2Element::FromNative(q);
    SecFree(*q);
    return ret;
}
  
PublicKey PrivateKey::GetPublicKey() const {
    g1_t *q = SecAlloc<g1_t>(1);
    g1_mul_gen(*q, *keydata);

    const PublicKey ret = PublicKey::FromG1(q);
    SecFree(*q);
    return ret;
}

PrivateKey PrivateKey::AggregateInsecure(std::vector<PrivateKey> const& privateKeys) {
    if (privateKeys.empty()) {
        throw std::length_error("Number of private keys must be at least 1");
    }

    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    PrivateKey ret(privateKeys[0]);
    for (size_t i = 1; i < privateKeys.size(); i++) {
        bn_add(*ret.keydata, *ret.keydata, *privateKeys[i].keydata);
        bn_mod_basic(*ret.keydata, *ret.keydata, order);
    }
    return ret;
}

PrivateKey PrivateKey::Aggregate(std::vector<PrivateKey> const& privateKeys,
                                 std::vector<PublicKey> const& pubKeys) {
    if (pubKeys.size() != privateKeys.size()) {
        throw std::length_error("Number of public keys must equal number of private keys");
    }
    if (privateKeys.empty()) {
        throw std::length_error("Number of keys must be at least 1");
    }

    std::vector<uint8_t*> serPubKeys(pubKeys.size());
    for (size_t i = 0; i < pubKeys.size(); i++) {
        serPubKeys[i] = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
        pubKeys[i].Serialize(serPubKeys[i]);
    }

    // Sort the public keys and private keys by public key
    std::vector<size_t> keysSorted(privateKeys.size());
    for (size_t i = 0; i < privateKeys.size(); i++) {
        keysSorted[i] = i;
    }

    std::sort(keysSorted.begin(), keysSorted.end(), [&serPubKeys](size_t a, size_t b) {
        return memcmp(serPubKeys[a], serPubKeys[b], PublicKey::PUBLIC_KEY_SIZE) < 0;
    });


    bn_t *computedTs = new bn_t[keysSorted.size()];
    for (size_t i = 0; i < keysSorted.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, keysSorted.size(), serPubKeys, keysSorted);

    // Raise all keys to power of the corresponding t's and aggregate the results into aggKey
    std::vector<PrivateKey> expKeys;
    expKeys.reserve(keysSorted.size());
    for (size_t i = 0; i < keysSorted.size(); i++) {
        auto& k = privateKeys[keysSorted[i]];
        expKeys.emplace_back(k.Mul(computedTs[i]));
    }
    PrivateKey aggKey = PrivateKey::AggregateInsecure(expKeys);

    for (auto p : serPubKeys) {
        delete[] p;
    }
    delete[] computedTs;

    BLS::CheckRelicErrors();
    return aggKey;
}

PrivateKey PrivateKey::Mul(const bn_t n) const {
    bn_t order;
    bn_new(order);
    g2_get_ord(order);

    PrivateKey ret;
    ret.AllocateKeyData();
    bn_mul_comba(*ret.keydata, *keydata, n);
    bn_mod_basic(*ret.keydata, *ret.keydata, order);
    return ret;
}

bool operator==(const PrivateKey& a, const PrivateKey& b) {
    return bn_cmp(*a.keydata, *b.keydata) == RLC_EQ;
}

bool operator!=(const PrivateKey& a, const PrivateKey& b) {
    return !(a == b);
}

PrivateKey& PrivateKey::operator=(const PrivateKey &rhs) {
    SecFree(keydata);
    AllocateKeyData();
    bn_copy(*keydata, *rhs.keydata);
    return *this;
}

void PrivateKey::Serialize(uint8_t* buffer) const {
    bn_write_bin(buffer, PrivateKey::PRIVATE_KEY_SIZE, *keydata);
}

std::vector<uint8_t> PrivateKey::Serialize() const {
    std::vector<uint8_t> data(PRIVATE_KEY_SIZE);
    Serialize(data.data());
    return data;
}


G2Element PrivateKey::SignG2(
    const uint8_t *msg,
    size_t len,
    const uint8_t *dst,
    size_t dst_len) const
{
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignG2Prehashed(messageHash, dst, dst_len);
}

G2Element PrivateKey::SignG2Prehashed(
    const uint8_t *messageHash,
    const uint8_t *dst,
    size_t dst_len) const
{
    g2_t sig, point;

    g2_map_ft(point, messageHash, BLS::MESSAGE_HASH_LEN);
    // ep2_map_impl(point, messageHash, BLS::MESSAGE_HASH_LEN, dst, dst_len);
    g2_mul(sig, point, *keydata);

    return G2Element::FromNative(&sig);
}

InsecureSignature PrivateKey::SignInsecure(const uint8_t *msg, size_t len) const {
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignInsecurePrehashed(messageHash);
}

InsecureSignature PrivateKey::SignInsecurePrehashed(const uint8_t *messageHash) const {
    g2_t sig, point;

    g2_map_ft(point, messageHash, BLS::MESSAGE_HASH_LEN);
    g2_mul(sig, point, *keydata);

    return InsecureSignature::FromG2(&sig);
}

Signature PrivateKey::Sign(const uint8_t *msg, size_t len) const {
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignPrehashed(messageHash);
}

Signature PrivateKey::SignPrehashed(const uint8_t *messageHash) const {
    InsecureSignature insecureSig = SignInsecurePrehashed(messageHash);
    Signature ret = Signature::FromInsecureSig(insecureSig);

    ret.SetAggregationInfo(AggregationInfo::FromMsgHash(GetPublicKey(),
            messageHash));

    return ret;
}

PrependSignature PrivateKey::SignPrepend(const uint8_t *msg, size_t len) const {
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignPrependPrehashed(messageHash);
}

PrependSignature PrivateKey::SignPrependPrehashed(const uint8_t *messageHash) const {
    uint8_t finalMessage[PublicKey::PUBLIC_KEY_SIZE + BLS::MESSAGE_HASH_LEN];
    GetPublicKey().Serialize(finalMessage);
    memcpy(finalMessage + PublicKey::PUBLIC_KEY_SIZE, messageHash, BLS::MESSAGE_HASH_LEN);

    uint8_t finalMessageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(finalMessageHash, finalMessage, PublicKey::PUBLIC_KEY_SIZE + BLS::MESSAGE_HASH_LEN);

    return PrependSignature::FromInsecureSig(SignInsecurePrehashed(finalMessageHash));
}

void PrivateKey::AllocateKeyData() {
    keydata = SecAlloc<bn_t>(1);
    bn_new(*keydata);  // Freed in destructor
    bn_zero(*keydata);
}
} // end namespace bls
