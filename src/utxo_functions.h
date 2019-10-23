// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Non-essential consensus functions used in RPC commands and possible QT wallet

#include <coins.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <txdb.h>

struct Amount;
/**
 * Calculate the difficulty for a given block index.
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

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats);
