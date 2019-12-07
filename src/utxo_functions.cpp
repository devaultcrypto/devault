// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Non-essential consensus functions used in RPC commands and possible QT wallet

#include <utxo_functions.h>
#include <amount.h>
#include <coins.h>
#include <config.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <streams.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h>
#include <util.h>
#include <utilstrencodings.h>
#include <validation.h>
#include <init.h> // for ShutdownRequested

#include <cashaddrenc.h>
#include <dstencode.h>
#include <chainparams.h>
/**
 * Calculate the difficulty for a given block index.
 */
double GetDifficulty(const CBlockIndex *blockindex) {
    assert(blockindex);

    int nShift = (blockindex->nBits >> 24) & 0xff;
    double dDiff = double(0x0000ffff) / double(blockindex->nBits & 0x00ffffff);

    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

void ApplyStats(CCoinsStats &stats, CHashWriter &ss, const uint256 &hash,
                       const std::map<uint32_t, Coin> &outputs) {
    assert(!outputs.empty());
    ss << hash;
    ss << VARINT(outputs.begin()->second.GetHeight() * 2 +
                 outputs.begin()->second.IsCoinBase());
    stats.nTransactions++;
    for (const auto& output : outputs) {
        ss << VARINT(output.first + 1);
        ss << output.second.GetTxOut().scriptPubKey;
        ss << VARINT(output.second.GetTxOut().nValue.toInt());
        stats.nTransactionOutputs++;
        stats.nTotalAmount += output.second.GetTxOut().nValue;
        stats.nBogoSize +=
            32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ +
            8 /* amount */ + 2 /* scriptPubKey len */ +
            output.second.GetTxOut().scriptPubKey.size() /* scriptPubKey */;
    }
    ss << VARINT(0);
}

std::map<COutPoint, Coin> GetUTXOSet(CCoinsView *view, const CTxDestination& source) {
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    pcursor->GetBestBlock();
    uint256 prevkey;
    std::map<COutPoint, Coin> outputs;

    std::string address = EncodeDestination(source);
    
    while (pcursor->Valid()) {
        interruption_point(ShutdownRequested());
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (address == GetAddrFromTxOut(coin.GetTxOut())) {
                outputs.emplace(key, coin);
            }
        } else {
            break; //error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    return outputs;
}
std::map<COutPoint, Coin> GetUTXOSet(CCoinsView *view, CScript& coinscript) {
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    pcursor->GetBestBlock();
    uint256 prevkey;
    std::map<COutPoint, Coin> outputs;
    while (pcursor->Valid()) {
        interruption_point(ShutdownRequested());
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (coin.GetTxOut().scriptPubKey == coinscript) {
                outputs.emplace(key, coin);
            }
        } else {
            break; //error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    return outputs;
}

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats) {
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = pcursor->GetBestBlock();
    {
        LOCK(cs_main);
        stats.nHeight = mapBlockIndex.find(stats.hashBlock)->second->nHeight;
    }
    ss << stats.hashBlock;
    uint256 prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        interruption_point(ShutdownRequested());
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (!outputs.empty() && key.GetTxId() != prevkey) {
                ApplyStats(stats, ss, prevkey, outputs);
                outputs.clear();
            }
            prevkey = key.GetTxId();
            outputs[key.GetN()] = std::move(coin);
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    if (!outputs.empty()) {
        ApplyStats(stats, ss, prevkey, outputs);
    }
    stats.hashSerialized = ss.GetHash();
    stats.nDiskSize = view->EstimateSize();
    return true;
}
