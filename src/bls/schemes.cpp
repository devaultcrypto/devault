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
#include <set>
#include <string>

#include "bls/bls.hpp"
#include "bls/elements.hpp"
#include "bls/schemes.hpp"

using std::string;
using std::vector;

namespace bls {
/* These are all for the min-pubkey-size variant.
   TODO : analogs for min-signature-size
*/

const uint8_t *BasicScheme::CIPHERSUITE_ID =
    (const uint8_t *)"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
const int BasicScheme::CIPHERSUITE_ID_LEN = 43;
const uint8_t *AugScheme::CIPHERSUITE_ID =
    (const uint8_t *)"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_AUG_";
const int AugScheme::CIPHERSUITE_ID_LEN = 43;
const uint8_t *PopScheme::CIPHERSUITE_ID =
    (const uint8_t *)"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
const int PopScheme::CIPHERSUITE_ID_LEN = 43;

// TODO: PrivateKey Core::KeyGen - use PrivateKey.FromSeed for now

vector<uint8_t> Core::SkToPk(const PrivateKey &seckey)
{
    return seckey.GetG1Element().Serialize();
}

G1Element Core::SkToG1(const PrivateKey &seckey)
{
    return seckey.GetG1Element();
}

vector<uint8_t> Core::Sign(
    const PrivateKey &seckey,
    const vector<uint8_t> &message,
    const uint8_t *dst,
    int dst_len)
{
    return Core::SignNative(seckey, message, dst, dst_len).Serialize();
}

G2Element Core::SignNative(
    const PrivateKey &seckey,
    const vector<uint8_t> &message,
    const uint8_t *dst,
    int dst_len)
{
    return seckey.SignG2(message.data(), message.size(), dst, dst_len);
}

bool Core::Verify(
    const vector<uint8_t> &pubkey,
    const vector<uint8_t> &message,  // unhashed
    const vector<uint8_t> &signature,
    const uint8_t *dst,
    int dst_len)
{
  try {
    return Core::Verify(
        G1Element::FromBytes(pubkey.data()),
        message,
        G2Element::FromBytes(signature.data()),
        dst,
        dst_len);
  } catch (...) {
    return false;
  }
}

bool Core::Verify(
    const G1Element &pubkey,
    const vector<uint8_t> &message,  // unhashed
    const G2Element &signature,
    const uint8_t *dst,
    int dst_len)
{
    G1Element geninverse = G1Element::Generator().Inverse();
    G2Element hashedPoint = G2Element::FromMessage(message, dst, dst_len);

    g1_t *g1s = new g1_t[2];
    g2_t *g2s = new g2_t[2];
    g1_copy(g1s[0], *(g1_t *)&geninverse.p);
    g1_copy(g1s[1], *(g1_t *)&pubkey.p);
    g2_copy(g2s[0], *(g2_t *)&signature.q);
    g2_copy(g2s[1], *(g2_t *)&hashedPoint.q);
    bool ans = Core::NativeVerify(g1s, g2s, 2);

    delete[] g1s;
    delete[] g2s;
    return ans;
}

vector<uint8_t> Core::Aggregate(vector<vector<uint8_t>> const &signatures)
{
    g2_t ans;
    g2_free(ans);
    g2_new(ans);
    g2_copy(ans, G2Element::FromBytes(signatures[0].data()).q);

    for (size_t i = 1; i < signatures.size(); ++i) {
      g2_add(ans, ans, G2Element::FromBytes(signatures[i].data()).q);
    }
    return G2Element::FromNative(&ans).Serialize();
}

G2Element Core::Aggregate(vector<G2Element> const &signatures)
{
    g2_t ans;
    g2_free(ans);
    g2_new(ans);
    g2_copy(ans, *(g2_t *)&signatures[0].q);

    for (size_t i = 1; i < signatures.size(); ++i) 
        g2_add(ans, ans, *(g2_t *)&signatures[i].q);
    return G2Element::FromNative(&ans);
}

bool Core::AggregateVerify(
    const vector<vector<uint8_t>> &pubkeys,
    const vector<vector<uint8_t>> &messages,  // unhashed
    const vector<uint8_t> &signature,
    const uint8_t *dst,
    int dst_len)
{
    size_t n = pubkeys.size();
    if (n != messages.size() || n <= 0)
        return false;
    vector<G1Element> pubkeyElements(n);
    try {
      for (size_t i = 0; i < n; ++i) {
         pubkeyElements[i] = G1Element::FromBytes(pubkeys[i].data());
      }
      G2Element signatureElement = G2Element::FromBytes(signature.data());
      return Core::AggregateVerify(
        pubkeyElements, messages, signatureElement, dst, dst_len);
    } catch (...) {
      throw std::runtime_error("Problem in Core::AggregateVerify");
      return false;
    }
}

bool Core::AggregateVerify(
    const vector<G1Element> &pubkeys,
    const vector<vector<uint8_t>> &messages,  // unhashed
    const G2Element &signature,
    const uint8_t *dst,
    int dst_len)
{
    size_t n = pubkeys.size();
    if (n != messages.size() || n <= 0)
        return false;

    g1_t *g1s = new g1_t[n + 1];
    g2_t *g2s = new g2_t[n + 1];

    G1Element geninverse = G1Element::Generator().Inverse();
    g1_copy(g1s[0], *(g1_t *)&geninverse.p);
    g2_copy(g2s[0], *(g2_t *)&signature.q);
    for (size_t i = 0; i < n; ++i) {
        g1_copy(g1s[i + 1], *(g1_t *)&pubkeys[i].p);
        g2_copy(
            g2s[i + 1], G2Element::FromMessage(messages[i], dst, dst_len).q);
    }

    bool ans = Core::NativeVerify(g1s, g2s, n + 1);
    delete[] g1s;
    delete[] g2s;
    return ans;
}

bool Core::NativeVerify(g1_t *pubkeys, g2_t *mappedHashes, size_t length)
{
    gt_t target, candidate;
    fp12_zero(target);
    fp_set_dig(target[0][0][0], 1);

    // prod e(pubkey[i], hash[i]) * e(-1 * g1, aggSig)
    // Performs pubKeys.size() pairings
    pc_map_sim(candidate, pubkeys, mappedHashes, length);

    // 1 =? prod e(pubkey[i], hash[i]) * e(g1, aggSig)
    if (gt_cmp(target, candidate) != RLC_EQ || core_get()->code != RLC_OK) {
        core_get()->code = RLC_OK;
        return false;
    }
    BLS::CheckRelicErrors();
    return true;
}

vector<uint8_t> BasicScheme::Sign(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    return Core::Sign(
        seckey,
        message,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

G2Element BasicScheme::SignNative(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    return Core::SignNative(
        seckey,
        message,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

bool BasicScheme::Verify(
    const vector<uint8_t> &pubkey,
    const vector<uint8_t> &message,
    const vector<uint8_t> &signature)
{
    return Core::Verify(
        pubkey,
        message,
        signature,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

bool BasicScheme::Verify(
    const G1Element &pubkey,
    const vector<uint8_t> &message,
    const G2Element &signature)
{
    return Core::Verify(
        pubkey,
        message,
        signature,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

bool BasicScheme::AggregateVerify(
    const vector<vector<uint8_t>> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const vector<uint8_t> &signature)
{
    size_t n = messages.size();
    if (n <= 0)
        return false;
    std::set<vector<uint8_t>> s(messages.begin(), messages.end());
    if (s.size() != n)
        return false;
    return Core::AggregateVerify(
        pubkeys,
        messages,
        signature,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

bool BasicScheme::AggregateVerify(
    const vector<G1Element> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const G2Element &signature)
{
    size_t n = messages.size();
    if (n <= 0)
        return false;
    std::set<vector<uint8_t>> s(messages.begin(), messages.end());
    if (s.size() != n)
        return false;
    return Core::AggregateVerify(
        pubkeys,
        messages,
        signature,
        BasicScheme::CIPHERSUITE_ID,
        BasicScheme::CIPHERSUITE_ID_LEN);
}

vector<uint8_t> AugScheme::Sign(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    return AugScheme::SignNative(seckey, message).Serialize();
}

G2Element AugScheme::SignNative(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    vector<uint8_t> augMessage = seckey.GetG1Element().Serialize();
    augMessage.reserve(
        augMessage.size() + std::distance(message.begin(), message.end()));
    augMessage.insert(augMessage.end(), message.begin(), message.end());
    return seckey.SignG2(
        augMessage.data(),
        augMessage.size(),
        AugScheme::CIPHERSUITE_ID,
        AugScheme::CIPHERSUITE_ID_LEN);
}

bool AugScheme::Verify(
    const vector<uint8_t> &pubkey,
    const vector<uint8_t> &message,
    const vector<uint8_t> &signature)
{
    vector<uint8_t> augMessage(pubkey);
    augMessage.reserve(
        augMessage.size() + std::distance(message.begin(), message.end()));
    augMessage.insert(augMessage.end(), message.begin(), message.end());
    return Core::Verify(
        pubkey,
        augMessage,
        signature,
        AugScheme::CIPHERSUITE_ID,
        AugScheme::CIPHERSUITE_ID_LEN);
}

bool AugScheme::Verify(
    const G1Element &pubkey,
    const vector<uint8_t> &message,
    const G2Element &signature)
{
    vector<uint8_t> augMessage(pubkey.Serialize());
    augMessage.reserve(
        augMessage.size() + std::distance(message.begin(), message.end()));
    augMessage.insert(augMessage.end(), message.begin(), message.end());
    return Core::Verify(
        pubkey,
        augMessage,
        signature,
        AugScheme::CIPHERSUITE_ID,
        AugScheme::CIPHERSUITE_ID_LEN);
}

bool AugScheme::AggregateVerify(
    const vector<vector<uint8_t>> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const vector<uint8_t> &signature)
{
    int n = messages.size();
    if (n <= 0)
        return false;
    vector<vector<uint8_t>> augMessages(n);
    for (int i = 0; i < n; ++i) {
        vector<uint8_t> aug(pubkeys[i]);
        aug.reserve(
            aug.size() + std::distance(messages[i].begin(), messages[i].end()));
        aug.insert(aug.end(), messages[i].begin(), messages[i].end());
        augMessages[i] = aug;
    }
    try {
    return Core::AggregateVerify(
        pubkeys,
        const_cast<const vector<vector<uint8_t>> &>(augMessages),
        signature,
        AugScheme::CIPHERSUITE_ID,
        AugScheme::CIPHERSUITE_ID_LEN);
    }
  catch (...) {
    throw std::runtime_error("Problem in AugScheme::AggregateVerify");
  }
  return false;
}

bool AugScheme::AggregateVerify(
    const vector<G1Element> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const G2Element &signature)
{
    int n = messages.size();
    if (n <= 0)
        return false;
    vector<vector<uint8_t>> augMessages(n);
    for (int i = 0; i < n; ++i) {
        vector<uint8_t> aug(pubkeys[i].Serialize());
        aug.reserve(
            aug.size() + std::distance(messages[i].begin(), messages[i].end()));
        aug.insert(aug.end(), messages[i].begin(), messages[i].end());
        augMessages[i] = aug;
    }

    return Core::AggregateVerify(
        pubkeys,
        const_cast<const vector<vector<uint8_t>> &>(augMessages),
        signature,
        AugScheme::CIPHERSUITE_ID,
        AugScheme::CIPHERSUITE_ID_LEN);
}

vector<uint8_t> PopScheme::Sign(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    return Core::Sign(
        seckey,
        message,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

G2Element PopScheme::SignNative(
    const PrivateKey &seckey,
    const vector<uint8_t> &message)
{
    return Core::SignNative(
        seckey,
        message,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

bool PopScheme::Verify(
    const vector<uint8_t> &pubkey,
    const vector<uint8_t> &message,
    const vector<uint8_t> &signature)
{
    return Core::Verify(
        pubkey,
        message,
        signature,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

bool PopScheme::Verify(
    const G1Element &pubkey,
    const vector<uint8_t> &message,
    const G2Element &signature)
{
    return Core::Verify(
        pubkey,
        message,
        signature,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

bool PopScheme::AggregateVerify(
    const vector<vector<uint8_t>> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const vector<uint8_t> &signature)
{
    return Core::AggregateVerify(
        pubkeys,
        messages,
        signature,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

bool PopScheme::AggregateVerify(
    const vector<G1Element> &pubkeys,
    const vector<vector<uint8_t>> &messages,
    const G2Element &signature)
{
    return Core::AggregateVerify(
        pubkeys,
        messages,
        signature,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

G2Element PopScheme::PopProveNative(const PrivateKey &seckey)
{
    G1Element pk = seckey.GetG1Element();
    G2Element hashedKey = G2Element::FromMessage(
        pk.Serialize(),
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);

    return seckey.GetG2Power(*(g2_t *)&hashedKey.q);
}

vector<uint8_t> PopScheme::PopProve(const PrivateKey &seckey)
{
    return PopScheme::PopProveNative(seckey).Serialize();
}

bool PopScheme::PopVerify(
    const G1Element &pubkey,
    const G2Element &signature_proof)
{
    G1Element geninverse = G1Element::Generator().Inverse();
    G2Element hashedPoint = G2Element::FromMessage(
        pubkey.Serialize(),
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);

    g1_t *g1s = new g1_t[2];
    g2_t *g2s = new g2_t[2];
    g1_copy(g1s[0], *(g1_t *)&geninverse.p);
    g1_copy(g1s[1], *(g1_t *)&pubkey.p);
    g2_copy(g2s[0], *(g2_t *)&signature_proof.q);
    g2_copy(g2s[1], *(g2_t *)&hashedPoint.q);

    bool ans = Core::NativeVerify(g1s, g2s, 2);
    delete[] g1s;
    delete[] g2s;
    return ans;
}

bool PopScheme::PopVerify(
    const vector<uint8_t> &pubkey,
    const vector<uint8_t> &proof)
{
    G1Element p = G1Element::FromBytes(pubkey.data());
    G2Element q = G2Element::FromBytes(proof.data());
    return PopScheme::PopVerify(p, q);
}

bool PopScheme::FastAggregateVerify(
    const vector<G1Element> &pubkeys,
    const vector<uint8_t> &message,
    const G2Element &signature)
{
    int n = pubkeys.size();
    if (n <= 0)
        return false;

    G1Element pkagg = G1Element();
    for (G1Element pk : pubkeys) pkagg += pk;

    return Core::Verify(
        pkagg,
        message,
        signature,
        PopScheme::CIPHERSUITE_ID,
        PopScheme::CIPHERSUITE_ID_LEN);
}

bool PopScheme::FastAggregateVerify(
    const vector<vector<uint8_t>> &pubkeys,
    const vector<uint8_t> &message,
    const vector<uint8_t> &signature)
{
    int n = pubkeys.size();
    if (n <= 0) {
        return false;
    }
    vector<G1Element> pkelements(n);
    for (int i = 0; i < n; ++i) {
        pkelements[i] = G1Element::FromBytes(pubkeys[i].data());
    }

    return PopScheme::FastAggregateVerify(
        pkelements, message, G2Element::FromBytes(signature.data()));
}
}  // end namespace bls
