// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>
#include <utxo_functions.h>

#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <checkpoints.h>
#include <coins.h>
#include <config.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <hash.h>
#include <index/txindex.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <streams.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <warnings.h>
#include <dstencode.h>
#include <init.h> // for ShutdownRequested
#include <validationinterface.h>

#include <devault/coinreward.h>
#include <devault/rewards.h>
#include <devault/budget.h>

#include <thread> // boost::thread::interrupt
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <fstream>
#include <iostream>

struct CUpdatedBlock {
    uint256 hash;
    int height;
};

static Mutex cs_blockchange;
static std::condition_variable cond_blockchange;
static CUpdatedBlock latestblock;

double AmountAsPercent(Amount v, Amount ref) {  return 0.1*int(1000.0*double(v.toInt())/ref.toInt());}  

static int ComputeNextBlockAndDepth(const CBlockIndex *tip,
                                    const CBlockIndex *blockindex,
                                    const CBlockIndex *&next) {
    next = tip->GetAncestor(blockindex->nHeight + 1);
    if (next && next->pprev == blockindex) {
        return tip->nHeight - blockindex->nHeight + 1;
    }
    next = nullptr;
    return blockindex == tip ? 1 : -1;
}

UniValue blockheaderToJSON(const CBlockIndex *tip,
                           const CBlockIndex *blockindex) {
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    const CBlockIndex *pnext;
    int confirmations = ComputeNextBlockAndDepth(tip, blockindex, pnext);
    result.pushKV("confirmations", confirmations);
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", blockindex->nVersion);
    result.pushKV("versionHex", strprintf("%08x", blockindex->nVersion));
    result.pushKV("merkleroot", blockindex->hashMerkleRoot.GetHex());
    result.pushKV("time", int64_t(blockindex->nTime));
    result.pushKV("mediantime", int64_t(blockindex->GetMedianTimePast()));
    result.pushKV("nonce", uint64_t(blockindex->nNonce));
    result.pushKV("bits", strprintf("%08x", blockindex->nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());

    if (blockindex->pprev) {
        result.pushKV("previousblockhash",
                      blockindex->pprev->GetBlockHash().GetHex());
    }
    if (pnext) {
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());
    }
    return result;
}

UniValue blockToJSON(const CBlock &block, const CBlockIndex *tip,
                     const CBlockIndex *blockindex, bool txDetails) {
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    const CBlockIndex *pnext;
    int confirmations = ComputeNextBlockAndDepth(tip, blockindex, pnext);
    result.pushKV("confirmations", confirmations);
    result.pushKV("size", (int)::GetSerializeSize(block, PROTOCOL_VERSION));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion);
    result.pushKV("versionHex", strprintf("%08x", block.nVersion));
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());
    UniValue txs(UniValue::VARR);
    for (const auto &tx : block.vtx) {
        if (txDetails) {
            UniValue objTx(UniValue::VOBJ);
            TxToUniv(*tx, uint256(), objTx, true, RPCSerializationFlags());
            txs.push_back(objTx);
        } else {
            txs.push_back(tx->GetId().GetHex());
        }
    }
    result.pushKV("tx", txs);
    result.pushKV("time", block.GetBlockTime());
    result.pushKV("mediantime", int64_t(blockindex->GetMedianTimePast()));
    result.pushKV("nonce", uint64_t(block.nNonce));
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());

    if (blockindex->pprev) {
        result.pushKV("previousblockhash",
                      blockindex->pprev->GetBlockHash().GetHex());
    }
    if (pnext) {
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());
    }
    return result;
}


static UniValue getblockcount(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest blockchain.\n"
            "\nResult:\n"
            "n    (numeric) The current block count\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockcount", "") +
            HelpExampleRpc("getblockcount", ""));
    }

    LOCK(cs_main);
    return chainActive.Height();
}

static UniValue getbestblockhash(const Config &config,
                                 const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getbestblockhash\n"
            "\nReturns the hash of the best (tip) block in the "
            "longest blockchain.\n"
            "\nResult:\n"
            "\"hex\"      (string) the block hash hex encoded\n"
            "\nExamples:\n" +
            HelpExampleCli("getbestblockhash", "") +
            HelpExampleRpc("getbestblockhash", ""));
    }

    LOCK(cs_main);
    return chainActive.Tip()->GetBlockHash().GetHex();
}

UniValue getfinalizedblockhash(const Config &config,
                               const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getfinalizedblockhash\n"
            "\nReturns the hash of the currently finalized block\n"
            "\nResult:\n"
            "\"hex\"      (string) the block hash hex encoded\n");
    }

    LOCK(cs_main);
    const CBlockIndex *blockIndexFinalized = GetFinalizedBlock();
    if (blockIndexFinalized) {
        return blockIndexFinalized->GetBlockHash().GetHex();
    }
    return UniValue(UniValue::VSTR);
}

void RPCNotifyBlockChange(bool ibd, const CBlockIndex *pindex) {
    if (pindex) {
        std::lock_guard<std::mutex> lock(cs_blockchange);
        latestblock.hash = pindex->GetBlockHash();
        latestblock.height = pindex->nHeight;
    }
    cond_blockchange.notify_all();
}

static UniValue waitfornewblock(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() > 1) {
        throw std::runtime_error(
            "waitfornewblock (timeout)\n"
            "\nWaits for a specific new block and returns "
            "useful info about it.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. timeout (int, optional, default=0) Time in "
            "milliseconds to wait for a response. 0 indicates "
            "no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("waitfornewblock", "1000") +
            HelpExampleRpc("waitfornewblock", "1000"));
    }

    int timeout = 0;
    if (!request.params[0].isNull()) {
        timeout = request.params[0].get_int();
    }

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        block = latestblock;
        if (timeout) {
            cond_blockchange.wait_for(
                lock, std::chrono::milliseconds(timeout), [&block] {
                    return latestblock.height != block.height ||
                           latestblock.hash != block.hash || !IsRPCRunning();
                });
        } else {
            cond_blockchange.wait(lock, [&block] {
                return latestblock.height != block.height ||
                       latestblock.hash != block.hash || !IsRPCRunning();
            });
        }
        block = latestblock;
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue waitforblock(const Config &config,
                             const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "waitforblock <blockhash> (timeout)\n"
            "\nWaits for a specific new block and returns useful info about "
            "it.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. \"blockhash\" (required, string) Block hash to wait for.\n"
            "2. timeout       (int, optional, default=0) Time in milliseconds "
            "to wait for a response. 0 indicates no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4"
                                           "570b24c9ed7b4a8c619eb02596f8862\", "
                                           "1000") +
            HelpExampleRpc("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4"
                                           "570b24c9ed7b4a8c619eb02596f8862\", "
                                           "1000"));
    }

    int timeout = 0;

    uint256 hash = uint256S(request.params[0].get_str());

    if (!request.params[1].isNull()) {
        timeout = request.params[1].get_int();
    }

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        if (timeout) {
            cond_blockchange.wait_for(
                lock, std::chrono::milliseconds(timeout), [&hash] {
                    return latestblock.hash == hash || !IsRPCRunning();
                });
        } else {
            cond_blockchange.wait(lock, [&hash] {
                return latestblock.hash == hash || !IsRPCRunning();
            });
        }
        block = latestblock;
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue waitforblockheight(const Config &config,
                                   const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "waitforblockheight <height> (timeout)\n"
            "\nWaits for (at least) block height and returns the height and "
            "hash\n"
            "of the current tip.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. height  (required, int) Block height to wait for (int)\n"
            "2. timeout (int, optional, default=0) Time in milliseconds to "
            "wait for a response. 0 indicates no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("waitforblockheight", "\"100\", 1000") +
            HelpExampleRpc("waitforblockheight", "\"100\", 1000"));
    }

    int timeout = 0;

    int height = request.params[0].get_int();

    if (!request.params[1].isNull()) {
        timeout = request.params[1].get_int();
    }

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        if (timeout) {
            cond_blockchange.wait_for(
                lock, std::chrono::milliseconds(timeout), [&height] {
                    return latestblock.height >= height || !IsRPCRunning();
                });
        } else {
            cond_blockchange.wait(lock, [&height] {
                return latestblock.height >= height || !IsRPCRunning();
            });
        }
        block = latestblock;
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue
syncwithvalidationinterfacequeue(const Config &config,
                                 const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() > 0) {
        throw std::runtime_error(
            "syncwithvalidationinterfacequeue\n"
            "\nWaits for the validation interface queue to catch up on "
            "everything that was there when we entered this function.\n"
            "\nExamples:\n" +
            HelpExampleCli("syncwithvalidationinterfacequeue", "") +
            HelpExampleRpc("syncwithvalidationinterfacequeue", ""));
    }
    SyncWithValidationInterfaceQueue();
    return NullUniValue;
}

static UniValue getdifficulty(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error("getdifficulty\n"
                                 "\nReturns the proof-of-work difficulty as a "
                                 "multiple of the minimum difficulty.\n"
                                 "\nResult:\n"
                                 "n.nnn       (numeric) the proof-of-work "
                                 "difficulty as a multiple of the minimum "
                                 "difficulty.\n"
                                 "\nExamples:\n" +
                                 HelpExampleCli("getdifficulty", "") +
                                 HelpExampleRpc("getdifficulty", ""));
    }

    LOCK(cs_main);
    return GetDifficulty(chainActive.Tip());
}

