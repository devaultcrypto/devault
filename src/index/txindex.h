// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_H
#define BITCOIN_INDEX_TXINDEX_H

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <threadinterrupt.h>
#include <txdb.h>
#include <uint256.h>
#include <validationinterface.h>

class CBlockIndex;

/**
 * Base class for indices of blockchain data. This implements
 * CValidationInterface and ensures blocks are indexed sequentially according
 * to their position in the active chain.
 */
class BaseIndex : public CValidationInterface {
private:
    /// Whether the index is in sync with the main chain. The flag is flipped
    /// from false to true once, after which point this starts processing
    /// ValidationInterface notifications to stay in sync.
    std::atomic<bool> m_synced{false};

    /// The last block in the chain that the index is in sync with.
    std::atomic<const CBlockIndex *> m_best_block_index{nullptr};

    std::thread m_thread_sync;
    CThreadInterrupt m_interrupt;

    /// Sync the index with the block index starting from the current best
    /// block. Intended to be run in its own thread, m_thread_sync, and can be
    /// interrupted with m_interrupt. Once the index gets in sync, the m_synced
    /// flag is set and the BlockConnected ValidationInterface callback takes
    /// over and the sync thread exits.
    void ThreadSync();

    /// Write the current chain block locator to the DB.
    bool WriteBestBlock(const CBlockIndex *block_index);

protected:
    void
    BlockConnected(const std::shared_ptr<const CBlock> &block,
                   const CBlockIndex *pindex,
                   const std::vector<CTransactionRef> &txn_conflicted) override;

    void ChainStateFlushed(const CBlockLocator &locator) override;

    /// Initialize internal state from the database and block index.
    virtual bool Init();

    /// Write update index entries for a newly connected block.
    virtual bool WriteBlock(const CBlock &block, const CBlockIndex *pindex) {
        return true;
    }

    virtual BaseIndexDB &GetDB() const = 0;

public:
    /// Destructor interrupts sync thread if running and blocks until it exits.
    virtual ~BaseIndex();

    /// Blocks the current thread until the index is caught up to the current
    /// state of the block chain. This only blocks if the index has gotten in
    /// sync once and only needs to process blocks in the ValidationInterface
    /// queue. If the index is catching up from far behind, this method does
    /// not block and immediately returns false.
    bool BlockUntilSyncedToCurrentChain();

    void Interrupt();

    /// Start initializes the sync state and registers the instance as a
    /// ValidationInterface so that it stays in sync with blockchain updates.
    void Start();

    /// Stops the instance from staying in sync with blockchain updates.
    void Stop();
};

/**
 * TxIndex is used to look up transactions included in the blockchain by hash.
 * The index is written to a LevelDB database and records the filesystem
 * location of each transaction by transaction hash.
 */
class TxIndex final : public BaseIndex {
private:
    const std::unique_ptr<TxIndexDB> m_db;

protected:
    /// Override base class init to migrate from old database.
    bool Init() override;

    bool WriteBlock(const CBlock &block, const CBlockIndex *pindex) override;

    BaseIndexDB &GetDB() const override;

public:
    /// Constructs the index, which becomes available to be queried.
    explicit TxIndex(std::unique_ptr<TxIndexDB> db);

    /// Look up a transaction by hash.
    ///
    /// @param[in]   tx_hash  The hash of the transaction to be returned.
    /// @param[out]  block_hash  The hash of the block the transaction is found
    /// in.
    /// @param[out]  tx  The transaction itself.
    /// @return  true if transaction is found, false otherwise
    bool FindTx(const uint256 &tx_hash, uint256 &block_hash,
                CTransactionRef &tx) const;
};

/// The global transaction index, used in GetTransaction. May be null.
extern std::unique_ptr<TxIndex> g_txindex;

#endif // BITCOIN_INDEX_TXINDEX_H
