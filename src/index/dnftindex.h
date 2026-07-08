// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <index/base.h>
#include <primitives/transaction.h> // TxId, COutPoint
#include <serialize.h>
#include <uint256.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class CDBBatch;

static constexpr bool DEFAULT_DNFTINDEX = false;

//! Per-item record (an inscribed DNFT). The item's mint is immutable; the record is created when
//! the item is minted and removed only if that block is disconnected in a reorg.
struct DnftItemRecord {
    TxId mintTxid;
    uint32_t mintVout{0};
    int32_t height{0};
    uint32_t envelopeVout{0};
    std::vector<uint8_t> contentType;
    uint64_t contentLength{0};
    std::vector<std::vector<uint8_t>> parents; // each 65 bytes (category||commitment)

    SERIALIZE_METHODS(DnftItemRecord, obj) {
        READWRITE(obj.mintTxid, obj.mintVout, obj.height, obj.envelopeVout, obj.contentType,
                  obj.contentLength, obj.parents);
    }
};

//! Per-collection (category) record. `mintingOutpoints` are the minting-token UTXOs ever created
//! for the category; a collection is "open" iff at least one is currently unspent (checked against
//! the live UTXO set at query time, so no spend-tracking state can go stale across reorgs).
struct DnftCategoryRecord {
    TxId genesisTxid; // the tx that first created a DNFT of this category
    int32_t genesisHeight{0};
    uint64_t mintedCount{0};
    std::vector<COutPoint> mintingOutpoints;

    SERIALIZE_METHODS(DnftCategoryRecord, obj) {
        READWRITE(obj.genesisTxid, obj.genesisHeight, obj.mintedCount, obj.mintingOutpoints);
    }
};

/**
 * DnftIndex — optional, rebuildable node-side index of onchain NFT (DNFT) mint events
 * (DEVAULT_NFT_SPEC.md §11, phase 4E). It records only CREATES (mints and minting-token outpoints);
 * open/closed status is derived from the live UTXO set at query time. Built on the BaseIndex
 * framework (threading, chain-follow, rebuild on -reindex); reorgs are handled by a manual rewind
 * in WriteBlock. Mint-vs-move classification uses block undo data, matching CheckDnftRules exactly.
 */
class DnftIndex final : public BaseIndex {
    struct DB;
    const std::unique_ptr<DB> m_db;
    BlockHash m_best_block_hash; // last block applied to the index (mirrors DB key 'B')

    //! Apply (apply=true) or reverse (apply=false) one block's DNFT effects into `batch`. Category
    //! records are read-modify-write; a single `cats` accumulator is threaded through the whole
    //! WriteBlock (rewind + apply) and flushed once, because writes buffered in `batch` are NOT
    //! visible to m_db->Read — so per-call re-reads would corrupt any category touched by more than
    //! one block in a single reorg/replay batch (4I review F2).
    bool ProcessBlock(const CBlock &block, const CBlockIndex *pindex, CDBBatch &batch,
                      std::map<uint256, DnftCategoryRecord> &cats, bool apply);
    //! Reverse blocks from the current best back to (but excluding) `target`.
    bool Rewind(const CBlockIndex *target, CDBBatch &batch,
                std::map<uint256, DnftCategoryRecord> &cats);

protected:
    bool WriteBlock(const CBlock &block, const CBlockIndex *pindex) override;
    BaseIndex::DB &GetDB() const override;
    bool Init() override;

public:
    explicit DnftIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
    virtual ~DnftIndex() override;

    //! Look up an item by (category, commitment). Independent of chain state / active locks.
    std::optional<DnftItemRecord> GetItem(const uint256 &category,
                                          const std::vector<uint8_t> &commitment) const;
    //! Look up a collection by category.
    std::optional<DnftCategoryRecord> GetCategory(const uint256 &category) const;
};

/// The global DNFT index object. May be null (only constructed when -nftindex is set).
extern std::unique_ptr<DnftIndex> g_dnft_index;
