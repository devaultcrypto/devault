// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/ft_registry.h>

#include <logging.h>
#include <primitives/block.h>
#include <util/system.h> // error()

#include <atomic>
#include <utility>

std::unique_ptr<CFtRegistry> g_ftRegistry;
std::atomic<bool> g_ftRegistrySuppress{false};

// ---------------------------------------------------------------- CFtRegistryDB

bool CFtRegistryDB::LoadAll(std::map<TxId, FtDeployRecord> &out, BlockHash &bestBlock) {
    out.clear();
    bestBlock = BlockHash();
    if (!Read(DB_FT_BESTBLOCK, bestBlock)) {
        bestBlock = BlockHash(); // absent => empty/fresh DB
    }

    std::unique_ptr<CDBIterator> it(NewIterator());
    it->Seek(std::make_pair(DB_FT_DEPLOY, TxId()));
    while (it->Valid()) {
        std::pair<char, TxId> key;
        if (!it->GetKey(key) || key.first != DB_FT_DEPLOY) {
            break;
        }
        FtDeployRecord rec;
        if (!it->GetValue(rec)) {
            return error("%s: failed to read a deploy record", __func__);
        }
        out.emplace(key.second, std::move(rec));
        it->Next();
    }
    return true;
}

bool CFtRegistryDB::BatchWrite(const std::map<TxId, FtDeployRecord> &map, const std::set<TxId> &dirty,
                               const std::set<TxId> &erased, const BlockHash &bestBlock) {
    CDBBatch batch(*this);
    for (const TxId &txid : erased) {
        batch.Erase(std::make_pair(DB_FT_DEPLOY, txid));
    }
    for (const TxId &txid : dirty) {
        auto it = map.find(txid);
        if (it == map.end()) {
            // Added and then removed again before the flush (a reorg within the flush window):
            // the erase above already covers it.
            continue;
        }
        batch.Write(std::make_pair(DB_FT_DEPLOY, txid), it->second);
    }
    batch.Write(DB_FT_BESTBLOCK, bestBlock);
    return WriteBatch(batch, /*fSync=*/true);
}

// ---------------------------------------------------------------- CFtRegistry

CFtRegistry::CFtRegistry(std::unique_ptr<CFtRegistryDB> db) : m_db(std::move(db)) {}

bool CFtRegistry::Load(BlockHash &dbBestBlock) {
    m_dirty.clear();
    m_erased.clear();
    if (!m_db->LoadAll(m_map, dbBestBlock)) {
        return false;
    }
    m_bestBlock = dbBestBlock;
    LogPrintf("FT deploy registry: loaded %d open-mint deploy(s), best block %s\n", m_map.size(),
              m_bestBlock.IsNull() ? "(none)" : m_bestBlock.ToString());
    return true;
}

const FtDeployRecord *CFtRegistry::Lookup(const TxId &deployTxid) const {
    auto it = m_map.find(deployTxid);
    return it == m_map.end() ? nullptr : &it->second;
}

void CFtRegistry::ApplyBlock(const FtBlockContext &ctx, const BlockHash &hash) {
    for (const auto &[txid, rec] : ctx.pendingDeploys) {
        // Write-once: a deploy txid is unique, so this can never overwrite a different record.
        m_map[txid] = rec;
        m_dirty.insert(txid);
        m_erased.erase(txid); // re-added after a reorg within the flush window
    }
    m_bestBlock = hash;
}

void CFtRegistry::UndoBlock(const CBlock &block, const BlockHash &prevHash) {
    for (const auto &ptx : block.vtx) {
        const TxId txid = ptx->GetId();
        if (m_map.erase(txid) > 0) {
            m_erased.insert(txid);
            m_dirty.erase(txid); // added and removed before ever being flushed
        }
    }
    m_bestBlock = prevHash;
}

bool CFtRegistry::Flush(const BlockHash &chainstateBest) {
    // The registry map and the coins view are advanced together under cs_main in
    // Connect/DisconnectBlock, so at flush time the map's tip IS the chainstate's tip. Tagging the
    // DB with the chainstate's best block is what guarantees "never ahead of the chainstate"; if
    // they ever diverged, writing records from a later state under an earlier tag would break that
    // invariant, so shout loudly rather than persist a lie.
    if (!m_bestBlock.IsNull() && m_bestBlock != chainstateBest) {
        LogPrintf("FT registry: WARNING flush tip mismatch (registry %s vs chainstate %s); "
                  "startup reconciliation will repair.\n",
                  m_bestBlock.ToString(), chainstateBest.ToString());
    }
    if (m_dirty.empty() && m_erased.empty() && m_bestBlock == chainstateBest) {
        return true; // nothing to do
    }
    if (!m_db->BatchWrite(m_map, m_dirty, m_erased, chainstateBest)) {
        return false;
    }
    m_dirty.clear();
    m_erased.clear();
    return true;
}

void CFtRegistry::Clear() {
    m_map.clear();
    m_dirty.clear();
    m_erased.clear();
    m_bestBlock = BlockHash();
}
