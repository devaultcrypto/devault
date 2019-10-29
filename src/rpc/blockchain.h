// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCBLOCKCHAIN_H
#define BITCOIN_RPCBLOCKCHAIN_H

#include <univalue.h>

class CBlock;
class CBlockIndex;
class Config;
class CTxMemPool;
class JSONRPCRequest;

UniValue getblockchaininfo(const Config &config, const JSONRPCRequest &request);

/** Callback for when block tip changed. */
void RPCNotifyBlockChange(bool ibd, const CBlockIndex *pindex);

/** Block description to JSON */
UniValue blockToJSON(const CBlock &block, const CBlockIndex *tip,
                     const CBlockIndex *blockindex, bool txDetails = false);

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool &pool);

/** Mempool to JSON */
UniValue mempoolToJSON(bool fVerbose = false);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex *tip,
                           const CBlockIndex *blockindex);

#endif // BITCOIN_RPCBLOCKCHAIN_H