static std::string EntryDescriptionString() {
    return "    \"size\" : n,             (numeric) transaction size.\n"
           "    \"fee\" : n,              (numeric) transaction fee in " +
           CURRENCY_UNIT + "(DEPRECATED)" +
           "\n"
           "    \"modifiedfee\" : n,      (numeric) transaction fee with fee "
           "deltas used for mining priority (DEPRECATED)\n"
           "    \"time\" : n,             (numeric) local time transaction "
           "entered pool in seconds since 1 Jan 1970 GMT\n"
           "    \"height\" : n,           (numeric) block height when "
           "transaction entered pool\n"
           "    \"startingpriority\" : n, (numeric) DEPRECATED. Priority when "
           "transaction entered pool\n"
           "    \"currentpriority\" : n,  (numeric) DEPRECATED. Transaction "
           "priority now\n"
           "    \"descendantcount\" : n,  (numeric) number of in-mempool "
           "descendant transactions (including this one)\n"
           "    \"descendantsize\" : n,   (numeric) virtual transaction size "
           "of in-mempool descendants (including this one)\n"
           "    \"descendantfees\" : n,   (numeric) modified fees (see above) "
           "of in-mempool descendants (including this one) (DEPRECATED)\n"
           "    \"ancestorcount\" : n,    (numeric) number of in-mempool "
           "ancestor transactions (including this one)\n"
           "    \"ancestorsize\" : n,     (numeric) virtual transaction size "
           "of in-mempool ancestors (including this one)\n"
           "    \"ancestorfees\" : n,     (numeric) modified fees (see above) "
           "of in-mempool ancestors (including this one) (DEPRECATED)\n"
           "    \"fees\" : {\n"
           "        \"base\" : n,         (numeric) transaction fee in " +
           CURRENCY_UNIT +
           "\n"
           "        \"modified\" : n,     (numeric) transaction fee with fee "
           "deltas used for mining priority in " +
           CURRENCY_UNIT +
           "\n"
           "        \"ancestor\" : n,     (numeric) modified fees (see above) "
           "of in-mempool ancestors (including this one) in " +
           CURRENCY_UNIT +
           "\n"
           "        \"descendant\" : n,   (numeric) modified fees (see above) "
           "of in-mempool descendants (including this one) in " +
           CURRENCY_UNIT +
           "\n"
           "    }\n"
           "    \"depends\" : [           (array) unconfirmed transactions "
           "used as inputs for this transaction\n"
           "        \"transactionid\",    (string) parent transaction id\n"
           "       ... ]\n";
}

static void entryToJSON(UniValue &info, const CTxMemPoolEntry &e)
    EXCLUSIVE_LOCKS_REQUIRED(g_mempool.cs) {
    AssertLockHeld(g_mempool.cs);

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(e.GetModFeesWithAncestors()));
    fees.pushKV("descendant", ValueFromAmount(e.GetModFeesWithDescendants()));
    info.pushKV("fees", fees);

    info.pushKV("size", (int)e.GetTxSize());
    info.pushKV("fee", ValueFromAmount(e.GetFee()));
    info.pushKV("modifiedfee", ValueFromAmount(e.GetModifiedFee()));
    info.pushKV("time", e.GetTime());
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("startingpriority", e.GetPriority(e.GetHeight()));
    info.pushKV("currentpriority", e.GetPriority(chainActive.Height()));
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    info.pushKV("descendantfees", e.GetModFeesWithDescendants().toInt());
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    info.pushKV("ancestorfees", e.GetModFeesWithAncestors().toInt());
    const CTransaction &tx = e.GetTx();
    std::set<std::string> setDepends;
    for (const CTxIn &txin : tx.vin) {
        if (g_mempool.exists(txin.prevout.GetTxId())) {
            setDepends.insert(txin.prevout.GetTxId().ToString());
        }
    }

    UniValue depends(UniValue::VARR);
    for (const std::string &dep : setDepends) {
        depends.push_back(dep);
    }

    info.pushKV("depends", depends);
}

UniValue mempoolToJSON(bool fVerbose) {
    if (fVerbose) {
        LOCK(g_mempool.cs);
        UniValue o(UniValue::VOBJ);
        for (const CTxMemPoolEntry &e : g_mempool.mapTx) {
            const uint256 &txid = e.GetTx().GetId();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(txid.ToString(), info);
        }
        return o;
    } else {
        std::vector<uint256> vtxids;
        g_mempool.queryHashes(vtxids);

        UniValue a(UniValue::VARR);
        for (const uint256 &txid : vtxids) {
            a.push_back(txid.ToString());
        }

        return a;
    }
}

static UniValue getrawmempool(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() > 1) {
        throw std::runtime_error(
            "getrawmempool ( verbose )\n"
            "\nReturns all transaction ids in memory pool as a json array of "
            "string transaction ids.\n"
            "\nHint: use getmempoolentry to fetch a specific transaction from "
            "the mempool.\n"
            "\nArguments:\n"
            "1. verbose (boolean, optional, default=false) True for a json "
            "object, false for array of transaction ids\n"
            "\nResult: (for verbose = false):\n"
            "[                     (json array of string)\n"
            "  \"transactionid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n" +
            EntryDescriptionString() +
            "  }, ...\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getrawmempool", "true") +
            HelpExampleRpc("getrawmempool", "true"));
    }

    bool fVerbose = false;
    if (!request.params[0].isNull()) {
        fVerbose = request.params[0].get_bool();
    }

    return mempoolToJSON(fVerbose);
}

static UniValue getmempoolancestors(const Config &config,
                                    const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "getmempoolancestors txid (verbose)\n"
            "\nIf txid is in the mempool, returns all in-mempool ancestors.\n"
            "\nArguments:\n"
            "1. \"txid\"                 (string, required) The transaction id "
            "(must be in mempool)\n"
            "2. verbose                  (boolean, optional, default=false) "
            "True for a json object, false for array of transaction ids\n"
            "\nResult (for verbose=false):\n"
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an "
            "in-mempool ancestor transaction\n"
            "  ,...\n"
            "]\n"
            "\nResult (for verbose=true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n" +
            EntryDescriptionString() +
            "  }, ...\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getmempoolancestors", "\"mytxid\"") +
            HelpExampleRpc("getmempoolancestors", "\"mytxid\""));
    }

    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].get_bool();
    }

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(g_mempool.cs);

    CTxMemPool::txiter it = g_mempool.mapTx.find(hash);
    if (it == g_mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Transaction not in mempool");
    }

    CTxMemPool::setEntries setAncestors;
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    g_mempool.CalculateMemPoolAncestors(*it, setAncestors, noLimit, noLimit,
                                        noLimit, noLimit, dummy, false);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            o.push_back(ancestorIt->GetTx().GetId().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256 &_hash = e.GetTx().GetId();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
}

static UniValue getmempooldescendants(const Config &config,
                                      const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "getmempooldescendants txid (verbose)\n"
            "\nIf txid is in the mempool, returns all in-mempool descendants.\n"
            "\nArguments:\n"
            "1. \"txid\"                 (string, required) The transaction id "
            "(must be in mempool)\n"
            "2. verbose                  (boolean, optional, default=false) "
            "True for a json object, false for array of transaction ids\n"
            "\nResult (for verbose=false):\n"
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an "
            "in-mempool descendant transaction\n"
            "  ,...\n"
            "]\n"
            "\nResult (for verbose=true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n" +
            EntryDescriptionString() +
            "  }, ...\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getmempooldescendants", "\"mytxid\"") +
            HelpExampleRpc("getmempooldescendants", "\"mytxid\""));
    }

    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].get_bool();
    }

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(g_mempool.cs);

    CTxMemPool::txiter it = g_mempool.mapTx.find(hash);
    if (it == g_mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    g_mempool.CalculateDescendants(it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(it);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            o.push_back(descendantIt->GetTx().GetId().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256 &_hash = e.GetTx().GetId();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
}

static UniValue getmempoolentry(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "getmempoolentry txid\n"
            "\nReturns mempool data for given transaction\n"
            "\nArguments:\n"
            "1. \"txid\"                   (string, required) "
            "The transaction id (must be in mempool)\n"
            "\nResult:\n"
            "{                           (json object)\n" +
            EntryDescriptionString() +
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getmempoolentry", "\"mytxid\"") +
            HelpExampleRpc("getmempoolentry", "\"mytxid\""));
    }

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(g_mempool.cs);

    CTxMemPool::txiter it = g_mempool.mapTx.find(hash);
    if (it == g_mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    entryToJSON(info, e);
    return info;
}

static UniValue getblockhash(const Config &config,
                             const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "getblockhash height\n"
            "\nReturns hash of block in best-block-chain at height provided.\n"
            "\nArguments:\n"
            "1. height         (numeric, required) The height index\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockhash", "1000") +
            HelpExampleRpc("getblockhash", "1000"));
    }

    LOCK(cs_main);

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    CBlockIndex *pblockindex = chainActive[nHeight];
    return pblockindex->GetBlockHash().GetHex();
}

