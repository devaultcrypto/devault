// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <index/dnftindex.h>

#include <chainparams.h>
#include <config.h>
#include <dbwrapper.h>
#include <devault/dnft_envelope.h>
#include <node/blockstorage.h>
#include <primitives/token.h>
#include <undo.h>
#include <util/system.h>
#include <validation.h>

#include <algorithm>
#include <map>
#include <set>
#include <utility>

std::unique_ptr<DnftIndex> g_dnft_index;

namespace {
constexpr uint8_t DB_ITEM = 'I';     // (category, commitment) -> DnftItemRecord
constexpr uint8_t DB_CATEGORY = 'C';  // category -> DnftCategoryRecord
constexpr uint8_t DB_BEST = 'B';      // -> BlockHash (last applied block)

using ItemKey = std::pair<uint8_t, std::pair<uint256, std::vector<uint8_t>>>;
ItemKey MakeItemKey(const uint256 &cat, const std::vector<uint8_t> &commit) {
    return {DB_ITEM, {cat, commit}};
}
using CatKey = std::pair<uint8_t, uint256>;
CatKey MakeCatKey(const uint256 &cat) { return {DB_CATEGORY, cat}; }

// (category || commitment) identity for an inscribed item, used for the per-tx immutable-input set.
std::vector<uint8_t> ItemIdentity(const token::Id &cat, const token::NFTCommitment &commit) {
    std::vector<uint8_t> k(cat.begin(), cat.end());
    k.insert(k.end(), commit.begin(), commit.end());
    return k;
}
} // namespace

struct DnftIndex::DB : BaseIndex::DB {
    explicit DB(size_t n_cache_size, bool f_memory, bool f_wipe)
        : BaseIndex::DB(GetDataDir() / "indexes" / "dnft", n_cache_size, f_memory, f_wipe) {}
};

DnftIndex::DnftIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex("dnftindex"), m_db(std::make_unique<DnftIndex::DB>(n_cache_size, f_memory, f_wipe)) {}

DnftIndex::~DnftIndex() {}

BaseIndex::DB &DnftIndex::GetDB() const { return *m_db; }

bool DnftIndex::Init() {
    if (!BaseIndex::Init()) {
        return false;
    }
    BlockHash best;
    if (m_db->Read(uint8_t(DB_BEST), best)) {
        m_best_block_hash = best;
    }
    return true;
}

