// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <serialize.h>
#include <uint256.h>

struct CTimestampIndexIteratorKey {
    uint32_t timestamp;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(timestamp);
    }

    CTimestampIndexIteratorKey(uint32_t time) { timestamp = time; }

    CTimestampIndexIteratorKey() { SetNull(); }

    void SetNull() { timestamp = 0; }
};

struct CTimestampIndexKey {
    uint32_t timestamp;
    uint256 blockHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(timestamp);
        READWRITE(blockHash);
    }

    CTimestampIndexKey(uint32_t time, const uint256 &hash) {
        timestamp = time;
        blockHash = hash;
    }

    CTimestampIndexKey() { SetNull(); }

    void SetNull() {
        timestamp = 0;
        blockHash.SetNull();
    }
};

struct CTimestampBlockIndexKey {
    uint256 blockHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(blockHash);
    }

    CTimestampBlockIndexKey(const uint256 &hash) { blockHash = hash; }

    CTimestampBlockIndexKey() { SetNull(); }

    void SetNull() { blockHash.SetNull(); }
};

struct CTimestampBlockIndexValue {
    uint32_t ltimestamp;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(ltimestamp);
    }

    CTimestampBlockIndexValue(uint32_t time) { ltimestamp = time; }

    CTimestampBlockIndexValue() { SetNull(); }

    void SetNull() { ltimestamp = 0; }
};