static UniValue getblockheader(const Config &config,
                               const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "getblockheader \"hash\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, "
            "hex-encoded data for blockheader 'hash'.\n"
            "If verbose is true, returns an Object with information about "
            "blockheader <hash>.\n"
            "\nArguments:\n"
            "1. \"hash\"          (string, required) The block hash\n"
            "2. verbose           (boolean, optional, default=true) true for a "
            "json object, false for the hex encoded data\n"
            "\nResult (for verbose = true):\n"
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as "
            "provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, "
            "or -1 if the block is not on the main chain\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version "
            "formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds "
            "since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in "
            "seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"0000...1f3\"     (string) Expected number of "
            "hashes required to produce the current chain (in hex)\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the "
            "previous block\n"
            "  \"nextblockhash\" : \"hash\",      (string) The hash of the "
            "next block\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, "
            "hex-encoded data for block 'hash'.\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockheader", "\"00000000c937983704a73af28acdec3"
                                             "7b049d214adbda81d7e2a3dd146f6ed09"
                                             "\"") +
            HelpExampleRpc("getblockheader", "\"00000000c937983704a73af28acdec3"
                                             "7b049d214adbda81d7e2a3dd146f6ed09"
                                             "\""));
    }

    LOCK(cs_main);

    std::string strHash = request.params[0].get_str();
    BlockHash hash(uint256S(strHash));

    bool fVerbose = true;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].get_bool();
    }

    if (mapBlockIndex.count(hash) == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    CBlockIndex *pblockindex = mapBlockIndex[hash];

    if (!fVerbose) {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << pblockindex->GetBlockHeader();
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockheaderToJSON(chainActive.Tip(), pblockindex);
}

static UniValue getdifficulties(const Config &config, const JSONRPCRequest &request) {
  if (request.fHelp || request.params.size() < 1)
    throw std::runtime_error(
                             "getdifficulties \"filename\" \"blocks\" \n"
                             "\nWrite the last \"blocks\" difficulty values to a file for analysis.\n"
                             "\nArguments:\n"
                             "1. \"filename\"       (string, required) The filename where results are written to\n"
                             "2. \"blocks\"         (int, optional, default=720) amount of blocks processed\n"
                             "\nExamples:\n" +
                             HelpExampleCli("getdifficulties", "\"diff.txt") +
                             HelpExampleRpc("getdifficulties", "\"diff200.txt\" 200"));

    LOCK(cs_main);
    fs::path filepath = request.params[0].get_str();
    int nLastHeight = chainActive.Height();
    int nHeight=1;

    if (!request.params[1].isNull()) {
      nHeight = nLastHeight - request.params[1].get_int();
    } else {
      nHeight = std::max(nLastHeight - 720,1);
    }
    if (nHeight < 0 || nHeight > chainActive.Height()) {
      throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }
  
    filepath = fs::absolute(filepath);
    
    std::ofstream file;
    file.open(filepath.string().c_str());
    if (!file.is_open()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Cannot open getdifficulties dump file");
    }

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.string());

    int start_time = 0;
    int sum_time_diff = 0;
    int sq_time = 0;
    int prev_time=0;
    double av_diff=0;
    double diff=0;
    for (int i=nHeight;i<=nLastHeight;i++) {
      CBlockIndex *pblockindex = chainActive[i];
      if (fHavePruned && !pblockindex->nStatus.hasData() &&
          pblockindex->nTx > 0) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
      }
      CBlock block;
      if (!ReadBlockFromDisk(block, pblockindex, config)) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
      }

      // We grab Height + double precision Difficulty from the block
      // and save in a file
      
      if (i==nHeight) {
        prev_time = start_time = block.GetBlockTime();
      }
      int time_diff = block.GetBlockTime() - prev_time;

      sum_time_diff += time_diff;
      sq_time += (time_diff*time_diff);
      // Put points ~10 seconds apart for granularity and
      // store time as minutes
      for (int j=0;j<time_diff;j += 10) {
        file << (prev_time - start_time + j)/60.0 << " " << diff << "\n";
      }
      diff = GetDifficulty(pblockindex);
      av_diff += diff;
      prev_time = block.GetBlockTime();
    }
    file.close();

    int blocks = nLastHeight - nHeight;
    double exsq = (double)sq_time/blocks;
    double av = (double)sum_time_diff/blocks;
    double sqex = av*av;
    double rms = sqrt(exsq - sqex);
    
    reply.pushKV("average block time (seconds)", int(av+0.5));
    reply.pushKV("std. deviation of block time (seconds)", int(rms+0.5));
    reply.pushKV("average difficulty", (int)(av_diff/blocks));
    reply.pushKV("last difficulty", (int)diff);
    reply.pushKV("blocks used", blocks);

    return reply;
}

static CBlock GetBlockChecked(const Config &config,
                              const CBlockIndex *pblockindex) {
    CBlock block;
    if (fHavePruned && !pblockindex->nStatus.hasData() &&
        pblockindex->nTx > 0) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, config)) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    return block;
}


static UniValue getblock(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "getblock \"blockhash\" ( verbosity )\n"
            "\nIf verbosity is 0 or false, returns a string that is "
            "serialized, hex-encoded data for block 'hash'.\n"
            "If verbosity is 1 or true, returns an Object with information "
            "about block <hash>.\n"
            "If verbosity is 2, returns an Object with information about block "
            "<hash> and information about each transaction.\n"
            "\nArguments:\n"
            "1. \"blockhash\"           (string, required) The block hash\n"
            "2. verbosity             (numeric, optional, default=1) 0 for "
            "hex-encoded data, 1 for a json object, and 2 for json object with "
            "transaction data\n"
            "\nResult (for verbosity = 0):\n"
            "\"data\"                   (string) A string that is serialized, "
            "hex-encoded data for block 'hash'.\n"
            "\nResult (for verbosity = 1):\n"
            "{\n"
            "  \"hash\" : \"hash\",       (string) The block hash (same as "
            "provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, "
            "or -1 if the block is not on the main chain\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version "
            "formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds "
            "since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in "
            "seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\",   (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"xxxx\",  (string) Expected number of hashes "
            "required to produce the chain up to this block (in hex)\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the "
            "previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the "
            "next block\n"
            "}\n"
            "\nResult (for verbosity = 2):\n"
            "{\n"
            "  ...,                   Same output as verbosity = 1\n"
            "  \"tx\" : [               (array of Objects) The transactions in "
            "the format of the getrawtransaction RPC; different from verbosity "
            "= 1 \"tx\" result\n"
            "    ...\n"
            "  ],\n"
            "  ...                    Same output as verbosity = 1\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d"
                                       "214adbda81d7e2a3dd146f6ed09\"") +
            HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d"
                                       "214adbda81d7e2a3dd146f6ed09\""));
    }

    LOCK(cs_main);

    std::string strHash = request.params[0].get_str();
    BlockHash hash(uint256S(strHash));

    int verbosity = 1;
    if (!request.params[1].isNull()) {
        if (request.params[1].isNum()) {
            verbosity = request.params[1].get_int();
        } else {
            verbosity = request.params[1].get_bool() ? 1 : 0;
        }
    }

    if (mapBlockIndex.count(hash) == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    
    CBlockIndex *pblockindex = mapBlockIndex[hash];
    CBlock block = GetBlockChecked(config, pblockindex);

    if (verbosity <= 0) {
        CDataStream ssBlock(SER_NETWORK,
                            PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, chainActive.Tip(), pblockindex, verbosity >= 2);
}

static UniValue getblockbynumber(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 2) {
        throw std::runtime_error(
            "getblockbynumber \"blocknumber\" ( verbosity )\n"
            "\nIf verbosity is 0 or false, returns a string that is "
            "serialized, hex-encoded data for block 'hash'.\n"
            "If verbosity is 1 or true, returns an Object with information "
            "about block <hash>.\n"
            "If verbosity is 2, returns an Object with information about block "
            "<hash> and information about each transaction.\n"
            "\nArguments:\n"
            "1. \"blocknumber\"           (string, required) The block number\n"
            "2. verbosity             (numeric, optional, default=1) 0 for "
            "hex-encoded data, 1 for a json object, and 2 for json object with "
            "transaction data\n"
            "\nResult (for verbosity = 0):\n"
            "\"data\"                   (string) A string that is serialized, "
            "hex-encoded data for block 'hash'.\n"
            "\nResult (for verbosity = 1):\n"
            "{\n"
            "  \"hash\" : \"hash\",       (string) The block hash (same as "
            "provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, "
            "or -1 if the block is not on the main chain\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version "
            "formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds "
            "since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in "
            "seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\",   (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"xxxx\",  (string) Expected number of hashes "
            "required to produce the chain up to this block (in hex)\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the "
            "previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the "
            "next block\n"
            "}\n"
            "\nResult (for verbosity = 2):\n"
            "{\n"
            "  ...,                   Same output as verbosity = 1\n"
            "  \"tx\" : [               (array of Objects) The transactions in "
            "the format of the getrawtransaction RPC; different from verbosity "
            "= 1 \"tx\" result\n"
            "    ...\n"
            "  ],\n"
            "  ...                    Same output as verbosity = 1\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockbynumber", "1000") +
            HelpExampleRpc("getblockbynumber", "1000"));
    }

    LOCK(cs_main);

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }
    CBlockIndex *pblockindex = chainActive[nHeight];
    auto hash = pblockindex->GetBlockHash(); 

    int verbosity = 1;
    if (!request.params[1].isNull()) {
        if (request.params[1].isNum()) {
            verbosity = request.params[1].get_int();
        } else {
            verbosity = request.params[1].get_bool() ? 1 : 0;
        }
    }

    if (mapBlockIndex.count(hash) == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    CBlock block = GetBlockChecked(config, pblockindex);

    if (verbosity <= 0) {
        CDataStream ssBlock(SER_NETWORK,
                            PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, chainActive.Tip(), pblockindex, verbosity >= 2);
}

static UniValue pruneblockchain(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "pruneblockchain\n"
            "\nArguments:\n"
            "1. \"height\"       (numeric, required) The block height to prune "
            "up to. May be set to a discrete height, or a unix timestamp\n"
            "                  to prune blocks whose block time is at least 2 "
            "hours older than the provided timestamp.\n"
            "\nResult:\n"
            "n    (numeric) Height of the last block pruned.\n"
            "\nExamples:\n" +
            HelpExampleCli("pruneblockchain", "1000") +
            HelpExampleRpc("pruneblockchain", "1000"));
    }

    if (!fPruneMode) {
        throw JSONRPCError(
            RPC_MISC_ERROR,
            "Cannot prune blocks because node is not in prune mode.");
    }

    LOCK(cs_main);

    int heightParam = request.params[0].get_int();
    if (heightParam < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative block height.");
    }

    // Height value more than a billion is too high to be a block height, and
    // too low to be a block time (corresponds to timestamp from Sep 2001).
    if (heightParam > 1000000000) {
        // Add a 2 hour buffer to include blocks which might have had old
        // timestamps
        CBlockIndex *pindex =
            chainActive.FindEarliestAtLeast(heightParam - TIMESTAMP_WINDOW);
        if (!pindex) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                "Could not find block with at least the specified timestamp.");
        }
        heightParam = pindex->nHeight;
    }

    unsigned int height = (unsigned int)heightParam;
    unsigned int chainHeight = (unsigned int)chainActive.Height();
    if (chainHeight < config.GetChainParams().PruneAfterHeight()) {
        throw JSONRPCError(RPC_MISC_ERROR,
                           "Blockchain is too short for pruning.");
    } else if (height > chainHeight) {
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            "Blockchain is shorter than the attempted prune height.");
    } else if (height > chainHeight - MIN_BLOCKS_TO_KEEP) {
        LogPrint(BCLog::RPC, "Attempt to prune blocks close to the tip. "
                             "Retaining the minimum number of blocks.");
        height = chainHeight - MIN_BLOCKS_TO_KEEP;
    }

    PruneBlockFilesManual(height);
    return uint64_t(height);
}

