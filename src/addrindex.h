// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "serialize.h"
#include "amount.h"
#include "uint256.h"

struct CAddrIndexKey {
    std::string addr;
    int32_t blockHeight;
    uint32_t txindex;
    uint256 txhash;
    int32_t index;
    bool spending;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(addr);
        READWRITE(blockHeight);
        READWRITE(txindex);
        READWRITE(txhash);
        READWRITE(index);
        READWRITE(spending);
    }

    CAddrIndexKey(const std::string &address, int32_t height,
                  int32_t blockindex, const uint256 &txid, int32_t indexValue,
                  bool isSpending) {
        addr = address;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddrIndexKey() { SetNull(); }

    void SetNull() {
        addr = "";
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
    }
};

struct CAddrIndexIteratorKey {
    std::string addr;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(addr);
    }

    CAddrIndexIteratorKey(const std::string &content) { addr = content; }

    CAddrIndexIteratorKey() { SetNull(); }

    void SetNull() { addr = ""; }
};

struct CAddrIndexIteratorHeightKey {
    std::string addr;
    int32_t blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(addr);
        READWRITE(blockHeight);
    }

    CAddrIndexIteratorHeightKey(const std::string &content, int32_t height) {
        addr = content;
        blockHeight = height;
    }

    CAddrIndexIteratorHeightKey() { SetNull(); }

    void SetNull() {
        addr = "";
        blockHeight = 0;
    }
};

struct CMempoolAddrDelta {
    int64_t time;
    Amount amount;
    uint256 prevhash;
    uint32_t prevout;

    CMempoolAddrDelta(int64_t t, Amount a, const uint256 &hash, uint32_t out) {
        time = t;
        amount = a;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddrDelta(int64_t t, Amount a) {
        time = t;
        amount = a;
        prevhash.SetNull();
        prevout = 0;
    }
};

struct CMempoolAddrDeltaKey {
    std::string addr;
    uint256 txhash;
    uint32_t index;
    int32_t spending;

    CMempoolAddrDeltaKey(const std::string& address,
                         const uint256 &hash, uint32_t i, int32_t s) {
        addr = address;
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddrDeltaKey(const std::string& address) {
        addr = address;
        txhash.SetNull();
        index = 0;
        spending = 0;
    }
};

struct CMempoolAddrDeltaKeyCompare {
    bool operator()(const CMempoolAddrDeltaKey &a,
                    const CMempoolAddrDeltaKey &b) const {
        if (a.addr == b.addr) {
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
            return a.addr < b.addr;
        }
    }
};
