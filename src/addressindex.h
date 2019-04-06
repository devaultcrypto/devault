// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "amount.h"
#include "script/script.h"
#include "uint256.h"
#include "cashaddrenc.h"

struct CAddressUnspentKey {
    uint8_t type;
    uint160 hashBytes;
    uint256 txhash;
    int32_t index;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(type);
        READWRITE(hashBytes);
        READWRITE(txhash);
        READWRITE(index);
    }

    CAddressUnspentKey(uint8_t addressType, const uint160 &addressHash,
                       const uint256 &txid, int32_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() { SetNull(); }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    Amount satoshis;
    CScript script;
    int32_t blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(satoshis);
        READWRITE(*(CScriptBase *)(&script));
        READWRITE(blockHeight);
    }

    CAddressUnspentValue(Amount sats, CScript scriptPubKey, int32_t height) {
        satoshis = sats;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() { SetNull(); }

    void SetNull() {
        satoshis = -SATOSHI;
        script.clear();
        blockHeight = 0;
    }

    bool IsNull() const { return (satoshis == -SATOSHI); }
};

struct CAddressIndexKey {
    uint8_t type;
    uint160 hashBytes;
    int32_t blockHeight;
    uint32_t txindex;
    uint256 txhash;
    int32_t index;
    bool spending;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(type);
        READWRITE(hashBytes);
        READWRITE(blockHeight);
        READWRITE(txindex);
        READWRITE(txhash);
        READWRITE(index);
        READWRITE(spending);
    }

    CAddressIndexKey(uint8_t addressType, const uint160 &addressHash,
                     int32_t height, int32_t blockindex, const uint256 &txid,
                     int32_t indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddressIndexKey() { SetNull(); }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
    }
};

struct CAddressIndexIteratorKey {
    uint8_t type;
    uint160 hashBytes;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(type);
        READWRITE(hashBytes);
    }

    CAddressIndexIteratorKey(const CashAddrContent& content) {
        type = content.type;
        hashBytes = uint160(content.hash);
    }

    CAddressIndexIteratorKey() { SetNull(); }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
    }
};

struct CAddressIndexIteratorHeightKey {
    uint8_t type;
    uint160 hashBytes;
    int32_t blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(type);
        READWRITE(hashBytes);
        READWRITE(blockHeight);
    }

    CAddressIndexIteratorHeightKey(const CashAddrContent& content, int32_t height) {
        type = content.type;
        hashBytes = uint160(content.hash);
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() { SetNull(); }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        blockHeight = 0;
    }
};

struct CMempoolAddressDelta {
    int64_t time;
    Amount amount;
    uint256 prevhash;
    uint32_t prevout;

    CMempoolAddressDelta(int64_t t, Amount a, const uint256 &hash,
                         uint32_t out) {
        time = t;
        amount = a;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddressDelta(int64_t t, Amount a) {
        time = t;
        amount = a;
        prevhash.SetNull();
        prevout = 0;
    }
};

struct CMempoolAddressDeltaKey {
    int32_t type;
    uint160 addressBytes;
    uint256 txhash;
    uint32_t index;
    int32_t spending;

    CMempoolAddressDeltaKey(uint8_t addressType, const uint160 &addressHash,
                            const uint256 &hash, uint32_t i, int32_t s) {
        type = addressType;
        addressBytes = addressHash;
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddressDeltaKey(uint8_t addressType, const uint160 &addressHash) {
        type = addressType;
        addressBytes = addressHash;
        txhash.SetNull();
        index = 0;
        spending = 0;
    }
};

struct CMempoolAddressDeltaKeyCompare {
    bool operator()(const CMempoolAddressDeltaKey &a,
                    const CMempoolAddressDeltaKey &b) const {
        if (a.type == b.type) {
            if (a.addressBytes == b.addressBytes) {
                if (a.txhash == b.txhash) {
                    if (a.index == b.index) {
                        return a.spending < b.spending;
                    } else {
                        return a.index < b.index;
                    }
                } else {
                    return a.txhash < b.txhash;
                }
            } else {
                return a.addressBytes < b.addressBytes;
            }
        } else {
            return a.type < b.type;
        }
    }
};