static UniValue gettxoutsetinfo(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "gettxoutsetinfo\n"
            "\nReturns statistics about the unspent transaction output set.\n"
            "Note this call may take some time.\n"
            "\nResult:\n"
            "{\n"
            "  \"height\":n,     (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",   (string) the best block hash hex\n"
            "  \"transactions\": n,      (numeric) The number of transactions\n"
            "  \"txouts\": n,            (numeric) The number of output "
            "transactions\n"
            "  \"bogosize\": n,          (numeric) A database-independent "
            "metric for UTXO set size\n"
            "  \"hash_serialized\": \"hash\",   (string) The serialized hash\n"
            "  \"disk_size\": n,         (numeric) The estimated size of the "
            "chainstate on disk\n"
            "  \"total_amount\": x.xxx          (numeric) The total amount\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("gettxoutsetinfo", "") +
            HelpExampleRpc("gettxoutsetinfo", ""));
    }

    UniValue ret(UniValue::VOBJ);

    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsdbview.get(), stats)) {
        ret.pushKV("height", int64_t(stats.nHeight));
        ret.pushKV("bestblock", stats.hashBlock.GetHex());
        ret.pushKV("transactions", int64_t(stats.nTransactions));
        ret.pushKV("txouts", int64_t(stats.nTransactionOutputs));
        ret.pushKV("bogosize", int64_t(stats.nBogoSize));
        ret.pushKV("hash_serialized", stats.hashSerialized.GetHex());
        ret.pushKV("disk_size", stats.nDiskSize);
        ret.pushKV("total_amount", ValueFromAmount(stats.nTotalAmount));
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }
    return ret;
}

UniValue gettxout(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 2 ||
        request.params.size() > 3) {
        throw std::runtime_error(
            "gettxout \"txid\" n ( include_mempool )\n"
            "\nReturns details about an unspent transaction output.\n"
            "\nArguments:\n"
            "1. \"txid\"             (string, required) The transaction id\n"
            "2. \"n\"                (numeric, required) vout number\n"
            "3. \"include_mempool\"  (boolean, optional) Whether to include "
            "the mempool. Default: true."
            "     Note that an unspent output that is spent in the mempool "
            "won't appear.\n"
            "\nResult:\n"
            "{\n"
            "  \"bestblock\" : \"hash\",    (string) the block hash\n"
            "  \"confirmations\" : n,       (numeric) The number of "
            "confirmations\n"
            "  \"value\" : x.xxx,           (numeric) The transaction value "
            "in " +
            CURRENCY_UNIT +
            "\n"
            "  \"scriptPubKey\" : {         (json object)\n"
            "     \"asm\" : \"code\",       (string) \n"
            "     \"hex\" : \"hex\",        (string) \n"
            "     \"reqSigs\" : n,          (numeric) Number of required "
            "signatures\n"
            "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
            "     \"addresses\" : [          (array of string) array of "
            "DeVault addresses\n"
            "        \"address\"     (string) DeVault address\n"
            "        ,...\n"
            "     ]\n"
            "  },\n"
            "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
            "}\n"

            "\nExamples:\n"
            "\nGet unspent transactions\n" +
            HelpExampleCli("listunspent", "") + "\nView the details\n" +
            HelpExampleCli("gettxout", "\"txid\" 1") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("gettxout", "\"txid\", 1"));
    }

    LOCK(cs_main);

    UniValue ret(UniValue::VOBJ);

    std::string strTxId = request.params[0].get_str();
    TxId txid(uint256S(strTxId));
    int n = request.params[1].get_int();
    COutPoint out(txid, n);
    bool fMempool = true;
    if (!request.params[2].isNull()) {
        fMempool = request.params[2].get_bool();
    }

    Coin coin;
    if (fMempool) {
        LOCK(g_mempool.cs);
        CCoinsViewMemPool view(pcoinsTip.get(), g_mempool);
        if (!view.GetCoin(out, coin) || g_mempool.isSpent(out)) {
            // TODO: this should be done by the CCoinsViewMemPool
            return NullUniValue;
        }
    } else {
        if (!pcoinsTip->GetCoin(out, coin)) {
            return NullUniValue;
        }
    }

    auto it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    CBlockIndex *pindex = it->second;
    ret.pushKV("bestblock", pindex->GetBlockHash().GetHex());
    if (coin.GetHeight() == MEMPOOL_HEIGHT) {
        ret.pushKV("confirmations", 0);
    } else {
        ret.pushKV("confirmations",
                   int64_t(pindex->nHeight - coin.GetHeight() + 1));
    }
    ret.pushKV("value", ValueFromAmount(coin.GetTxOut().nValue));
    UniValue o(UniValue::VOBJ);
    ScriptPubKeyToUniv(coin.GetTxOut().scriptPubKey, o, true);
    ret.pushKV("scriptPubKey", o);
    ret.pushKV("coinbase", coin.IsCoinBase());

    return ret;
}

static UniValue verifychain(const Config &config,
                            const JSONRPCRequest &request) {
    int nCheckLevel = gArgs.GetArg("-checklevel", DEFAULT_CHECKLEVEL);
    int nCheckDepth = gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS);
    if (request.fHelp || request.params.size() > 2) {
        throw std::runtime_error(
            "verifychain ( checklevel nblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1. checklevel   (numeric, optional, 0-4, default=" +
            strprintf("%d", nCheckLevel) +
            ") How thorough the block verification is.\n"
            "2. nblocks      (numeric, optional, default=" +
            strprintf("%d", nCheckDepth) +
            ", 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified or not\n"
            "\nExamples:\n" +
            HelpExampleCli("verifychain", "") +
            HelpExampleRpc("verifychain", ""));
    }

    LOCK(cs_main);

    if (!request.params[0].isNull()) {
        nCheckLevel = request.params[0].get_int();
    }
    if (!request.params[1].isNull()) {
        nCheckDepth = request.params[1].get_int();
    }

    bool ret = CVerifyDB().VerifyDB(config, pcoinsTip.get(), nCheckLevel, nCheckDepth);
    if (!ret) throw JSONRPCError(RPC_MISC_ERROR, "Failed Verifychain Check");
    return ret;
}

