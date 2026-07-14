// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_DEVAULT_FT_REGISTRY_H
#define DEVAULT_DEVAULT_FT_REGISTRY_H

#include <dbwrapper.h>
#include <fs.h>
#include <primitives/blockhash.h>
#include <primitives/token.h>
#include <primitives/txid.h>
#include <serialize.h>
#include <sync.h>

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

class CBlock;

/**
 * The DeVault fungible-token DEPLOY REGISTRY (DEVAULT_FT_SPEC.md §6.5).
 *
 * A mint transaction must be validated against its category's TRUE emission params, whose only
 * onchain record is the deploy transaction. A CashTokens category id is the *spent-prevout* txid of
 * the deploy's index-0 input, so it is NOT txid-addressable to its deploy ("what spent (X,0)" would
 * need a spend index). The mint marker therefore names the DEPLOY TXID `D` — which *is*
 * txid-addressable — and this registry resolves `D -> {category X, emission params}`.
 *
 * Properties that make this safe and cheap:
 *  - **Static, write-once.** An entry is added when a valid open-mint deploy connects and removed if
 *    that block is disconnected. It is NEVER mutated. It holds no `mints_so_far` counter: the cap is
 *    enforced structurally by the stateless height schedule (spec §6.1), so the "stateless emission"
 *    property is fully preserved. This is reference data, not an emission engine.
 *  - **Always-on and consensus-critical** (like the cold-reward engine, unlike the optional
 *    -nftindex): every validating node maintains it, so no node's verdict depends on an optional
 *    index -> no consensus split.
 *  - **Rebuildable from block data alone.** Deploy validity is a pure function of (tx, height) —
 *    see dnft::ValidateFtDeploy — so a corrupt/absent registry is reconstructed by replaying blocks;
 *    no chainstate or undo data is needed.
 *  - **Crash-safe.** The in-memory map is the operating source of truth; the DB is a persistence
 *    backing flushed in ONE batch AFTER the chainstate, so the registry is never AHEAD of the
 *    chainstate and an unclean stop leaves at most a forward-replayable gap (reconciled at startup).
 *    This is the cold-reward engine's structural pattern (3D.3), which fixed that shutdown desync.
 *  - **Fixed-supply deploys are NOT registered** — they can never be minted, so nothing ever needs
 *    to look them up. The registry holds open-mint deploys only, keeping it minimal.
 *
 * Threading: all access is under cs_main (ConnectBlock / DisconnectBlock / FlushStateToDisk / ATMP).
 */

//! One registered open-mint deploy. Immutable once written.
struct FtDeployRecord {
    token::Id category;         //!< X — the deployed CashTokens category (internal byte order)
    uint64_t quantity = 0;      //!< Q — tokens created per mint (>= 1)
    uint64_t perBlockLimit = 0; //!< M — max mints of this deploy per block (>= 1)
    uint32_t startHeight = 0;   //!< s — first height at which mints are valid (> deployHeight)
    uint64_t maxMints = 0;      //!< N — total mints allowed (canonical; derived if end_height was used)
    uint64_t premine = 0;       //!< P — tokens genesis-created to the deployer (may be 0)
    uint32_t deployHeight = 0;  //!< height of the deploy block (for RPC/explorer; not used by consensus)
    uint8_t decimals = 0;       //!< display precision (0..8); consensus does not interpret it

    SERIALIZE_METHODS(FtDeployRecord, obj) {
        READWRITE(obj.category, obj.quantity, obj.perBlockLimit, obj.startHeight, obj.maxMints,
                  obj.premine, obj.deployHeight, obj.decimals);
    }

    bool operator==(const FtDeployRecord &o) const {
        return category == o.category && quantity == o.quantity && perBlockLimit == o.perBlockLimit &&
               startHeight == o.startHeight && maxMints == o.maxMints && premine == o.premine &&
               deployHeight == o.deployHeight && decimals == o.decimals;
    }
};

