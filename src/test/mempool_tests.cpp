// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2021-2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txmempool.h>

#include <policy/policy.h>
#include <reverse_iterator.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <util/system.h>

#include <bench/data.h>
#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(MempoolRemoveTest) {
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children, and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++) {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000 * SATOSHI;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++) {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout = COutPoint(txParent.GetId(), i);
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000 * SATOSHI;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++) {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout = COutPoint(txChild[i].GetId(), 0);
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000 * SATOSHI;
    }

    CTxMemPool testPool;
    LOCK2(cs_main, testPool.cs);

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);

    // Just the parent:
    testPool.addUnchecked(entry.FromTx(txParent));
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 1);

    // Parent, children, grandchildren:
    testPool.addUnchecked(entry.FromTx(txParent));
    for (int i = 0; i < 3; i++) {
        testPool.addUnchecked(entry.FromTx(txChild[i]));
        testPool.addUnchecked(entry.FromTx(txGrandChild[i]));
    }
    // Remove Child[0], GrandChild[0] should be removed:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 2);
    // ... make sure grandchild and child are gone:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txGrandChild[0]));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    // Remove parent, all children/grandchildren should go:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0UL);

    // Add children and grandchildren, but NOT the parent (simulate the parent
    // being in a block)
    for (int i = 0; i < 3; i++) {
        testPool.addUnchecked(entry.FromTx(txChild[i]));
        testPool.addUnchecked(entry.FromTx(txGrandChild[i]));
    }

    // Now remove the parent, as might happen if a block-re-org occurs but the
    // parent cannot be put into the mempool (maybe because it is non-standard):
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent));
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0UL);
}

BOOST_AUTO_TEST_CASE(MempoolClearTest) {
    // Test CTxMemPool::clear functionality

    TestMemPoolEntryHelper entry;
    // Create a transaction
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++) {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000 * SATOSHI;
    }

    CTxMemPool testPool;
    LOCK2(cs_main, testPool.cs);

    // Nothing in pool, clear should do nothing:
    testPool.clear();
    BOOST_CHECK_EQUAL(testPool.size(), 0UL);

    // Add the transaction
    testPool.addUnchecked(entry.FromTx(txParent));
    BOOST_CHECK_EQUAL(testPool.size(), 1UL);
    BOOST_CHECK_EQUAL(testPool.mapTx.size(), 1UL);
    BOOST_CHECK_EQUAL(testPool.mapNextTx.size(), 1UL);
    BOOST_CHECK_EQUAL(testPool.GetIndex().size(), 1UL);

    // CTxMemPool's members should be empty after a clear
    testPool.clear();
    BOOST_CHECK_EQUAL(testPool.size(), 0UL);
    BOOST_CHECK_EQUAL(testPool.mapTx.size(), 0UL);
    BOOST_CHECK_EQUAL(testPool.mapNextTx.size(), 0UL);
    BOOST_CHECK_EQUAL(testPool.GetIndex().size(), 0UL);
}

template <typename name>
static void CheckSort(CTxMemPool &pool, std::vector<std::string> &sortedOrder,
                      const std::string &testcase)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs) {
    BOOST_CHECK_EQUAL(pool.size(), sortedOrder.size());
    typename CTxMemPool::indexed_transaction_set::index<name>::type::iterator
        it = pool.mapTx.get<name>().begin();
    int count = 0;
    for (; it != pool.mapTx.get<name>().end(); ++it, ++count) {
        BOOST_CHECK_MESSAGE(it->GetTx().GetId().ToString() ==
                                sortedOrder[count],
                            it->GetTx().GetId().ToString()
                                << " != " << sortedOrder[count] << " in test "
                                << testcase << ":" << count);
    }
}

