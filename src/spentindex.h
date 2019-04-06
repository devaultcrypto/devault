// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "amount.h"
#include "serialize.h"
#include "uint256.h"

struct CSpentIndexKey {
    uint256 txid;
    uint32_t outputIndex;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(txid);
        READWRITE(outputIndex);
    }

    CSpentIndexKey(const uint256 &t, uint32_t i) {
        txid = t;
        outputIndex = i;
    }

    CSpentIndexKey() { SetNull(); }

    void SetNull() {
        txid.SetNull();
        outputIndex = 0;
    }
};

struct CSpentIndexValue {
    uint256 txid;
    uint32_t inputIndex;
    int32_t blockHeight;
    Amount satoshis;
    int32_t addressType;
    uint160 addressHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(txid);
        READWRITE(inputIndex);
        READWRITE(blockHeight);
        READWRITE(satoshis);
        READWRITE(addressType);
        READWRITE(addressHash);
    }

    CSpentIndexValue(const uint256 &t, uint32_t i, int32_t h, Amount s,
                     int32_t type, const uint160 &a) {
        txid = t;
        inputIndex = i;
        blockHeight = h;
        satoshis = s;
        addressType = type;
        addressHash = a;
    }

    CSpentIndexValue() { SetNull(); }

    void SetNull() {
        txid.SetNull();
        inputIndex = 0;
        blockHeight = 0;
        satoshis = Amount();
        addressType = 0;
        addressHash.SetNull();
    }

    bool IsNull() const { return txid.IsNull(); }
};

struct CSpentIndexKeyCompare {
    bool operator()(const CSpentIndexKey &a, const CSpentIndexKey &b) const {
        if (a.txid == b.txid) {
            return a.outputIndex < b.outputIndex;
        } else {
            return a.txid < b.txid;
        }
    }
};
