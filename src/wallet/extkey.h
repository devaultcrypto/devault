// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <key.h>
#include <pubkey.h>

#include <stdexcept>
#include <vector>

const unsigned int BIP32_EXTKEY_SIZE = 74;
const unsigned int BIP32_EXTKEY_BLS_SIZE = 89;

typedef uint256 ChainCode;

struct CExtPubKey;

struct CExtKey {
    uint8_t nDepth;
    uint8_t vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    CKey key;

    friend bool operator==(const CExtKey &a, const CExtKey &b) {
        return a.nDepth == b.nDepth &&
               memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0],
                      sizeof(vchFingerprint)) == 0 &&
               a.nChild == b.nChild && a.chaincode == b.chaincode &&
               a.key == b.key;
    }

    void Encode(uint8_t code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const uint8_t code[BIP32_EXTKEY_SIZE]);
    bool Derive(CExtKey &out, unsigned int nChild) const;
    CExtPubKey Neuter() const;
    CExtPubKey NeuterBLS() const;
    void SetMaster(const uint8_t *seed, unsigned int nSeedLen);
    template <typename Stream> void Serialize(Stream &s) const {
        unsigned int len = BIP32_EXTKEY_SIZE;
        ::WriteCompactSize(s, len);
        uint8_t code[BIP32_EXTKEY_SIZE];
        Encode(code);
        s.write((const char *)&code[0], len);
    }
    template <typename Stream> void Unserialize(Stream &s) {
        unsigned int len = ::ReadCompactSize(s);
        if (len != BIP32_EXTKEY_SIZE) {
            throw std::runtime_error("Invalid extended key size\n");
        }

        uint8_t code[BIP32_EXTKEY_SIZE];
        s.read((char *)&code[0], len);
        Decode(code);
    }
};

struct CExtPubKey {
    uint8_t nDepth;
    uint8_t vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    CPubKey pubkey;

    friend bool operator==(const CExtPubKey &a, const CExtPubKey &b) {
        return a.nDepth == b.nDepth &&
               memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0],
                      sizeof(vchFingerprint)) == 0 &&
            a.nChild == b.nChild && a.chaincode == b.chaincode && a.pubkey == b.pubkey;
    }
    bool IsBLS() const { return pubkey.IsBLS(); }

    void Encode(uint8_t code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const uint8_t code[BIP32_EXTKEY_SIZE]);
    
    void BLSEncode(uint8_t code[BIP32_EXTKEY_BLS_SIZE]) const;
    void BLSDecode(const uint8_t code[BIP32_EXTKEY_BLS_SIZE]);
    
    void Serialize(CSizeComputer &s) const {
        // Optimized implementation for ::GetSerializeSize that avoids copying.
        // add one byte for the size (compact int)
        s.seek(BIP32_EXTKEY_SIZE + 1);
    }
    template <typename Stream> void Serialize(Stream &s) const {
        unsigned int len = BIP32_EXTKEY_SIZE;
        bool is_bls = false;
        if (pubkey.size() > 33) {
            is_bls = true;
            len = BIP32_EXTKEY_BLS_SIZE;
        }
        ::WriteCompactSize(s, len);
        if (is_bls) {
            uint8_t bls_code[BIP32_EXTKEY_BLS_SIZE];
            BLSEncode(bls_code);
            s.write((const char *)&bls_code[0], len);
        } else {
            uint8_t code[BIP32_EXTKEY_SIZE];
            Encode(code);
            s.write((const char *)&code[0], len);
        }
    }
    template <typename Stream> void Unserialize(Stream &s) {
        unsigned int total_len = ::ReadCompactSize(s);
        if (!((total_len == BIP32_EXTKEY_BLS_SIZE) || (total_len == BIP32_EXTKEY_SIZE))) {
            throw std::runtime_error("Invalid extended key size\n");
        }
        if (total_len != BIP32_EXTKEY_SIZE) {
            uint8_t code[BIP32_EXTKEY_BLS_SIZE];
            s.read((char *)&code[0], total_len);
            BLSDecode(code);
        } else {
            uint8_t code[BIP32_EXTKEY_SIZE];
            s.read((char *)&code[0], total_len);
            Decode(code);
        }
    }
};