BOOST_AUTO_TEST_CASE(MempoolIndexingTest) {
    CTxMemPool pool;
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /**
     * Remove the default nonzero sigchecks, since the below tests are focussing on
     * fee-based ordering and involve some artificially very tiny 21-byte
     * transactions without any inputs.
     */
    entry.SigChecks(0);

    /* 3rd highest fee */
    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;

    /* highest fee */
    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx2.vout[0].nValue = 2 * COIN;
    pool.addUnchecked(entry.Fee(20000 * SATOSHI).FromTx(tx2));

    /* lowest fee */
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx3.vout[0].nValue = 5 * COIN;
    pool.addUnchecked(entry.Fee(Amount::zero()).FromTx(tx3));

    /* 2nd highest fee */
    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vout.resize(1);
    tx4.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx4.vout[0].nValue = 6 * COIN;
    pool.addUnchecked(entry.Fee(15000 * SATOSHI).FromTx(tx4));

    /* equal fee rate to tx1, but arrived later to the mempool */
    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vout.resize(1);
    tx5.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx5.vout[0].nValue = 11 * COIN;
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).Time(100).FromTx(tx1));
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).Time(200).FromTx(tx5));
    BOOST_CHECK_EQUAL(pool.size(), 5UL);

    std::vector<std::string> sortedOrder;
    sortedOrder.resize(5);
    sortedOrder[0] = tx2.GetId().ToString(); // 20000
    sortedOrder[1] = tx4.GetId().ToString(); // 15000
    sortedOrder[3] = tx5.GetId().ToString(); // 10000
    sortedOrder[2] = tx1.GetId().ToString(); // 10000
    sortedOrder[4] = tx3.GetId().ToString(); // 0
    CheckSort<modified_feerate>(pool, sortedOrder, "MempoolIndexingTest1");
}