UniValue getblockchaininfo(const Config &config,
                           const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getblockchaininfo\n"
            "Returns an object containing various state info regarding "
            "blockchain processing.\n"
            "\nResult:\n"
            "{\n"
            "  \"chain\": \"xxxx\",        (string) current network name as "
            "defined in BIP70 (main, test, regtest)\n"
            "  \"blocks\": xxxxxx,         (numeric) the current number of "
            "blocks processed in the server\n"
            "  \"headers\": xxxxxx,        (numeric) the current number of "
            "headers we have validated\n"
            "  \"bestblockhash\": \"...\", (string) the hash of the currently "
            "best block\n"
            "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
            "  \"mediantime\": xxxxxx,     (numeric) median time for the "
            "current best block\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of "
            "verification progress [0..1]\n"
            "  \"initialblockdownload\": xxxx, (bool) (debug information) "
            "estimate of whether this node is in Initial Block Download mode.\n"
            "  \"chainwork\": \"xxxx\"     (string) total amount of work in "
            "active chain, in hexadecimal\n"
            "  \"pruned\": xx,             (boolean) if the blocks are subject "
            "to pruning\n"
            "  \"pruneheight\": xxxxxx,    (numeric) lowest-height complete "
            "block stored\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockchaininfo", "") +
            HelpExampleRpc("getblockchaininfo", ""));
    }

    LOCK(cs_main);

    const CBlockIndex *tip = chainActive.Tip();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("chain", config.GetChainParams().NetworkIDString());
    obj.pushKV("blocks", int(chainActive.Height()));
    obj.pushKV("headers", pindexBestHeader ? pindexBestHeader->nHeight : -1);
    obj.pushKV("bestblockhash", tip->GetBlockHash().GetHex());
    obj.pushKV("difficulty", double(GetDifficulty(tip)));
    obj.pushKV("mediantime", int64_t(tip->GetMedianTimePast()));
    obj.pushKV("verificationprogress",
               GuessVerificationProgress(Params().TxData(), tip));
    obj.pushKV("initialblockdownload", IsInitialBlockDownload());
    obj.pushKV("chainwork", tip->nChainWork.GetHex());
    //    obj.pushKV("size_on_disk", CalculateCurrentUsage());

    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsdbview.get(), stats)) {
      obj.pushKV("coinsupply", ValueFromAmount(stats.nTotalAmount));
    }
 
    
    obj.pushKV("pruned", fPruneMode);

    if (fPruneMode) {
        const CBlockIndex *block = tip;
        assert(block);
        while (block->pprev && (block->pprev->nStatus.hasData())) {
            block = block->pprev;
        }
        if (block) 
            obj.pushKV("pruneheight", block->nHeight);
    }
    obj.pushKV("warnings", GetWarnings("statusbar"));
    return obj;
}

/** Comparison function for sorting the getchaintips heads.  */
struct CompareBlocksByHeight {
    bool operator()(const CBlockIndex *a, const CBlockIndex *b) const {
        // Make sure that unequal blocks with the same height do not compare
        // equal. Use the pointers themselves to make a distinction.
        if (a->nHeight != b->nHeight) {
            return (a->nHeight > b->nHeight);
        }

        return a < b;
    }
};

static UniValue getchaintips(const Config &config,
                             const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getchaintips\n"
            "Return information about all known tips in the block tree,"
            " including the main chain as well as orphaned branches.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"height\": xxxx,         (numeric) height of the chain tip\n"
            "    \"hash\": \"xxxx\",         (string) block hash of the tip\n"
            "    \"branchlen\": 0          (numeric) zero for main chain\n"
            "    \"status\": \"active\"      (string) \"active\" for the main "
            "chain\n"
            "  },\n"
            "  {\n"
            "    \"height\": xxxx,\n"
            "    \"hash\": \"xxxx\",\n"
            "    \"branchlen\": 1          (numeric) length of branch "
            "connecting the tip to the main chain\n"
            "    \"status\": \"xxxx\"        (string) status of the chain "
            "(active, valid-fork, valid-headers, headers-only, invalid)\n"
            "  }\n"
            "]\n"
            "Possible values for status:\n"
            "1.  \"invalid\"               This branch contains at least one "
            "invalid block\n"
            "2.  \"parked\"                This branch contains at least one "
            "parked block\n"
            "3.  \"headers-only\"          Not all blocks for this branch are "
            "available, but the headers are valid\n"
            "4.  \"valid-headers\"         All blocks are available for this "
            "branch, but they were never fully validated\n"
            "5.  \"valid-fork\"            This branch is not part of the "
            "active chain, but is fully validated\n"
            "6.  \"active\"                This is the tip of the active main "
            "chain, which is certainly valid\n"
            "\nExamples:\n" +
            HelpExampleCli("getchaintips", "") +
            HelpExampleRpc("getchaintips", ""));
    }

    LOCK(cs_main);

    /**
     * Idea:  the set of chain tips is chainActive.tip, plus orphan blocks which
     * do not have another orphan building off of them.
     * Algorithm:
     *  - Make one pass through mapBlockIndex, picking out the orphan blocks,
     * and also storing a set of the orphan block's pprev pointers.
     *  - Iterate through the orphan blocks. If the block isn't pointed to by
     * another orphan, it is a chain tip.
     *  - add chainActive.Tip()
     */
    std::set<const CBlockIndex *, CompareBlocksByHeight> setTips;
    std::set<const CBlockIndex *> setOrphans;
    std::set<const CBlockIndex *> setPrevs;

    for (const std::pair<const BlockHash, CBlockIndex *> &item :
         mapBlockIndex) {
        if (!chainActive.Contains(item.second)) {
            setOrphans.insert(item.second);
            setPrevs.insert(item.second->pprev);
        }
    }

    for (auto& it : setOrphans) {
        if (setPrevs.erase(it) == 0) {
            setTips.insert(it);
        }
    }

    // Always report the currently active tip.
    setTips.insert(chainActive.Tip());

    /* Construct the output array.  */
    UniValue res(UniValue::VARR);
    for (const CBlockIndex *block : setTips) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("height", block->nHeight);
        obj.pushKV("hash", block->phashBlock->GetHex());

        const int branchLen =
            block->nHeight - chainActive.FindFork(block)->nHeight;
        obj.pushKV("branchlen", branchLen);

        std::string status;
        if (chainActive.Contains(block)) {
            // This block is part of the currently active chain.
            status = "active";
        } else if (block->nStatus.isInvalid()) {
            // This block or one of its ancestors is invalid.
            status = "invalid";
        } else if (block->nStatus.isOnParkedChain()) {
            // This block or one of its ancestors is parked.
            status = "parked";
        } else if (block->nChainTx == 0) {
            // This block cannot be connected because full block data for it or
            // one of its parents is missing.
            status = "headers-only";
        } else if (block->IsValid(BlockValidity::SCRIPTS)) {
            // This block is fully validated, but no longer part of the active
            // chain. It was probably the active block once, but was
            // reorganized.
            status = "valid-fork";
        } else if (block->IsValid(BlockValidity::TREE)) {
            // The headers for this block are valid, but it has not been
            // validated. It was probably never part of the most-work chain.
            status = "valid-headers";
        } else {
            // No clue.
            status = "unknown";
        }
        obj.pushKV("status", status);

        res.push_back(obj);
    }

    return res;
}

UniValue MempoolInfoToJSON(const CTxMemPool &pool) {
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("loaded", pool.IsLoaded());
    ret.pushKV("size", (int64_t)pool.size());
    ret.pushKV("bytes", (int64_t)pool.GetTotalTxSize());
    ret.pushKV("usage", (int64_t)pool.DynamicMemoryUsage());
    size_t maxmempool =
        gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    ret.pushKV("maxmempool", (int64_t)maxmempool);
    ret.pushKV("mempoolminfee",
               ValueFromAmount(
                               std::max(g_mempool.GetMinFee(maxmempool), ::minRelayTxFee)
                               .GetFeePerK()));
    ret.pushKV("minrelaytxfee", ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    return ret;
}

static UniValue getmempoolinfo(const Config &config,
                               const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getmempoolinfo\n"
            "\nReturns details on the active state of the TX memory pool.\n"
            "\nResult:\n"
            "{\n"
            "  \"loaded\": true|false         (boolean) True if the mempool is "
            "fully loaded\n"
            "  \"size\": xxxxx,               (numeric) Current tx count\n"
            "  \"bytes\": xxxxx,              (numeric) Transaction size.\n"
            "  \"usage\": xxxxx,              (numeric) Total memory usage for "
            "the mempool\n"
            "  \"maxmempool\": xxxxx,         (numeric) Maximum memory usage "
            "for the mempool\n"
            "  \"mempoolminfee\": xxxxx       (numeric) Minimum fee rate in " +
            CURRENCY_UNIT +
            "/kB for tx to be accepted. Is the maximum of minrelaytxfee and "
            "minimum mempool fee\n"
            "  \"minrelaytxfee\": xxxxx       (numeric) Current minimum relay "
            "fee for transactions\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getmempoolinfo", "") +
            HelpExampleRpc("getmempoolinfo", ""));
    }
    return MempoolInfoToJSON(::g_mempool);
}