bool DnftIndex::ProcessBlock(const CBlock &block, const CBlockIndex *pindex, CDBBatch &batch,
                             const bool apply) {
    // Genesis (and any pre-DU1 block) carries no DNFT/token data; undo may be absent for genesis.
    CBlockUndo undo;
    const bool haveUndo = pindex->nHeight > 0 && UndoReadFromDisk(undo, pindex);
    if (pindex->nHeight > 0 && !haveUndo) {
        return false; // cannot classify mint-vs-move without undo (pruned?)
    }

    // Category records are read-modify-write and may be touched many times per block, so cache them
    // in memory and flush once at the end. Item records are add/erase only -> written directly.
    std::map<uint256, DnftCategoryRecord> cats;
    const auto loadCat = [&](const uint256 &c) -> DnftCategoryRecord & {
        auto it = cats.find(c);
        if (it != cats.end()) {
            return it->second;
        }
        DnftCategoryRecord rec;
        m_db->Read(MakeCatKey(c), rec); // leaves rec default (genesisTxid null) if absent
        return cats.emplace(c, std::move(rec)).first->second;
    };
    const auto touchGenesis = [&](DnftCategoryRecord &rec, const TxId &txid) {
        if (rec.genesisTxid.IsNull()) {
            rec.genesisTxid = txid;
            rec.genesisHeight = pindex->nHeight;
        }
    };

    for (size_t i = 0; i < block.vtx.size(); ++i) {
        const CTransactionRef &tx = block.vtx[i];
        if (tx->IsCoinBase()) {
            continue;
        }

        // Immutable NFT inputs (from undo) — the set of items already existing on the inputs. An
        // inscribed output whose identity is in this set is a MOVE, not a mint (matches CheckDnftRules).
        std::set<std::vector<uint8_t>> immInputs;
        const CTxUndo &txundo = undo.vtxundo.at(i - 1);
        for (const Coin &coin : txundo.vprevout) {
            const token::OutputDataPtr &ptd = coin.GetTxOut().tokenDataPtr;
            if (ptd && ptd->IsImmutableNFT()) {
                immInputs.insert(ItemIdentity(ptd->GetId(), ptd->GetCommitment()));
            }
        }

        // One pass over outputs: collect envelopes + new-mint indices (in order), and record
        // minting-token creates.
        std::vector<size_t> envVouts, mintVouts;
        for (size_t n = 0; n < tx->vout.size(); ++n) {
            const CTxOut &out = tx->vout[n];
            if (dnft::IsDnftEnvelope(out.scriptPubKey) && !out.tokenDataPtr) {
                envVouts.push_back(n);
                continue;
            }
            const token::OutputDataPtr &ptd = out.tokenDataPtr;
            if (!ptd) {
                continue;
            }
            if (ptd->IsMintingNFT()) {
                const uint256 cat = ptd->GetId();
                DnftCategoryRecord &rec = loadCat(cat);
                touchGenesis(rec, TxId(tx->GetId()));
                const COutPoint op(tx->GetId(), n);
                if (apply) {
                    rec.mintingOutpoints.push_back(op);
                } else {
                    auto &v = rec.mintingOutpoints;
                    v.erase(std::remove(v.begin(), v.end(), op), v.end());
                }
            }
            const token::NFTCommitment &commit = ptd->GetCommitment();
            const bool inscribed =
                ptd->IsImmutableNFT() && !commit.empty() && commit[0] == dnft::BINDING_VERSION;
            if (inscribed && immInputs.count(ItemIdentity(ptd->GetId(), commit)) == 0) {
                mintVouts.push_back(n); // a MINT (new inscribed item, not a move)
            }
        }

        // Pair the i-th mint with the i-th envelope (block validity guarantees equal counts).
        for (size_t k = 0; k < mintVouts.size(); ++k) {
            const size_t nftIdx = mintVouts[k];
            const CTxOut &nftOut = tx->vout[nftIdx];
            const uint256 cat = nftOut.tokenDataPtr->GetId();
            const std::vector<uint8_t> commit(nftOut.tokenDataPtr->GetCommitment().begin(),
                                              nftOut.tokenDataPtr->GetCommitment().end());
            DnftCategoryRecord &rec = loadCat(cat);
            if (apply) {
                touchGenesis(rec, TxId(tx->GetId()));
                ++rec.mintedCount;
                DnftItemRecord item;
                item.mintTxid = TxId(tx->GetId());
                item.mintVout = uint32_t(nftIdx);
                item.height = pindex->nHeight;
                if (k < envVouts.size()) {
                    item.envelopeVout = uint32_t(envVouts[k]);
                    const dnft::ParsedEnvelope pe =
                        dnft::ParseDnftEnvelope(tx->vout[envVouts[k]].scriptPubKey);
                    if (pe.valid) {
                        if (pe.content_type) {
                            item.contentType = *pe.content_type;
                        }
                        item.contentLength = pe.has_body ? pe.body.size() : 0;
                        item.parents = pe.parents;
                    }
                }
                batch.Write(MakeItemKey(cat, commit), item);
            } else {
                if (rec.mintedCount > 0) {
                    --rec.mintedCount;
                }
                batch.Erase(MakeItemKey(cat, commit));
            }
        }
    }

    // Flush category records (erase when the collection has nothing left after a reversal).
    for (const auto &[cat, rec] : cats) {
        if (rec.mintedCount == 0 && rec.mintingOutpoints.empty()) {
            batch.Erase(MakeCatKey(cat));
        } else {
            batch.Write(MakeCatKey(cat), rec);
        }
    }
    return true;
}

bool DnftIndex::Rewind(const CBlockIndex *target, CDBBatch &batch) {
    const CBlockIndex *cur;
    {
        LOCK(cs_main);
        cur = LookupBlockIndex(m_best_block_hash);
    }
    const auto &params = ::Params().GetConsensus();
    const int targetHeight = target ? target->nHeight : -1;
    while (cur && cur != target) {
        if (cur->nHeight <= targetHeight) {
            return false; // overshot the fork point (inconsistent state)
        }
        CBlock block;
        if (!ReadBlockFromDisk(block, cur, params)) {
            return false;
        }
        if (!ProcessBlock(block, cur, batch, /*apply=*/false)) {
            return false;
        }
        cur = cur->pprev;
    }
    return cur == target;
}

bool DnftIndex::WriteBlock(const CBlock &block, const CBlockIndex *pindex) {
    CDBBatch batch(*m_db);

    // Reorg detection: if this block does not extend our current best, rewind to its parent first.
    if (pindex->pprev != nullptr && !m_best_block_hash.IsNull() &&
        m_best_block_hash != pindex->pprev->GetBlockHash()) {
        if (!Rewind(pindex->pprev, batch)) {
            return false;
        }
    }

    if (!ProcessBlock(block, pindex, batch, /*apply=*/true)) {
        return false;
    }

    const BlockHash hash = pindex->GetBlockHash();
    batch.Write(uint8_t(DB_BEST), hash);
    if (!m_db->WriteBatch(batch)) {
        return false;
    }
    m_best_block_hash = hash;
    return true;
}

std::optional<DnftItemRecord> DnftIndex::GetItem(const uint256 &category,
                                                 const std::vector<uint8_t> &commitment) const {
    DnftItemRecord rec;
    if (m_db->Read(MakeItemKey(category, commitment), rec)) {
        return rec;
    }
    return std::nullopt;
}

std::optional<DnftCategoryRecord> DnftIndex::GetCategory(const uint256 &category) const {
    DnftCategoryRecord rec;
    if (m_db->Read(MakeCatKey(category), rec)) {
        return rec;
    }
    return std::nullopt;
}