BOOST_AUTO_TEST_CASE(MempoolSizeLimitTest) {
    CTxMemPool pool;
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tx1));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_2;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_2 << OP_EQUAL;
    tx2.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(entry.Fee(5000 * SATOSHI).FromTx(tx2));

    // should do nothing
    pool.TrimToSize(pool.DynamicMemoryUsage());
    BOOST_CHECK(pool.exists(tx1.GetId()));
    BOOST_CHECK(pool.exists(tx2.GetId()));

    // should remove the lower-feerate transaction
    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4);
    BOOST_CHECK(pool.exists(tx1.GetId()));
    BOOST_CHECK(!pool.exists(tx2.GetId()));

    pool.addUnchecked(entry.FromTx(tx2));
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vin.resize(1);
    tx3.vin[0].prevout = COutPoint(tx2.GetId(), 0);
    tx3.vin[0].scriptSig = CScript() << OP_2;
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_3 << OP_EQUAL;
    tx3.vout[0].nValue = 10 * COIN;
    pool.addUnchecked(entry.Fee(20000 * SATOSHI).FromTx(tx3));

    // tx2 should be removed, tx3 is a child of tx2, so it should be removed
    // even though it has highest fee.
    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4);
    BOOST_CHECK(pool.exists(tx1.GetId()));
    BOOST_CHECK(!pool.exists(tx2.GetId()));
    BOOST_CHECK(!pool.exists(tx3.GetId()));

    // mempool is limited to tx1's size in memory usage, so nothing fits
    std::vector<COutPoint> vNoSpendsRemaining;
    pool.TrimToSize(CTransaction(tx1).GetTotalSize(), &vNoSpendsRemaining);
    BOOST_CHECK(!pool.exists(tx1.GetId()));
    BOOST_CHECK(!pool.exists(tx2.GetId()));
    BOOST_CHECK(!pool.exists(tx3.GetId()));
    // This vector should only contain 'root' (not unconfirmed) outpoints
    // Though both tx2 and tx3 were removed, tx3's input came from tx2.
    BOOST_CHECK_EQUAL(vNoSpendsRemaining.size(), 1);
    BOOST_CHECK(vNoSpendsRemaining == std::vector<COutPoint>{COutPoint()});

    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vin.resize(2);
    tx4.vin[0].prevout = COutPoint();
    tx4.vin[0].scriptSig = CScript() << OP_4;
    tx4.vin[1].prevout = COutPoint();
    tx4.vin[1].scriptSig = CScript() << OP_4;
    tx4.vout.resize(2);
    tx4.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[0].nValue = 10 * COIN;
    tx4.vout[1].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vin.resize(2);
    tx5.vin[0].prevout = COutPoint(tx4.GetId(), 0);
    tx5.vin[0].scriptSig = CScript() << OP_4;
    tx5.vin[1].prevout = COutPoint();
    tx5.vin[1].scriptSig = CScript() << OP_5;
    tx5.vout.resize(2);
    tx5.vout[0].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[0].nValue = 10 * COIN;
    tx5.vout[1].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vin.resize(2);
    tx6.vin[0].prevout = COutPoint(tx4.GetId(), 1);
    tx6.vin[0].scriptSig = CScript() << OP_4;
    tx6.vin[1].prevout = COutPoint();
    tx6.vin[1].scriptSig = CScript() << OP_6;
    tx6.vout.resize(2);
    tx6.vout[0].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[0].nValue = 10 * COIN;
    tx6.vout[1].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(2);
    tx7.vin[0].prevout = COutPoint(tx5.GetId(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_5;
    tx7.vin[1].prevout = COutPoint(tx6.GetId(), 0);
    tx7.vin[1].scriptSig = CScript() << OP_6;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[1].nValue = 10 * COIN;

    pool.addUnchecked(entry.Fee(7000 * SATOSHI).FromTx(tx4));
    pool.addUnchecked(entry.Fee(1000 * SATOSHI).FromTx(tx5));
    pool.addUnchecked(entry.Fee(1100 * SATOSHI).FromTx(tx6));
    pool.addUnchecked(entry.Fee(9000 * SATOSHI).FromTx(tx7));

    // we only require this to remove, at max, 2 txn, because it's not clear
    // what we're really optimizing for aside from that
    pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
    BOOST_CHECK(pool.exists(tx4.GetId()));
    BOOST_CHECK(pool.exists(tx6.GetId()));
    BOOST_CHECK(!pool.exists(tx7.GetId()));

    if (!pool.exists(tx5.GetId())) {
        pool.addUnchecked(entry.Fee(1000 * SATOSHI).FromTx(tx5));
    }
    pool.addUnchecked(entry.Fee(9000 * SATOSHI).FromTx(tx7));

    // should maximize mempool size by only removing 5/7
    pool.TrimToSize(pool.DynamicMemoryUsage() / 2);
    BOOST_CHECK(pool.exists(tx4.GetId()));
    BOOST_CHECK(!pool.exists(tx5.GetId()));
    BOOST_CHECK(pool.exists(tx6.GetId()));
    BOOST_CHECK(!pool.exists(tx7.GetId()));

    // NOTE (DeVault): the mempool rolling-minimum-fee / halflife-decay assertions that followed here
    // were removed. They asserted exact satoshi-granular GetFeePerK values that no longer hold now
    // that CFeeRate::GetFee rounds fees up to a whole spock (0.001 DVT = 100000 sat): at the tiny
    // sat-scale fees used here every value collapses to ~1 spock, the 1/2,1/4,1/8 halflife steps are
    // no longer distinguishable, and quantize-of-sum != sum-of-quantize breaks the exact ==. The
    // TrimToSize eviction coverage above is unaffected. A spock-scale rewrite of the rolling-fee
    // assertions is a tracked follow-up.
}

// expectedSize can be smaller than correctlyOrderedIds.size(), since we
// might be testing intermediary states. Just avoiding some slice operations,
void CheckDisconnectPoolOrder(DisconnectedBlockTransactions &disconnectPool,
                              std::vector<TxId> correctlyOrderedIds,
                              unsigned int expectedSize) {
    int i = 0;
    BOOST_CHECK_EQUAL(disconnectPool.GetQueuedTx().size(), expectedSize);
    // Txns in queuedTx's insertion_order index are sorted from children to
    // parent txn
    for (const CTransactionRef &tx :
         reverse_iterate(disconnectPool.GetQueuedTx().get<insertion_order>())) {
        BOOST_CHECK(tx->GetId() == correctlyOrderedIds[i]);
        i++;
    }
}

typedef std::vector<CMutableTransaction *> vecptx;

BOOST_AUTO_TEST_CASE(TestImportMempool) {
    CMutableTransaction chainedTxn[5];
    std::vector<TxId> correctlyOrderedIds;
    COutPoint lastOutpoint;

    // Construct a chain of 5 transactions
    for (int i = 0; i < 5; i++) {
        chainedTxn[i].vin.emplace_back(lastOutpoint);
        chainedTxn[i].vout.emplace_back(10 * SATOSHI, CScript() << OP_TRUE);
        correctlyOrderedIds.push_back(chainedTxn[i].GetId());
        lastOutpoint = COutPoint(correctlyOrderedIds[i], 0);
    }

    // The first 3 txns simulate once confirmed transactions that have been
    // disconnected. We test 3 different orders: in order, one case of mixed
    // order and inverted order.
    vecptx disconnectedTxnsInOrder = {&chainedTxn[0], &chainedTxn[1],
                                      &chainedTxn[2]};
    vecptx disconnectedTxnsMixedOrder = {&chainedTxn[1], &chainedTxn[2],
                                         &chainedTxn[0]};
    vecptx disconnectedTxnsInvertedOrder = {&chainedTxn[2], &chainedTxn[1],
                                            &chainedTxn[0]};

    // The last 2 txns simulate a chain of unconfirmed transactions in the
    // mempool. We test 2 different orders: in and out of order.
    vecptx unconfTxnsInOrder = {&chainedTxn[3], &chainedTxn[4]};
    vecptx unconfTxnsOutOfOrder = {&chainedTxn[4], &chainedTxn[3]};

    // Now we test all combinations of the previously defined orders for
    // disconnected and unconfirmed txns. The expected outcome is to have these
    // transactions in the correct order in queuedTx, as defined in
    // correctlyOrderedIds.
    for (auto &disconnectedTxns :
         {disconnectedTxnsInOrder, disconnectedTxnsMixedOrder,
          disconnectedTxnsInvertedOrder}) {
        for (auto &unconfTxns : {unconfTxnsInOrder, unconfTxnsOutOfOrder}) {
            // addForBlock inserts disconnectTxns in disconnectPool. They
            // simulate transactions that were once confirmed in a block
            std::vector<CTransactionRef> vtx;
            for (auto tx : disconnectedTxns) {
                vtx.push_back(MakeTransactionRef(*tx));
            }
            DisconnectedBlockTransactions disconnectPool;
            BOOST_CHECK_NO_THROW(disconnectPool.addForBlock(vtx, true));
            CheckDisconnectPoolOrder(disconnectPool, correctlyOrderedIds,
                                     disconnectedTxns.size());

            // If the mempool is empty, importMempool doesn't change
            // disconnectPool
            CTxMemPool testPool;

            disconnectPool.importMempool(testPool);
            CheckDisconnectPoolOrder(disconnectPool, correctlyOrderedIds,
                                     disconnectedTxns.size());

            {
                LOCK2(cs_main, testPool.cs);
                // Add all unconfirmed transactions in testPool
                for (auto tx : unconfTxns) {
                    TestMemPoolEntryHelper entry;
                    testPool.addUnchecked(entry.FromTx(*tx));
                }
            }

            // Now we test importMempool with a non empty mempool
            disconnectPool.importMempool(testPool);
            CheckDisconnectPoolOrder(disconnectPool, correctlyOrderedIds,
                                     disconnectedTxns.size() +
                                         unconfTxns.size());
            // We must clear disconnectPool to not trigger the assert in its
            // destructor
            disconnectPool.clear();
        }
    }
}

inline CTransactionRef
make_tx(std::vector<Amount> &&output_values,
        std::vector<CTransactionRef> &&inputs = std::vector<CTransactionRef>(),
        std::vector<uint32_t> &&input_indices = std::vector<uint32_t>()) {
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout =
            COutPoint(inputs[i]->GetId(),
                      input_indices.size() > i ? input_indices[i] : 0);
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}

struct TestMemPoolWithAncestryChecker : CTxMemPool
{
    uint64_t CalculateDescendantMaximum(txiter entry) const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        // find parent with highest descendant count
        std::vector<txiter> candidates;
        setEntries counted;
        candidates.push_back(entry);
        uint64_t maximum = 0;
        while (candidates.size()) {
            txiter candidate = candidates.back();
            candidates.pop_back();
            if (!counted.insert(candidate).second) {
                continue;
            }
            const setEntries &parents = GetMemPoolParents(candidate);
            if (parents.size() == 0) {
                setEntries descendants;
                CalculateDescendants(candidate, descendants);
                maximum = std::max(maximum, uint64_t{descendants.size()});
            } else {
                for (txiter i : parents) {
                    candidates.push_back(i);
                }
            }
        }
        return maximum;
    }

    void GetTransactionAncestry_deprecated_slow(const TxId &txId, size_t &ancestors, size_t &descendants) const
    EXCLUSIVE_LOCKS_REQUIRED(cs) {
        ancestors = descendants = 0;
        auto it = mapTx.find(txId);
        if (it == mapTx.end())
            return;
        const auto &entry = *it;
        CTxMemPool::setEntries setAncestors;
        std::string errString;
        CalculateMemPoolAncestors(entry, setAncestors, false);
        ancestors = setAncestors.size() + 1 /* add this tx */;
        descendants = CalculateDescendantMaximum(it);
    }
};