static UniValue preciousblock(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "preciousblock \"blockhash\"\n"
            "\nTreats a block as if it were received before others with the "
            "same work.\n"
            "\nA later preciousblock call can override the effect of an "
            "earlier one.\n"
            "\nThe effects of preciousblock are not retained across restarts.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to "
            "mark as precious\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("preciousblock", "\"blockhash\"") +
            HelpExampleRpc("preciousblock", "\"blockhash\""));
    }

    std::string strHash = request.params[0].get_str();
    BlockHash hash(uint256S(strHash));
    CBlockIndex *pblockindex;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        pblockindex = mapBlockIndex[hash];
    }

    CValidationState state;
    PreciousBlock(config, state, pblockindex);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue finalizeblock(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "finalizeblock \"blockhash\"\n"

            "\nTreats a block as final. It cannot be reorged. Any chain\n"
            "that does not contain this block is invalid. Used on a less\n"
            "work chain, it can effectively PUTS YOU OUT OF CONSENSUS.\n"
            "USE WITH CAUTION!\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("finalizeblock", "\"blockhash\"") +
            HelpExampleRpc("finalizeblock", "\"blockhash\""));
    }

    std::string strHash = request.params[0].get_str();
    BlockHash hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        CBlockIndex *pblockindex = mapBlockIndex[hash];
        FinalizeBlockAndInvalidate(config, state, pblockindex);
    }

    if (state.IsValid()) {
        ActivateBestChain(config, state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

static UniValue invalidateblock(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "invalidateblock \"blockhash\"\n"
            "\nPermanently marks a block as invalid, as if it "
            "violated a consensus rule.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of "
            "the block to mark as invalid\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("invalidateblock", "\"blockhash\"") +
            HelpExampleRpc("invalidateblock", "\"blockhash\""));
    }

    const std::string strHash = request.params[0].get_str();
    const BlockHash hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        CBlockIndex *pblockindex = mapBlockIndex[hash];
        InvalidateBlock(config, state, pblockindex);
    }

    if (state.IsValid()) {
        ActivateBestChain(config, state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue parkblock(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error("parkblock \"blockhash\"\n"
                                 "\nMarks a block as parked.\n"
                                 "\nArguments:\n"
                                 "1. \"blockhash\"   (string, required) the "
                                 "hash of the block to park\n"
                                 "\nResult:\n"
                                 "\nExamples:\n" +
                                 HelpExampleCli("parkblock", "\"blockhash\"") +
                                 HelpExampleRpc("parkblock", "\"blockhash\""));
    }

    const std::string strHash = request.params[0].get_str();
    const BlockHash hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        CBlockIndex *pblockindex = mapBlockIndex[hash];
        ParkBlock(config, state, pblockindex);
    }

    if (state.IsValid()) {
        ActivateBestChain(config, state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

static UniValue reconsiderblock(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "reconsiderblock \"blockhash\"\n"
            "\nRemoves invalidity status of a block and its descendants, "
            "reconsider them for activation.\n"
            "This can be used to undo the effects of invalidateblock.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to "
            "reconsider\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("reconsiderblock", "\"blockhash\"") +
            HelpExampleRpc("reconsiderblock", "\"blockhash\""));
    }

    const std::string strHash = request.params[0].get_str();
    const BlockHash hash(uint256S(strHash));

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        CBlockIndex *pblockindex = mapBlockIndex[hash];
        ResetBlockFailureFlags(pblockindex);
    }

    CValidationState state;
    ActivateBestChain(config, state);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue unparkblock(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "unparkblock \"blockhash\"\n"
            "\nRemoves parked status of a block and its descendants, "
            "reconsider them for activation.\n"
            "This can be used to undo the effects of parkblock.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to "
            "unpark\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("unparkblock", "\"blockhash\"") +
            HelpExampleRpc("unparkblock", "\"blockhash\""));
    }

    const std::string strHash = request.params[0].get_str();
    const BlockHash hash(uint256S(strHash));

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        CBlockIndex *pblockindex = mapBlockIndex[hash];
        UnparkBlockAndChildren(pblockindex);
    }

    CValidationState state;
    ActivateBestChain(config, state);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

static UniValue getchaintxstats(const Config &config,
                                const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() > 2) {
        throw std::runtime_error(
            "getchaintxstats ( nblocks blockhash )\n"
            "\nCompute statistics about the total number and rate of "
            "transactions in the chain.\n"
            "\nArguments:\n"
            "1. nblocks      (numeric, optional) Size of the window in number "
            "of blocks (default: one month).\n"
            "2. \"blockhash\"  (string, optional) The hash of the block that "
            "ends the window.\n"
            "\nResult:\n"
            "{\n"
            "  \"time\": xxxxx,                         (numeric) The "
            "timestamp for the final block in the window in UNIX format.\n"
            "  \"txcount\": xxxxx,                      (numeric) The total "
            "number of transactions in the chain up to that point.\n"
            "  \"window_final_block_hash\": \"...\",      (string) The hash of "
            "the final block in the window.\n"
            "  \"window_block_count\": xxxxx,           (numeric) Size of "
            "the window in number of blocks.\n"
            "  \"window_tx_count\": xxxxx,              (numeric) The number "
            "of transactions in the window. Only returned if "
            "\"window_block_count\" is > 0.\n"
            "  \"window_interval\": xxxxx,              (numeric) The elapsed "
            "time in the window in seconds. Only returned if "
            "\"window_block_count\" is > 0.\n"
            "  \"txrate\": x.xx,                        (numeric) The average "
            "rate of transactions per second in the window. Only returned if "
            "\"window_interval\" is > 0.\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getchaintxstats", "") +
            HelpExampleRpc("getchaintxstats", "2016"));
    }

    const CBlockIndex *pindex;

    // By default: 1 month
    int blockcount = 30 * 24 * 60 * 60 /
                     config.GetChainParams().GetConsensus().nPowTargetSpacing;

    if (request.params[1].isNull()) {
        LOCK(cs_main);
        pindex = chainActive.Tip();
    } else {
        BlockHash hash(uint256S(request.params[1].get_str()));
        LOCK(cs_main);
        auto it = mapBlockIndex.find(hash);
        if (it == mapBlockIndex.end()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        pindex = it->second;
        if (!chainActive.Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Block is not in main chain");
        }
    }

    assert(pindex != nullptr);

    if (request.params[0].isNull()) {
        blockcount = std::max(0, std::min(blockcount, pindex->nHeight - 1));
    } else {
        blockcount = request.params[0].get_int();

        if (blockcount < 0 ||
            (blockcount > 0 && blockcount >= pindex->nHeight)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid block count: "
                                                      "should be between 0 and "
                                                      "the block's height - 1");
        }
    }

    const CBlockIndex *pindexPast =
        pindex->GetAncestor(pindex->nHeight - blockcount);
    int nTimeDiff =
        pindex->GetMedianTimePast() - pindexPast->GetMedianTimePast();
    int nTxDiff = pindex->nChainTx - pindexPast->nChainTx;

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("time", int64_t(pindex->nTime));
    ret.pushKV("txcount", int64_t(pindex->nChainTx));
    ret.pushKV("window_final_block_hash", pindex->GetBlockHash().GetHex());
    ret.pushKV("window_block_count", blockcount);
    if (blockcount > 0) {
        ret.pushKV("window_tx_count", nTxDiff);
        ret.pushKV("window_interval", nTimeDiff);
        if (nTimeDiff > 0) {
            ret.pushKV("txrate", double(nTxDiff) / nTimeDiff);
        }
    }

    return ret;
}


static UniValue getrewardinfo(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
                                 "getrewardinfo\n"
                                 "\nReturns a summary of viable rewards.\n"
                                 "{\n"
                                 "  \"sum\"  (string) The current sum of reward balances\n"
                                 "  \"count\"  (string) The total number of viable reward utxos\n"
                                 "  \"payouts\"  (string) The total number of time that rewards were paid out for current set\n"
                                 "  \"ratio\"  (string) % of coin supply in reward utxos (truncated to integer)\n"
                                 "}\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("getrewardinfo","")
                                 + HelpExampleRpc("getrewardinfo",""));
    

    std::vector<CRewardValue> rewards = prewards->GetOrderedRewards();
    UniValue result(UniValue::VOBJ);

    Amount sum;
    int count=0;
    int total_payouts=0;
    std::set<std::string> addresses;
    std::vector<Amount> all_rewards_amounts;
    for (auto& val : rewards) {
      if (val.IsActive()) {
        sum = sum + val.GetValue();
        all_rewards_amounts.push_back(val.GetValue());
        count++;
        total_payouts += val.GetPayCount();
        addresses.insert(GetAddrFromTxOut(val.GetTxOut()));
      }
    }
    Amount median;
    if (all_rewards_amounts.size() > 0) {
      sort(all_rewards_amounts.begin(),all_rewards_amounts.end());
      int mid_point = (all_rewards_amounts.size()/2);
      if (all_rewards_amounts.size() % 2 == 1) {
        median = all_rewards_amounts[mid_point];
      } else  {
        median = (all_rewards_amounts[mid_point] + all_rewards_amounts[mid_point+1])/2;
      }      
    }
    
    UniValue reply(UniValue::VOBJ);
    reply.pushKV("Current number of viable rewards", count);
    reply.pushKV("Current number of unique addresses with viable rewards", int(addresses.size()));
    reply.pushKV("Number of reward payouts to date (for current set of utxos)", total_payouts);
    reply.pushKV("Median value of viable utxo rewards", ValueFromAmount(median));

    int nHeight = chainActive.Height();

    Amount nMiningRewards;
    Amount nSuperBlockRewards;

    CBudget bud(config);

    auto Params = config.GetChainParams().GetConsensus();
    for (int i=1;i<nHeight;i++) {
      Amount nBlockSubsidy = GetBlockSubsidy(i, Params);
      nMiningRewards += nBlockSubsidy;
      if (bud.IsSuperBlock(i)) {
        Amount BudgetValue = bud.CalculateSuperBlockRewards(i, nBlockSubsidy);
        nSuperBlockRewards += BudgetValue;
      }
    }

    double ratio=0;
    Amount nTotalSupply(1); // 1 to avoid divide by 0 if GetUTXOStats fails
    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsdbview.get(), stats)) {
      nTotalSupply = stats.nTotalAmount;
      reply.pushKV("Total Coin supply", ValueFromAmount(nTotalSupply));
    }
    

    Amount nCold = stats.nTotalAmount;
    nCold -= nMiningRewards;
    nCold -= nSuperBlockRewards;
    reply.pushKV("Total Mining Rewards", ValueFromAmount(nMiningRewards));
    reply.pushKV("Total Budget Rewards", ValueFromAmount(nSuperBlockRewards));
    reply.pushKV("Total Cold Rewards paid", ValueFromAmount(nCold));
    reply.pushKV("Coin Amount in cold reward utxos", ValueFromAmount(sum));
    ratio = int(100.0*double(sum.toInt())/nTotalSupply.toInt());
    reply.pushKV("% of coins in cold reward utxos", ratio);
    reply.pushKV("Mining Rewards as % of Total Coins", AmountAsPercent(nMiningRewards,nTotalSupply));
    reply.pushKV("Budget Rewards as % of Total Coins", AmountAsPercent(nSuperBlockRewards,nTotalSupply));
    reply.pushKV("Cold Rewards as % of Total Coins", AmountAsPercent(nCold,nTotalSupply));
    reply.pushKV("Number of Blocks processed", (int)nHeight);

  
    return reply;
}

