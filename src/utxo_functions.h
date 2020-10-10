// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Non-essential consensus functions used in RPC commands and possible QT wallet

#pragma once

#include <coins.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <txdb.h>

struct Amount;
/**
 * Get the required difficulty of the next block w/r/t the given block index.
 *
 * @return A floating point number that is a multiple of the main net minimum
 * difficulty (4295032833 hashes).
 */
double GetDifficulty(const CBlockIndex *blockindex);

struct CCoinsStats {
    int nHeight;
    uint256 hashBlock;
    uint64_t nTransactions;
    uint64_t nTransactionOutputs;
    uint64_t nBogoSize;
    uint256 hashSerialized;
    uint64_t nDiskSize;
    Amount nTotalAmount;

    CCoinsStats()
        : nHeight(0), nTransactions(0), nTransactionOutputs(0), nBogoSize(0),
          nDiskSize(0), nTotalAmount() {}
};

void ApplyStats(CCoinsStats &stats, CHashWriter &ss, const uint256 &hash,
                const std::map<uint32_t, Coin> &outputs);

std::map<COutPoint, Coin> GetUTXOSet(CCoinsView *view, CScript& coinscript);
std::map<COutPoint, Coin> GetUTXOSet(CCoinsView *view, const CTxDestination& source);
//std::map<COutPoint, Coin> GetUTXOSet(CCoinsView *view, const std::string& address);

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats);