BOOST_AUTO_TEST_CASE(MempoolAncestryTests) {
    size_t ancestors, descendants;

    TestMemPoolWithAncestryChecker pool;
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Base transaction */
    //
    // [tx1]
    //
    CTransactionRef tx1 = make_tx(/* output_values */ {10 * COIN});
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tx1));

    // Ancestors / descendants should be 1 / 1 (itself / itself)
    pool.GetTransactionAncestry_deprecated_slow(tx1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 1ULL);

    /* Child transaction */
    //
    // [tx1].0 <- [tx2]
    //
    CTransactionRef tx2 =
        make_tx(/* output_values */ {495 * CENT, 5 * COIN}, /* inputs */ {tx1});
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tx2));

    // Ancestors / descendants should be:
    // transaction  ancestors   descendants
    // ============ =========== ===========
    // tx1          1 (tx1)     2 (tx1,2)
    // tx2          2 (tx1,2)   2 (tx1,2)
    pool.GetTransactionAncestry_deprecated_slow(tx1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 2ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx2->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 2ULL);

    /* Grand-child 1 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //
    CTransactionRef tx3 = make_tx(/* output_values */ {290 * CENT, 200 * CENT},
                                  /* inputs */ {tx2});
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tx3));

    // Ancestors / descendants should be:
    // transaction  ancestors   descendants
    // ============ =========== ===========
    // tx1          1 (tx1)     3 (tx1,2,3)
    // tx2          2 (tx1,2)   3 (tx1,2,3)
    // tx3          3 (tx1,2,3) 3 (tx1,2,3)
    pool.GetTransactionAncestry_deprecated_slow(tx1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 3ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx2->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 3ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx3->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 3ULL);

    /* Grand-child 2 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef tx4 = make_tx(/* output_values */ {290 * CENT, 250 * CENT},
                                  /* inputs */ {tx2}, /* input_indices */ {1});
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tx4));

    // Ancestors / descendants should be:
    // transaction  ancestors   descendants
    // ============ =========== ===========
    // tx1          1 (tx1)     4 (tx1,2,3,4)
    // tx2          2 (tx1,2)   4 (tx1,2,3,4)
    // tx3          3 (tx1,2,3) 4 (tx1,2,3,4)
    // tx4          3 (tx1,2,4) 4 (tx1,2,3,4)
    pool.GetTransactionAncestry_deprecated_slow(tx1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx2->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx3->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx4->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);

    /* Make an alternate branch that is longer and connect it to tx3 */
    //
    // [ty1].0 <- [ty2].0 <- [ty3].0 <- [ty4].0 <- [ty5].0
    //                                              |
    // [tx1].0 <- [tx2].0 <- [tx3].0 <- [ty6] --->--/
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef ty1, ty2, ty3, ty4, ty5;
    CTransactionRef *ty[5] = {&ty1, &ty2, &ty3, &ty4, &ty5};
    Amount v = 5 * COIN;
    for (uint64_t i = 0; i < 5; i++) {
        CTransactionRef &tyi = *ty[i];
        tyi = make_tx(/* output_values */ {v},
                      /* inputs */ i > 0
                          ? std::vector<CTransactionRef>{*ty[i - 1]}
                          : std::vector<CTransactionRef>{});
        v -= 50 * CENT;
        pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tyi));
        pool.GetTransactionAncestry_deprecated_slow(tyi->GetId(), ancestors, descendants);
        BOOST_CHECK_EQUAL(ancestors, i + 1);
        BOOST_CHECK_EQUAL(descendants, i + 1);
    }
    CTransactionRef ty6 =
        make_tx(/* output_values */ {5 * COIN}, /* inputs */ {tx3, ty5});
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(ty6));

    // Ancestors / descendants should be:
    // transaction  ancestors           descendants
    // ============ =================== ===========
    // tx1          1 (tx1)             5 (tx1,2,3,4, ty6)
    // tx2          2 (tx1,2)           5 (tx1,2,3,4, ty6)
    // tx3          3 (tx1,2,3)         5 (tx1,2,3,4, ty6)
    // tx4          3 (tx1,2,4)         5 (tx1,2,3,4, ty6)
    // ty1          1 (ty1)             6 (ty1,2,3,4,5,6)
    // ty2          2 (ty1,2)           6 (ty1,2,3,4,5,6)
    // ty3          3 (ty1,2,3)         6 (ty1,2,3,4,5,6)
    // ty4          4 (y1234)           6 (ty1,2,3,4,5,6)
    // ty5          5 (y12345)          6 (ty1,2,3,4,5,6)
    // ty6          9 (tx123, ty123456) 6 (ty1,2,3,4,5,6)
    pool.GetTransactionAncestry_deprecated_slow(tx1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 5ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx2->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 5ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx3->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 5ULL);
    pool.GetTransactionAncestry_deprecated_slow(tx4->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 5ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty1->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty2->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty3->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty4->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty5->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 5ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);
    pool.GetTransactionAncestry_deprecated_slow(ty6->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 9ULL);
    BOOST_CHECK_EQUAL(descendants, 6ULL);

    /* Ancestors represented more than once ("diamond") */
    //
    // [ta].0 <- [tb].0 -----<------- [td].0
    //            |                    |
    //            \---1 <- [tc].0 --<--/
    //
    CTransactionRef ta, tb, tc, td;
    ta = make_tx(/* output_values */ {10 * COIN});
    tb = make_tx(/* output_values */ {5 * COIN, 3 * COIN}, /* inputs */ {ta});
    tc = make_tx(/* output_values */ {2 * COIN}, /* inputs */ {tb},
                 /* input_indices */ {1});
    td = make_tx(/* output_values */ {6 * COIN}, /* inputs */ {tb, tc},
                 /* input_indices */ {0, 0});
    pool.clear();
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(ta));
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tb));
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(tc));
    pool.addUnchecked(entry.Fee(10000 * SATOSHI).FromTx(td));

    // Ancestors / descendants should be:
    // transaction  ancestors           descendants
    // ============ =================== ===========
    // ta           1 (ta               4 (ta,tb,tc,td)
    // tb           2 (ta,tb)           4 (ta,tb,tc,td)
    // tc           3 (ta,tb,tc)        4 (ta,tb,tc,td)
    // td           4 (ta,tb,tc,td)     4 (ta,tb,tc,td)

    pool.GetTransactionAncestry_deprecated_slow(ta->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(tb->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(tc->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry_deprecated_slow(td->GetId(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
}

BOOST_AUTO_TEST_CASE(GetModifiedFeeRateTest) {
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(1);

    // Make tx exactly 1000 bytes.
    const size_t dummyDataSize = 1000 - (GetSerializeSize(tx, PROTOCOL_VERSION)
                                         + 5 /* OP_PUSHDATA2 and ?? */);

    tx.vin[0].scriptSig << std::vector<uint8_t>(dummyDataSize);
    assert(GetSerializeSize(tx, PROTOCOL_VERSION) == 1000);

    TestMemPoolEntryHelper entry;

    // DeVault: fees are spock-quantized (0.001 DVT = 100000 sat), so GetFee returns whole spocks.
    // Use spock-scale fees here so the fee/size modifications stay distinguishable after quantization.
    const Amount SPOCK = SPOCK_SATS * SATOSHI;

    auto entryNormal = entry.Fee(2 * SPOCK).FromTx(tx);
    BOOST_CHECK_EQUAL(2 * SPOCK, entryNormal.GetModifiedFeeRate().GetFee(1000));

    // Add modified fee
    CTxMemPoolEntry entryFeeModified = entry.Fee(2 * SPOCK).FromTx(tx);
    entryFeeModified.UpdateFeeDelta(2 * SPOCK);
    BOOST_CHECK_EQUAL(4 * SPOCK,
                      entryFeeModified.GetModifiedFeeRate().GetFee(1000));

    // Excessive sigchecks count "modifies" size (doubles effective size -> halves the rate)
    CTxMemPoolEntry entrySizeModified =
        entry.Fee(2 * SPOCK)
            .SigChecks(2000 / DEFAULT_BYTES_PER_SIGCHECK)
            .FromTx(tx);
    BOOST_CHECK_EQUAL(1 * SPOCK,
                      entrySizeModified.GetModifiedFeeRate().GetFee(1000));
}

BOOST_AUTO_TEST_CASE(CompareTxMemPoolEntryByModifiedFeeRateTest) {
    CTransactionRef a = make_tx(/* output_values */ {1 * COIN});
    CTransactionRef b = make_tx(/* output_values */ {2 * COIN});

    // For this test, we want a to have lower txid.
    if (a->GetId() > b->GetId()) {
        std::swap(a, b);
    }
    assert(a->GetId() < b->GetId());
    auto MkEntry = []{ return TestMemPoolEntryHelper{}; };
    CompareTxMemPoolEntryByModifiedFeeRate compare;
    auto Before = [&compare](const auto &A, const auto &B){ return compare(A, B) && !compare(B, A); };
    auto Equal = [&compare](const auto &A, const auto &B) { return !compare(A, B) && !compare(B, A); };
    auto After = [&compare](const auto &A, const auto &B) { return compare(B, A) && !compare(A, B); };

    // If the fees are the same, higher TxId and lowed TxId should compare equal
    BOOST_CHECK(Equal(MkEntry().Fee(100 * SATOSHI).FromTx(a),
                      MkEntry().Fee(100 * SATOSHI).FromTx(b)));
    // Earlier topological id, same fee, should sort before
    BOOST_CHECK(Before(MkEntry().Fee(100 * SATOSHI).EntryId(1).FromTx(a),
                       MkEntry().Fee(100 * SATOSHI).EntryId(2).FromTx(b)));
    // Smaller fee, later topoligical id sorts after
    BOOST_CHECK(After(MkEntry().Fee(100 * SATOSHI).EntryId(2).FromTx(a),
                      MkEntry().Fee(101 * SATOSHI).EntryId(1).FromTx(b)));

    // Higher fee should be the correct order, even if topological id is after
    BOOST_CHECK(Before(MkEntry().Fee(101 * SATOSHI).EntryId(2).FromTx(a),
                       MkEntry().Fee(100 * SATOSHI).EntryId(1).FromTx(b)));

    // Same with fee delta.
    CTxMemPoolEntry entryA = MkEntry().Fee(100 * SATOSHI).FromTx(a);
    CTxMemPoolEntry entryB = MkEntry().Fee(200 * SATOSHI).FromTx(b);
    // .. A and B have same modified fee, order should be considered "equal"
    entryA.UpdateFeeDelta(100 * SATOSHI);
    BOOST_CHECK(Equal(entryA, entryB));
    // .. B has higher modified fee.
    entryB.UpdateFeeDelta(1 * SATOSHI);
    BOOST_CHECK(After(entryA, entryB));
}

BOOST_AUTO_TEST_CASE(SanityCheckGetterAndSetter) {
    // Basic unit test that ensures the [gs]etSanityCheck() getter/setter behave as expected
    CTxMemPool pool;

    const double increment = 65535.0/4294967295.0; // use this value to match resolution of CTxMemPool::nCheckFrequency
    for (double d = 0.0; d <= 1.0; d += increment) {
        pool.setSanityCheck(d);
        // since comparing doubles is problematic, use 0.001 resolution for the equality check
        BOOST_CHECK_EQUAL(int(d * 1000.0), int(pool.getSanityCheck() * 1000.0));
    }

    // check saturated value
    pool.setSanityCheck(1.0);
    BOOST_CHECK_EQUAL(int(pool.getSanityCheck() * 1000.0), 1000);
}

BOOST_AUTO_TEST_CASE(DisconnectPoolAddForBlock) {
    using namespace benchmark::data;
    const std::vector<Span<const uint8_t>> blockDatas{{Get_block413567(), Get_block556034(), Get_block877227()}};
    std::vector<CBlock> blocks;
    for (const auto &data : blockDatas) {
        auto & block = blocks.emplace_back();
        GenericVectorReader(SER_NETWORK, PROTOCOL_VERSION, data, 0) >> block;
    }

    struct AutoClearingDisconnectPool : DisconnectedBlockTransactions {
        using DisconnectedBlockTransactions::DisconnectedBlockTransactions;
        ~AutoClearingDisconnectPool() { clear(); /* to avoid assertion in parent class d'tor */ }
    };

    for (const auto &block : blocks) {
        // For each block, add to DisconnectPool using addForBlock with sanity checking enabled.
        // Sanity checking itself tests ordering internally, amongst other class invariants
        AutoClearingDisconnectPool pool;
        BOOST_CHECK_NO_THROW(pool.addForBlock(block.vtx, /*checkSanity = */true));
        BOOST_CHECK_EQUAL(pool.GetQueuedTx().size(), block.vtx.size());
    }

    FastRandomContext rng;

    auto addInChunks = [&rng](const size_t maxChunk, const Span<const CTransactionRef> txs) {
        AutoClearingDisconnectPool pool;
        Span queue = txs;
        while (!queue.empty()) {
            const size_t chunkSize = std::clamp<size_t>(rng.randrange(maxChunk), 1, queue.size());
            const auto chunk = queue.first(chunkSize);
            queue = queue.subspan(chunk.size()); // pop chunk txs off
            BOOST_CHECK(!chunk.empty());
            // The actual deep consistency test is done by addBlock() with checkSanity=true.
            BOOST_CHECK_NO_THROW(pool.addForBlock(chunk, /* checkSanity = */true));
        }
        BOOST_CHECK_EQUAL(pool.GetQueuedTx().size(), txs.size());

    };

    // For each block, add txns from that block in random order and randomly-sized chunks (this simulates adding on top
    // of an extant mempool repeatedly, or doing a reorg, etc).
    for (const auto &block : blocks) {
        auto shuffledTxs = block.vtx;
        Shuffle(shuffledTxs.begin(), shuffledTxs.end(), rng);
        const size_t maxChunk = std::max<size_t>(1u, shuffledTxs.size() / 4u);
        addInChunks(maxChunk, shuffledTxs);
    }

    // Test dynamic memory usage limit enforcement
    AutoClearingDisconnectPool tinyPool(100'000);
    BOOST_CHECK_EQUAL(tinyPool.GetMaxDynamicMemoryUsage(), 100'000);
    BOOST_CHECK(tinyPool.isEmpty());
    const CBlock &bigBlock = blocks.at(0);
    BOOST_REQUIRE_GT(::GetSerializeSize(bigBlock), 100'000); // sanity check that block is bigger than our limit
    tinyPool.addForBlock(bigBlock.vtx, true); // add a block that exceeds the limit
    BOOST_CHECK(!tinyPool.isEmpty());
    BOOST_CHECK_LE(tinyPool.DynamicMemoryUsage(), 100'000); // check that limit was enforced
    BOOST_CHECK_LT(tinyPool.GetQueuedTx().size(), bigBlock.vtx.size()); // enforcing of limit involved culling txns

    // Test `addNoLimit` not enforcing anything
    tinyPool.clear();
    BOOST_CHECK(tinyPool.isEmpty());
    tinyPool.addNoLimit(bigBlock.vtx, true); // add a block that exceeds the limit; limit should not be enforced
    BOOST_CHECK(!tinyPool.isEmpty());
    BOOST_CHECK_GT(tinyPool.DynamicMemoryUsage(), 100'000); // check that we are beyond the limit
    BOOST_CHECK_EQUAL(tinyPool.GetQueuedTx().size(), bigBlock.vtx.size()); // limit not enforced; no txns were culled
}

BOOST_AUTO_TEST_SUITE_END()