static UniValue getrewards(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
                                 "getrewards \"filename\"\n"
                                 "\nDump all of the valid reward UTXO values into a server-side file.\n"
                                 "{\nArguments"
                                 "1. \"filename\"    (string, required) The filename with path (either absolute or relative to devaultd)\n"
                                 "\nExamples:\n"
                                 + HelpExampleCli("getrewards","")
                                 + HelpExampleRpc("getrewards",""));
    

    std::vector<CRewardValue> rewards = prewards->GetOrderedRewards();
    UniValue result(UniValue::VOBJ);

    fs::path filepath = request.params[0].get_str();
    filepath = fs::absolute(filepath);
    
    std::ofstream file;
    file.open(filepath.string().c_str());
    if (!file.is_open()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Cannot open getrewards dump file");
    }

    for (auto& val : rewards) {
        file << "Value " << val.GetValue().ToString() << " ";
        file << "active "<<  val.IsActive() << " ";
        file << "creationHeight " << val.GetCreationHeight() << " ";
        file << "OldHeight " << val.GetOldHeight() << " ";
        file << "Height " << val.GetHeight() << " ";
        file << "addr " << GetAddrFromTxOut(val.GetTxOut()) << " ";
        file << "payCount " << val.GetPayCount() << "\n";
    }   
    file.close();

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.string());

    return reply;
}



template <typename T>
static T CalculateTruncatedMedian(std::vector<T> &scores) {
    size_t size = scores.size();
    if (size == 0) {
        return T();
    }

    std::sort(scores.begin(), scores.end());
    if (size % 2 == 0) {
        return (scores[size / 2 - 1] + scores[size / 2]) / 2;
    } else {
        return scores[size / 2];
    }
}

template <typename T> static inline bool SetHasKeys(const std::set<T> &set) {
    return false;
}
template <typename T, typename Tk, typename... Args>
static inline bool SetHasKeys(const std::set<T> &set, const Tk &key,
                              const Args &... args) {
    return (set.count(key) != 0) || SetHasKeys(set, args...);
}

// outpoint (needed for the utxo index) + nHeight + fCoinBase
static constexpr size_t PER_UTXO_OVERHEAD =
    sizeof(COutPoint) + sizeof(uint32_t) + sizeof(bool);

static UniValue getblockstats(const Config &config,
                              const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() < 1 ||
        request.params.size() > 4) {
        throw std::runtime_error(
            "getblockstats hash_or_height ( stats )\n"
            "\nCompute per block statistics for a given window. All amounts "
            "are in " +
            CURRENCY_UNIT +
            ".\n"
            "It won't work for some heights with pruning.\n"
            "It won't work without -txindex for utxo_size_inc, *fee or "
            "*feerate stats.\n"
            "\nArguments:\n"
            "1. \"hash_or_height\"     (string or numeric, required) The block "
            "hash or height of the target block\n"
            "2. \"stats\"              (array,  optional) Values to plot, by "
            "default all values (see result below)\n"
            "    [\n"
            "      \"height\",         (string, optional) Selected statistic\n"
            "      \"time\",           (string, optional) Selected statistic\n"
            "      ,...\n"
            "    ]\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"avgfee\": x.xxx,          (numeric) Average fee in the block\n"
            "  \"avgfeerate\": x.xxx,      (numeric) Average feerate (in " +
            CURRENCY_UNIT +
            " per byte)\n"
            "  \"avgtxsize\": xxxxx,       (numeric) Average transaction size\n"
            "  \"blockhash\": xxxxx,       (string) The block hash (to check "
            "for potential reorgs)\n"
            "  \"height\": xxxxx,          (numeric) The height of the block\n"
            "  \"ins\": xxxxx,             (numeric) The number of inputs "
            "(excluding coinbase)\n"
            "  \"maxfee\": xxxxx,          (numeric) Maximum fee in the block\n"
            "  \"maxfeerate\": xxxxx,      (numeric) Maximum feerate (in " +
            CURRENCY_UNIT +
            " per byte)\n"
            "  \"maxtxsize\": xxxxx,       (numeric) Maximum transaction size\n"
            "  \"medianfee\": x.xxx,       (numeric) Truncated median fee in "
            "the block\n"
            "  \"medianfeerate\": x.xxx,   (numeric) Truncated median feerate "
            "(in " +
            CURRENCY_UNIT +
            " per byte)\n"
            "  \"mediantime\": xxxxx,      (numeric) The block median time "
            "past\n"
            "  \"mediantxsize\": xxxxx,    (numeric) Truncated median "
            "transaction size\n"
            "  \"minfee\": x.xxx,          (numeric) Minimum fee in the block\n"
            "  \"minfeerate\": xx.xx,      (numeric) Minimum feerate (in " +
            CURRENCY_UNIT +
            " per byte)\n"
            "  \"mintxsize\": xxxxx,       (numeric) Minimum transaction size\n"
            "  \"outs\": xxxxx,            (numeric) The number of outputs\n"
            "  \"subsidy\": x.xxx,         (numeric) The block subsidy\n"
            "  \"time\": xxxxx,            (numeric) The block time\n"
            "  \"total_out\": x.xxx,       (numeric) Total amount in all "
            "outputs (excluding coinbase and thus reward [ie subsidy + "
            "totalfee])\n"
            "  \"total_size\": xxxxx,      (numeric) Total size of all "
            "non-coinbase transactions\n"
            "  \"totalfee\": x.xxx,        (numeric) The fee total\n"
            "  \"txs\": xxxxx,             (numeric) The number of "
            "transactions (excluding coinbase)\n"
            "  \"utxo_increase\": xxxxx,   (numeric) The increase/decrease in "
            "the number of unspent outputs\n"
            "  \"utxo_size_inc\": xxxxx,   (numeric) The increase/decrease in "
            "size for the utxo index (not discounting op_return and similar)\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockstats",
                           "1000 '[\"minfeerate\",\"avgfeerate\"]'") +
            HelpExampleRpc("getblockstats",
                           "1000 '[\"minfeerate\",\"avgfeerate\"]'"));
    }

    LOCK(cs_main);

    CBlockIndex *pindex;
    if (request.params[0].isNum()) {
        const int height = request.params[0].get_int();
        const int current_tip = chainActive.Height();
        if (height < 0) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                strprintf("Target block height %d is negative", height));
        }
        if (height > current_tip) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                strprintf("Target block height %d after current tip %d", height,
                          current_tip));
        }

        pindex = chainActive[height];
    } else {
        const std::string strHash = request.params[0].get_str();
        const BlockHash hash(uint256S(strHash));
        pindex = LookupBlockIndex(hash);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        if (!chainActive.Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Block is not in chain %s",
                                         Params().NetworkIDString()));
        }
    }

    assert(pindex != nullptr);

    std::set<std::string> stats;
    if (!request.params[1].isNull()) {
        const UniValue stats_univalue = request.params[1].get_array();
        for (unsigned int i = 0; i < stats_univalue.size(); i++) {
            const std::string stat = stats_univalue[i].get_str();
            stats.insert(stat);
        }
    }

    const CBlock block = GetBlockChecked(config, pindex);

    // Calculate everything if nothing selected (default)
    const bool do_all = stats.size() == 0;
    const bool do_mediantxsize = do_all || stats.count("mediantxsize") != 0;
    const bool do_medianfee = do_all || stats.count("medianfee") != 0;
    const bool do_medianfeerate = do_all || stats.count("medianfeerate") != 0;
    const bool loop_inputs =
        do_all || do_medianfee || do_medianfeerate ||
        SetHasKeys(stats, "utxo_size_inc", "totalfee", "avgfee", "avgfeerate",
                   "minfee", "maxfee", "minfeerate", "maxfeerate");
    const bool loop_outputs = do_all || loop_inputs || stats.count("total_out");
    const bool do_calculate_size =
        do_mediantxsize || loop_inputs ||
        SetHasKeys(stats, "total_size", "avgtxsize", "mintxsize", "maxtxsize");

    const int64_t blockMaxSize = config.GetMaxBlockSize();
    Amount maxfee = Amount::zero();
    Amount maxfeerate = Amount::zero();
    Amount minfee = MAX_MONEY;
    Amount minfeerate = MAX_MONEY;
    Amount total_out = Amount::zero();
    Amount totalfee = Amount::zero();
    int64_t inputs = 0;
    int64_t maxtxsize = 0;
    int64_t mintxsize = blockMaxSize;
    int64_t outputs = 0;
    int64_t total_size = 0;
    int64_t utxo_size_inc = 0;
    std::vector<Amount> fee_array;
    std::vector<Amount> feerate_array;
    std::vector<int64_t> txsize_array;

    for (const auto &tx : block.vtx) {
        outputs += tx->vout.size();
        Amount tx_total_out = Amount::zero();
        if (loop_outputs) {
            for (const CTxOut &out : tx->vout) {
                tx_total_out += out.nValue;
                utxo_size_inc +=
                    GetSerializeSize(out, PROTOCOL_VERSION) + PER_UTXO_OVERHEAD;
            }
        }

        if (tx->IsCoinBase()) {
            continue;
        }

        // Don't count coinbase's fake input
        inputs += tx->vin.size();
        // Don't count coinbase reward
        total_out += tx_total_out;

        int64_t tx_size = 0;
        if (do_calculate_size) {

            tx_size = tx->GetTotalSize();
            if (do_mediantxsize) {
                txsize_array.push_back(tx_size);
            }
            maxtxsize = std::max(maxtxsize, tx_size);
            mintxsize = std::min(mintxsize, tx_size);
            total_size += tx_size;
        }

        if (loop_inputs) {

            if (!g_txindex) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "One or more of the selected stats requires "
                                   "-txindex enabled");
            }
            Amount tx_total_in = Amount::zero();
            for (const CTxIn &in : tx->vin) {
                CTransactionRef tx_in;
                uint256 hashBlock;
                if (!GetTransaction(config, in.prevout.GetTxId(), tx_in,
                                    hashBlock, false)) {
                    throw JSONRPCError(RPC_INTERNAL_ERROR,
                                       std::string("Unexpected internal error "
                                                   "(tx index seems corrupt)"));
                }

                CTxOut prevoutput = tx_in->vout[in.prevout.GetN()];

                tx_total_in += prevoutput.nValue;
                utxo_size_inc -=
                    GetSerializeSize(prevoutput, PROTOCOL_VERSION) +
                    PER_UTXO_OVERHEAD;
            }

            Amount txfee = tx_total_in - tx_total_out;
            assert(MoneyRange(txfee));
            if (do_medianfee) {
                fee_array.push_back(txfee);
            }
            maxfee = std::max(maxfee, txfee);
            minfee = std::min(minfee, txfee);
            totalfee += txfee;

            Amount feerate = txfee / tx_size;
            if (do_medianfeerate) {
                feerate_array.push_back(feerate);
            }
            maxfeerate = std::max(maxfeerate, feerate);
            minfeerate = std::min(minfeerate, feerate);
        }
    }

    UniValue ret_all(UniValue::VOBJ);
    ret_all.pushKV("avgfee",
                   ValueFromAmount((block.vtx.size() > 1)
                                       ? totalfee / int((block.vtx.size() - 1))
                                       : Amount::zero()));
    ret_all.pushKV("avgfeerate",
                   ValueFromAmount((total_size > 0) ? totalfee / total_size
                                                    : Amount::zero()));
    ret_all.pushKV("avgtxsize", (block.vtx.size() > 1)
                                    ? total_size / (block.vtx.size() - 1)
                                    : 0);
    ret_all.pushKV("blockhash", pindex->GetBlockHash().GetHex());
    ret_all.pushKV("height", (int64_t)pindex->nHeight);
    ret_all.pushKV("ins", inputs);
    ret_all.pushKV("maxfee", ValueFromAmount(maxfee));
    ret_all.pushKV("maxfeerate", ValueFromAmount(maxfeerate));
    ret_all.pushKV("maxtxsize", maxtxsize);
    ret_all.pushKV("medianfee",
                   ValueFromAmount(CalculateTruncatedMedian(fee_array)));
    ret_all.pushKV("medianfeerate",
                   ValueFromAmount(CalculateTruncatedMedian(feerate_array)));
    ret_all.pushKV("mediantime", pindex->GetMedianTimePast());
    ret_all.pushKV("mediantxsize", CalculateTruncatedMedian(txsize_array));
    ret_all.pushKV(
        "minfee",
        ValueFromAmount((minfee == MAX_MONEY) ? Amount::zero() : minfee));
    ret_all.pushKV("minfeerate",
                   ValueFromAmount((minfeerate == MAX_MONEY) ? Amount::zero()
                                                             : minfeerate));
    ret_all.pushKV("mintxsize", mintxsize == blockMaxSize ? 0 : mintxsize);
    ret_all.pushKV("outs", outputs);
    ret_all.pushKV("subsidy", ValueFromAmount(GetBlockSubsidy(
                                  pindex->nHeight, Params().GetConsensus())));
    ret_all.pushKV("time", pindex->GetBlockTime());
    ret_all.pushKV("total_out", ValueFromAmount(total_out));
    ret_all.pushKV("total_size", total_size);
    ret_all.pushKV("totalfee", ValueFromAmount(totalfee));
    ret_all.pushKV("txs", (int64_t)block.vtx.size());
    ret_all.pushKV("utxo_increase", outputs - inputs);
    ret_all.pushKV("utxo_size_inc", utxo_size_inc);

    if (do_all) {
        return ret_all;
    }

    UniValue ret(UniValue::VOBJ);
    for (const std::string &stat : stats) {
        const UniValue &value = ret_all[stat];
        if (value.isNull()) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                strprintf("Invalid selected statistic %s", stat));
        }
        ret.pushKV(stat, value);
    }
    return ret;
}


