// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>

#include <chainparams.h>
#include <script/standard.h>
#include <util/system.h>
#include <util/time.h>
#include <validation.h>

#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

BOOST_FIXTURE_TEST_CASE(txindex_initial_sync, TestChain100Setup) {
    TxIndex txindex(1 << 20, true);

    CTransactionRef tx_disk;
    BlockHash block_hash;

    // Transaction should not be found in the index before it is started.
    for (const auto &txn : coinbaseTxns) {
        BOOST_CHECK(!txindex.FindTx(txn.GetHash(), block_hash, tx_disk));
    }

    // BlockUntilSyncedToCurrentChain should return false before txindex is
    // started.
    BOOST_CHECK(!txindex.BlockUntilSyncedToCurrentChain());

    txindex.Start();

    // Allow tx index to catch up with the block index.
    constexpr int64_t timeout_ms = 10 * 1000;
    int64_t time_start = GetTimeMillis();
    while (!txindex.BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(time_start + timeout_ms > GetTimeMillis());
        MilliSleep(100);
    }

    // Check that txindex excludes genesis block transactions.
    const CBlock &genesis_block = Params().GenesisBlock();
    for (const auto &txn : genesis_block.vtx) {
        BOOST_CHECK(!txindex.FindTx(txn->GetId(), block_hash, tx_disk));
    }

    // Check that txindex has all txs that were in the chain before it started.
    for (const auto &txn : coinbaseTxns) {
        if (!txindex.FindTx(txn.GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn.GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key =
            GetScriptForDestination(coinbaseKey.GetPubKey().GetID());
        std::vector<CMutableTransaction> no_txns;
        const CBlock &block =
            CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction &txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        if (!txindex.FindTx(txn.GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn.GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txindex.Stop();

    // Rest of shutdown sequence and destructors happen in ~TestingSetup()
}

BOOST_AUTO_TEST_SUITE_END()
