// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/extkey.h>
#include <bls/bls_functions.h>

#include <crypto/common.h>
#include <crypto/hmac_sha512.h>

#include "bls/bls_functions.h"


bool CExtKey::Derive(CExtKey &out, unsigned int _nChild) const {
    out.nDepth = nDepth + 1;
    if (is_bls) {
      // Ignores nDepths, chaincode and FingerPrint....
      out.key = bls::GetBLSChild(key, _nChild);
      out.nChild = _nChild;
      out.is_bls = true;
      return true;
    } else {
      CKeyID id = key.GetPubKey().GetKeyID(); // doesn't matter than it's only EC
      memcpy(&out.vchFingerprint[0], &id, 4);
      out.nChild = _nChild;
      return key.Derive(out.key, out.chaincode, _nChild, chaincode);
    }
}

void CExtKey::SetMaster(const SecureVector& seed, bool bls) {
    static const uint8_t hashkey[] = {'B', 'i', 't', 'c', 'o', 'i',
                                      'n', ' ', 's', 'e', 'e', 'd'};
    std::vector<uint8_t, secure_allocator<uint8_t>> vout(64);
    CHMAC_SHA512(hashkey, sizeof(hashkey))
        .Write(&seed[0], seed.size())
        .Finalize(vout.data());
    if (bls) {
      key = bls::GetBLSMasterKey(seed);
    } else {
      key.Set(vout.data(), vout.data() + 32);
    }
    memcpy(chaincode.begin(), vout.data() + 32, 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
    is_bls = bls;
}

CExtPubKey CExtKey::Neuter() const {
    if (is_bls) return NeuterBLS();
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    ret.chaincode = chaincode;
    return ret;
}

CExtPubKey CExtKey::NeuterBLS() const {
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.chaincode = chaincode;
    ret.pubkey = key.GetPubKeyForBLS();
    return ret;
}

void CExtKey::Encode(uint8_t code[BIP32_EXTKEY_SIZE]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, chaincode.begin(), 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code + 42, key.begin(), 32);
}

void CExtKey::Decode(const uint8_t code[BIP32_EXTKEY_SIZE]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(chaincode.begin(), code + 9, 32);
    key.Set(code + 42, code + BIP32_EXTKEY_SIZE);
}

///------------------------------------------------------------------------------------------
/// ExtPubKey
///------------------------------------------------------------------------------------------

void CExtPubKey::Encode(uint8_t code[BIP32_EXTKEY_SIZE]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, chaincode.begin(), 32);
    assert(pubkey.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE);
    memcpy(code + 41, pubkey.begin(), CPubKey::COMPRESSED_PUBLIC_KEY_SIZE);
}

void CExtPubKey::BLSEncode(uint8_t code[BIP32_EXTKEY_BLS_SIZE]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, chaincode.begin(), 32);
    assert(pubkey.size() == CPubKey::BLS_PUBLIC_KEY_SIZE);
    memcpy(code + 41, pubkey.begin(), CPubKey::BLS_PUBLIC_KEY_SIZE);
}

void CExtPubKey::Decode(const uint8_t code[BIP32_EXTKEY_SIZE]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(chaincode.begin(), code + 9, 32);
    pubkey.Set(code + 41, code + BIP32_EXTKEY_SIZE);
}

void CExtPubKey::BLSDecode(const uint8_t code[BIP32_EXTKEY_BLS_SIZE]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(chaincode.begin(), code + 9, 32);
    pubkey.Set(code + 41, code + 41+48);
}