/**
 * Per-block state threaded through the validation of ONE block (ConnectBlock). Never persisted.
 *  - `mintCounts` gives the in-block index `k` of each mint (spec §6.3): the count of mints of the
 *    same deploy already applied earlier in block order. It is transient — no cross-block counter
 *    exists anywhere (that is the whole point of the stateless schedule).
 *  - `pendingDeploys` stages the block's deploys; they are committed to the registry only when the
 *    block actually connects (so a block that fails validation late cannot pollute the registry).
 *
 * A null FtBlockContext means "mempool path": the per-block cap cannot be known there, so ATMP does
 * the loose check (window open) and leaves the strict count to mining + ConnectBlock (spec §6.6).
 */
struct FtBlockContext {
    std::map<TxId, uint64_t> mintCounts;
    std::vector<std::pair<TxId, FtDeployRecord>> pendingDeploys;
};

//! Key prefix under which a deploy record is stored ('D' + deploy txid).
static constexpr char DB_FT_DEPLOY = 'D';
//! Key under which the registry stores the block hash its contents correspond to (crash reconciliation).
static constexpr char DB_FT_BESTBLOCK = 'b';

/** On-disk backing store for the deploy registry (a LevelDB at <datadir>/ftregistry). */
class CFtRegistryDB : public CDBWrapper {
public:
    CFtRegistryDB(const fs::path &path, size_t nCacheSize, bool fMemory = false, bool fWipe = false)
        : CDBWrapper(path, nCacheSize, fMemory, fWipe) {}

    //! Load every record into `out` and read the recorded best block.
    bool LoadAll(std::map<TxId, FtDeployRecord> &out, BlockHash &bestBlock);
    //! Atomically write the staged additions/removals plus the best block.
    bool BatchWrite(const std::map<TxId, FtDeployRecord> &map, const std::set<TxId> &dirty,
                    const std::set<TxId> &erased, const BlockHash &bestBlock);
};

class CFtRegistry {
public:
    explicit CFtRegistry(std::unique_ptr<CFtRegistryDB> db);

    //! Populate the in-memory map from the backing DB; returns the DB's recorded best block.
    bool Load(BlockHash &dbBestBlock) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // ---- read-only consensus lookup (no mutation) ----
    //! Resolve a mint marker's deploy txid. Returns nullptr when `D` is not a registered open deploy.
    const FtDeployRecord *Lookup(const TxId &deployTxid) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // ---- state maintenance (called only from Connect/DisconnectBlock) ----
    //! Commit a connected block's staged deploys. Called only when the block is committed
    //! (!fJustCheck) and not during VerifyDB.
    void ApplyBlock(const FtBlockContext &ctx, const BlockHash &hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    //! Reverse a disconnected block: erase every entry keyed by one of its txids. Safe and exact —
    //! a txid is in the registry IFF that transaction was a registered deploy, so erasing by txid
    //! removes precisely the deploys this block introduced (all other erases are no-ops). No undo
    //! data and no re-validation are needed.
    void UndoBlock(const CBlock &block, const BlockHash &prevHash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    void SetBestBlock(const BlockHash &hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { m_bestBlock = hash; }
    BlockHash GetBestBlock() const EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return m_bestBlock; }

    //! Write the staged mutations + best block in one atomic batch. Called AFTER the chainstate
    //! flush so the registry is never ahead of the chainstate.
    bool Flush(const BlockHash &chainstateBest) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Drop all in-memory state (used before a full rebuild).
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    size_t Size() const EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return m_map.size(); }

private:
    std::map<TxId, FtDeployRecord> m_map;
    std::set<TxId> m_dirty;  //!< added since the last flush
    std::set<TxId> m_erased; //!< removed since the last flush
    BlockHash m_bestBlock;
    std::unique_ptr<CFtRegistryDB> m_db;
};

//! The global registry. Always constructed on a node with a chainstate (consensus-critical).
extern std::unique_ptr<CFtRegistry> g_ftRegistry;
//! Suppress registry mutations during VerifyDB (which disconnects/reconnects on a throw-away view).
extern std::atomic<bool> g_ftRegistrySuppress;

#endif // DEVAULT_DEVAULT_FT_REGISTRY_H