static UniValue savemempool(const Config &config,
                            const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error("savemempool\n"
                                 "\nDumps the mempool to disk. It will fail "
                                 "until the previous dump is fully loaded.\n"
                                 "\nExamples:\n" +
                                 HelpExampleCli("savemempool", "") +
                                 HelpExampleRpc("savemempool", ""));
    }

    if (!::g_mempool.IsLoaded()) {
        throw JSONRPCError(RPC_MISC_ERROR, "The mempool was not loaded yet");
    }

    if (!DumpMempool(::g_mempool)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to dump mempool to disk");
    }

    return NullUniValue;
}

// clang-format off
static const ContextFreeRPCCommand commands[] = {
    //  category            name                      actor (function)        argNames
    //  ------------------- ------------------------  ----------------------  ----------
    { "blockchain",         "getblockchaininfo",      getblockchaininfo,      {} },
    { "blockchain",         "getchaintxstats",        &getchaintxstats,       {"nblocks", "blockhash"} },
    { "blockchain",         "getbestblockhash",       getbestblockhash,       {} },
    { "blockchain",         "getblockcount",          getblockcount,          {} },
    { "blockchain",         "getblock",               getblock,               {"blockhash","verbosity|verbose"} },
    { "blockchain",         "getblockhash",           getblockhash,           {"height"} },
    { "blockchain",         "getblockbynumber",       getblockbynumber,       {"height"} },
    { "blockchain",         "getdifficulties",        getdifficulties,        {"height"} },
    { "blockchain",         "getblockheader",         getblockheader,         {"blockhash","verbose"} },
    { "blockchain",         "getblockstats",          getblockstats,          {"hash_or_height","stats"} },
    { "blockchain",         "getchaintips",           getchaintips,           {} },
    { "blockchain",         "getdifficulty",          getdifficulty,          {} },
    { "blockchain",         "getmempoolancestors",    getmempoolancestors,    {"txid","verbose"} },
    { "blockchain",         "getmempooldescendants",  getmempooldescendants,  {"txid","verbose"} },
    { "blockchain",         "getmempoolentry",        getmempoolentry,        {"txid"} },
    { "blockchain",         "getmempoolinfo",         getmempoolinfo,         {} },
    { "blockchain",         "getrawmempool",          getrawmempool,          {"verbose"} },
    { "blockchain",         "gettxout",               gettxout,               {"txid","n","include_mempool"} },
    { "blockchain",         "gettxoutsetinfo",        gettxoutsetinfo,        {} },
    { "blockchain",         "pruneblockchain",        pruneblockchain,        {"height"} },
    { "blockchain",         "savemempool",            savemempool,            {} },
    { "blockchain",         "verifychain",            verifychain,            {"checklevel","nblocks"} },
    { "blockchain",         "preciousblock",          preciousblock,          {"blockhash"} },
    { "blockchain",         "preciousblock",          preciousblock,          {"blockhash"} },
    
    { "rewards",         "getrewards",             getrewards,             {} },
    { "rewards",         "getrewardinfo",          getrewardinfo,          {} },

    /* Not shown in help */
    { "hidden",             "getfinalizedblockhash",            getfinalizedblockhash,            {} },
    { "hidden",             "finalizeblock",                    finalizeblock,                    {"blockhash"} },
    { "hidden",             "invalidateblock",                  invalidateblock,                  {"blockhash"} },
    { "hidden",             "parkblock",                        parkblock,                        {"blockhash"} },
    { "hidden",             "reconsiderblock",                  reconsiderblock,                  {"blockhash"} },
    { "hidden",             "syncwithvalidationinterfacequeue", syncwithvalidationinterfacequeue, {} },
    { "hidden",             "unparkblock",                      unparkblock,                      {"blockhash"} },
    { "hidden",             "waitfornewblock",                  waitfornewblock,                  {"timeout"} },
    { "hidden",             "waitforblock",                     waitforblock,                     {"blockhash","timeout"} },
    { "hidden",             "waitforblockheight",               waitforblockheight,               {"height","timeout"} },
};
// clang-format on

void RegisterBlockchainRPCCommands(CRPCTable &t) {
    for (auto& command : commands) { t.appendCommand(command.name, &command); }
}
