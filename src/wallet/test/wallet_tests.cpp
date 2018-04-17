// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <wallet/wallet.h>

#include <chainparams.h>
#include <config.h>

#include <consensus/validation.h>
#include <rpc/server.h>
#include <catch_tests/test_bitcoin.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcdump.h>
#include <wallet/test/wallet_test_fixture.h>

#include <univalue.h>

#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#define CATCH_CONFIG_MAIN  
#include <catch2/catch.hpp>


// how many times to run all the tests to have a chance to catch errors that
// only show up with particular random shuffles
#define RUN_TESTS 100

// some tests fail 1% of the time due to bad luck. We repeat those tests this
// many times and only complain if all iterations of the test fail.
#define RANDOM_REPEATS 5

std::vector<std::unique_ptr<CWalletTx>> wtxn;

typedef std::set<CInputCoin> CoinSet;

// Critical section is used to prevent concurrent execution of
// tests in this fixture
static CCriticalSection walletCriticalSection;

static std::vector<COutput> vCoins;

static void add_coin(const CWallet &wallet, const Amount nValue,
                     int nAge = 6 * 24, bool fIsFromMe = false,
                     int nInput = 0) {
    static int nextLockTime = 0;
    CMutableTransaction tx;
    // So all transactions get different hashes.
    tx.nLockTime = nextLockTime++;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (fIsFromMe) {
        // IsFromMe() returns (GetDebit() > 0), and GetDebit() is 0 if
        // vin.empty(), so stop vin being empty, and cache a non-zero Debit to
        // fake out IsFromMe()
        tx.vin.resize(1);
    }
    std::unique_ptr<CWalletTx> wtx(
        new CWalletTx(&wallet, MakeTransactionRef(std::move(tx))));
    if (fIsFromMe) {
        wtx->fDebitCached = true;
        wtx->nDebitCached = Amount::min_amount();
    }
    COutput output(wtx.get(), nInput, nAge, true /* spendable */,
                   true /* solvable */, true /* safe */);
    vCoins.push_back(output);
    wtxn.emplace_back(std::move(wtx));
}

static void empty_wallet(void) {
    vCoins.clear();
    wtxn.clear();
}

static bool equal_sets(CoinSet a, CoinSet b) {
    std::pair<CoinSet::iterator, CoinSet::iterator> ret =
        mismatch(a.begin(), a.end(), b.begin());
    return ret.first == a.end() && ret.second == b.end();
}


TEST_CASE("coin_selection_tests") {
    CoinSet setCoinsRet, setCoinsRet2;
    Amount nValueRet;

    WalletTestingSetup setup;

    const CWallet wallet(Params());
    LOCK(walletCriticalSection);

    // test multiple times to allow for differences in the shuffle order
    for (int i = 0; i < RUN_TESTS; i++) {
        empty_wallet();

        // with an empty wallet we can't even pay one cent
        REQUIRE(!wallet.SelectCoinsMinConf(1 * COIN, 1, 6, 0, vCoins,
                                           setCoinsRet, nValueRet));
        // add a new 1 cent coin
        add_coin(wallet, 1 * COIN, 4);

        // with a new 1 cent coin, we still can't find a mature 1 cent
        REQUIRE(!wallet.SelectCoinsMinConf(1 * COIN, 1, 6, 0, vCoins,
                                           setCoinsRet, nValueRet));

        // but we can find a new 1 cent
        REQUIRE(wallet.SelectCoinsMinConf(1 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 1 * COIN);
        // add a mature 2 cent coin
        add_coin(wallet, 2 * COIN);

        // we can't make 3 cents of mature coins
        REQUIRE(!wallet.SelectCoinsMinConf(3 * COIN, 1, 6, 0, vCoins,
                                           setCoinsRet, nValueRet));

        // we can make 3 cents of new coins
        REQUIRE(wallet.SelectCoinsMinConf(3 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 3 * COIN);

        // add a mature 5 cent coin,
        add_coin(wallet, 5 * COIN);
        // a new 10 cent coin sent from one of our own addresses
        add_coin(wallet, 10 * COIN, 3, true);
        // and a mature 20 cent coin
        add_coin(wallet, 20 * COIN);

        // now we have new: 1+10=11 (of which 10 was self-sent), and mature:
        // 2+5+20=27.  total = 38

        // we can't make 38 cents only if we disallow new coins:
        REQUIRE(!wallet.SelectCoinsMinConf(38 * COIN, 1, 6, 0, vCoins,
                                           setCoinsRet, nValueRet));
        // we can't even make 37 cents if we don't allow new coins even if
        // they're from us
        REQUIRE(!wallet.SelectCoinsMinConf(38 * COIN, 6, 6, 0, vCoins,
                                           setCoinsRet, nValueRet));
        // but we can make 37 cents if we accept new coins from ourself
        REQUIRE(wallet.SelectCoinsMinConf(37 * COIN, 1, 6, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 37 * COIN);
        // and we can make 38 cents if we accept all new coins
        REQUIRE(wallet.SelectCoinsMinConf(38 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 38 * COIN);

        // try making 34 cents from 1,2,5,10,20 - we can't do it exactly
        REQUIRE(wallet.SelectCoinsMinConf(34 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // but 35 cents is closest because of MIN_CHANGE/2==1 DVT
        REQUIRE(nValueRet == 35 * COIN);
        // the best should be 20+10+5.
        REQUIRE(setCoinsRet.size() == 3U);

        // when we try making 7 cents, the smaller coins (1,2,5) are enough.  We
        // should see just 2+5
        REQUIRE(wallet.SelectCoinsMinConf(7 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 7 * COIN);
        REQUIRE(setCoinsRet.size() == 2U);

        // when we try making 8 cents, the smaller coins (1,2,5) are exactly
        // enough.
        REQUIRE(wallet.SelectCoinsMinConf(8 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 8 * COIN);
        REQUIRE(setCoinsRet.size() == 3U);

        // when we try making 9 cents, no subset of smaller coins is enough, and
        // we get 15 due to min change of 6
        REQUIRE(wallet.SelectCoinsMinConf(9 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 10 * COIN);
        REQUIRE(setCoinsRet.size() == 1U);

        // now clear out the wallet and start again to test choosing between
        // subsets of smaller coins and the next biggest coin
        empty_wallet();

        add_coin(wallet, 6 * COIN);
        add_coin(wallet, 7 * COIN);
        add_coin(wallet, 8 * COIN);
        add_coin(wallet, 20 * COIN);
        // now we have 6+7+8+20+30 = 71 cents total
        add_coin(wallet, 30 * COIN);

        // check that we have 71 and not 72
        REQUIRE(wallet.SelectCoinsMinConf(71 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(!wallet.SelectCoinsMinConf(72 * COIN, 1, 1, 0, vCoins,
                                           setCoinsRet, nValueRet));

        // now try making 16. Due to min change, etc, we must use 20+6
        REQUIRE(wallet.SelectCoinsMinConf(16 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get 20 in one coin
        REQUIRE(nValueRet == 20 * COIN);
        REQUIRE(setCoinsRet.size() == 1U);

        // now we have 5+6+7+8+20+30 = 75 cents total
        add_coin(wallet, 5 * COIN);

        // now if we try making 16 again,
        REQUIRE(wallet.SelectCoinsMinConf(16 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get 18 in 3 coins
        REQUIRE(nValueRet == 18 * COIN);
        REQUIRE(setCoinsRet.size() == 3U);

        // now we have 5+6+7+8+18+20+30
        add_coin(wallet, 18 * COIN);

        // and now if we try making 16 cents again, the smaller coins can make
        // 5+6+7 = 18 cents, the same as the next biggest coin, 18
        REQUIRE(wallet.SelectCoinsMinConf(16 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));

        REQUIRE(nValueRet == 18 * COIN);
        // because in the event of a tie, the biggest coin wins
        REQUIRE(setCoinsRet.size() == 1U);

        // now try making 11 cents.  we should get 5+6
        REQUIRE(wallet.SelectCoinsMinConf(11 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 11 * COIN);
        REQUIRE(setCoinsRet.size() == 2U);

        // check that the smallest bigger coin is used
        add_coin(wallet, 100 * COIN);
        add_coin(wallet, 200 * COIN);
        add_coin(wallet, 300 * COIN);
        // now we have 5+6+7+8+18+20+30+100+200+300+400 = 1094 cents
        add_coin(wallet, 400 * COIN);
        REQUIRE(wallet.SelectCoinsMinConf(95 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get 105 DVT in 2 coins
        REQUIRE(nValueRet == 100 * COIN);
        REQUIRE(setCoinsRet.size() == 1U);

        REQUIRE(wallet.SelectCoinsMinConf(195 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get 200 BCH in 1 coin
        REQUIRE(nValueRet == 200 * COIN);
        REQUIRE(setCoinsRet.size() == 1U);

        // empty the wallet and start again, now with fractions of a cent, to
        // test small change avoidance

        empty_wallet();
        add_coin(wallet, 1 * MIN_CHANGE / 10);
        add_coin(wallet, 2 * MIN_CHANGE / 10);
        add_coin(wallet, 3 * MIN_CHANGE / 10);
        add_coin(wallet, 4 * MIN_CHANGE / 10);
        add_coin(wallet, 5 * MIN_CHANGE / 10);

        // try making 1 * MIN_CHANGE from the 1.5 * MIN_CHANGE we'll get change
        // smaller than MIN_CHANGE whatever happens, so can expect MIN_CHANGE
        // exactly
        REQUIRE(wallet.SelectCoinsMinConf(MIN_CHANGE, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        REQUIRE(nValueRet == MIN_CHANGE);

        // but if we add a bigger coin, small change is avoided
        add_coin(wallet, 1111 * MIN_CHANGE);

        // try making 1 from 0.1 + 0.2 + 0.3 + 0.4 + 0.5 + 1111 = 1112.5
        REQUIRE(wallet.SelectCoinsMinConf(1 * MIN_CHANGE, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get the exact amount
        REQUIRE(nValueRet == 1 * MIN_CHANGE);

        // if we add more small coins:
        add_coin(wallet, 6 * MIN_CHANGE / 10);
        add_coin(wallet, 7 * MIN_CHANGE / 10);

        // and try again to make 1.0 * MIN_CHANGE
        REQUIRE(wallet.SelectCoinsMinConf(1 * MIN_CHANGE, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get the exact amount
        REQUIRE(nValueRet == 1 * MIN_CHANGE);

        // run the 'mtgox' test (see
        // http://blockexplorer.com/tx/29a3efd3ef04f9153d47a990bd7b048a4b2d213daaa5fb8ed670fb85f13bdbcf)
        // they tried to consolidate 10 50k coins into one 500k coin, and ended
        // up with 50k in change
        empty_wallet();
        for (int j = 0; j < 20; j++) {
            add_coin(wallet, 50000 * COIN);
        }

        REQUIRE(wallet.SelectCoinsMinConf(500000 * COIN, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get the exact amount
        REQUIRE(nValueRet == 500000 * COIN);
        // in ten coins
        REQUIRE(setCoinsRet.size() == 10U);

        // if there's not enough in the smaller coins to make at least 1 *
        // MIN_CHANGE change (0.5+0.6+0.7 < 1.0+1.0), we need to try finding an
        // exact subset anyway

        // sometimes it will fail, and so we use the next biggest coin:
        empty_wallet();
        add_coin(wallet, 5 * MIN_CHANGE / 10);
        add_coin(wallet, 6 * MIN_CHANGE / 10);
        add_coin(wallet, 7 * MIN_CHANGE / 10);
        add_coin(wallet, 1111 * MIN_CHANGE);
        REQUIRE(wallet.SelectCoinsMinConf(1 * MIN_CHANGE, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we get the bigger coin
        REQUIRE(nValueRet == 1111 * MIN_CHANGE);
        REQUIRE(setCoinsRet.size() == 1U);

        // but sometimes it's possible, and we use an exact subset (0.4 + 0.6 =
        // 1.0)
        empty_wallet();
        add_coin(wallet, 4 * MIN_CHANGE / 10);
        add_coin(wallet, 6 * MIN_CHANGE / 10);
        add_coin(wallet, 8 * MIN_CHANGE / 10);
        add_coin(wallet, 1111 * MIN_CHANGE);
        REQUIRE(wallet.SelectCoinsMinConf(MIN_CHANGE, 1, 1, 0, vCoins,
                                          setCoinsRet, nValueRet));
        // we should get the exact amount
        REQUIRE(nValueRet == MIN_CHANGE);
        // in two coins 0.4+0.6
        REQUIRE(setCoinsRet.size() == 2U);

        // test avoiding small change
#ifdef DEBUG_THIS
        empty_wallet();
        add_coin(wallet, 5 * MIN_CHANGE / 10);
        add_coin(wallet, 1 * MIN_CHANGE);
        add_coin(wallet, 100 * MIN_CHANGE);

        // trying to make 100.01 from these three coins
        REQUIRE(wallet.SelectCoinsMinConf(10001 * MIN_CHANGE / 10, 1, 1, 0,
                                          vCoins, setCoinsRet, nValueRet));
        // we should get all coins
        REQUIRE(nValueRet == 10105 * MIN_CHANGE / 10);
        REQUIRE(setCoinsRet.size() == 3U);

        // but if we try to make 99.9, we should take the bigger of the two
        // small coins to avoid small change
        REQUIRE(wallet.SelectCoinsMinConf(9990 * MIN_CHANGE / 100, 1, 1, 0,
                                          vCoins, setCoinsRet, nValueRet));
        REQUIRE(nValueRet == 101 * MIN_CHANGE);
        REQUIRE(setCoinsRet.size() == 2U);

        // test with many inputs
        for (Amount amt = Amount(1500); amt < COIN; amt = 10 * amt) {
            empty_wallet();
            // Create 676 inputs (=  (old MAX_STANDARD_TX_SIZE == 100000)  / 148
            // bytes per input)
            for (uint16_t j = 0; j < 676; j++) {
                add_coin(wallet, amt);
            }
            REQUIRE(wallet.SelectCoinsMinConf(Amount(2000), 1, 1, 0, vCoins,
                                              setCoinsRet, nValueRet));
            if (amt - Amount(2000) < MIN_CHANGE) {
                // needs more than one input:
                uint16_t returnSize = std::ceil(
                    double(2000 + (MIN_CHANGE.toInt())) / (amt.toInt()));
                Amount returnValue = returnSize * amt;
                REQUIRE(nValueRet == returnValue);
                REQUIRE(setCoinsRet.size() == returnSize);
            } else {
                // one input is sufficient:
                REQUIRE(nValueRet == amt);
                REQUIRE(setCoinsRet.size() == 1U);
            }
        }
#endif

        // test randomness
        {
            empty_wallet();
            for (int i2 = 0; i2 < 100; i2++) {
                add_coin(wallet, COIN);
            }

            // picking 50 from 100 coins doesn't depend on the shuffle, but does
            // depend on randomness in the stochastic approximation code
            REQUIRE(wallet.SelectCoinsMinConf(50 * COIN, 1, 6, 0, vCoins,
                                              setCoinsRet, nValueRet));
            REQUIRE(wallet.SelectCoinsMinConf(50 * COIN, 1, 6, 0, vCoins,
                                              setCoinsRet2, nValueRet));
            REQUIRE(!equal_sets(setCoinsRet, setCoinsRet2));

            int fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++) {
                // selecting 1 from 100 identical coins depends on the shuffle;
                // this test will fail 1% of the time run the test
                // RANDOM_REPEATS times and only complain if all of them fail
                REQUIRE(wallet.SelectCoinsMinConf(COIN, 1, 6, 0, vCoins,
                                                  setCoinsRet, nValueRet));
                REQUIRE(wallet.SelectCoinsMinConf(COIN, 1, 6, 0, vCoins,
                                                  setCoinsRet2, nValueRet));
                if (equal_sets(setCoinsRet, setCoinsRet2)) {
                    fails++;
                }
            }
            REQUIRE(fails != RANDOM_REPEATS);

            // add 75 cents in small change.  not enough to make 90 cents, then
            // try making 90 cents.  there are multiple competing "smallest
            // bigger" coins, one of which should be picked at random
            add_coin(wallet, 5 * COIN);
            add_coin(wallet, 10 * COIN);
            add_coin(wallet, 15 * COIN);
            add_coin(wallet, 20 * COIN);
            add_coin(wallet, 25 * COIN);

            fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++) {
                // selecting 1 from 100 identical coins depends on the shuffle;
                // this test will fail 1% of the time run the test
                // RANDOM_REPEATS times and only complain if all of them fail
                REQUIRE(wallet.SelectCoinsMinConf(90 * COIN, 1, 6, 0, vCoins,
                                                  setCoinsRet, nValueRet));
                REQUIRE(wallet.SelectCoinsMinConf(90 * COIN, 1, 6, 0, vCoins,
                                                  setCoinsRet2, nValueRet));
                if (equal_sets(setCoinsRet, setCoinsRet2)) {
                    fails++;
                }
            }
            REQUIRE(fails != RANDOM_REPEATS);
        }
    }
    empty_wallet();
}

TEST_CASE("ApproximateBestSubset") {
    CoinSet setCoinsRet;
    Amount nValueRet;

    WalletTestingSetup setup;

    const CWallet wallet(Params());
    LOCK(walletCriticalSection);

    empty_wallet();

    // Test vValue sort order
    for (int i = 0; i < 1000; i++) {
        add_coin(wallet, 1000 * COIN);
    }
    add_coin(wallet, 3 * COIN);

    REQUIRE(wallet.SelectCoinsMinConf(1003 * COIN, 1, 6, 0, vCoins, setCoinsRet,
                                      nValueRet));
    REQUIRE(nValueRet == 1003 * COIN);
    REQUIRE(setCoinsRet.size() == 2U);

    empty_wallet();
}

static void AddKey(CWallet &wallet, const CKey &key) {
    LOCK(wallet.cs_wallet);
    wallet.AddKeyPubKey(key, key.GetPubKey());
}

#ifdef DEBUG_THIS
TEST_CASE("rescan, TestChain100Setup") {
    WalletTestingSetup setup;

    // Cap last block file size, and mine new block in a new block file.
    CBlockIndex *const nullBlock = nullptr;
    CBlockIndex *oldTip = chainActive.Tip();
    GetBlockFileInfo(oldTip->GetBlockPos().nFile)->nSize = MAX_BLOCKFILE_SIZE;
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    CBlockIndex *newTip = chainActive.Tip();

    LOCK(cs_main);

    // Verify ScanForWalletTransactions picks up transactions in both the old
    // and new block files.
    {
        CWallet wallet(Params());
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        REQUIRE(nullBlock ==
                wallet.ScanForWalletTransactions(oldTip, nullptr, reserver));
        REQUIRE(wallet.GetImmatureBalance() == 1000 * COIN);
    }

    // Prune the older block file.
    PruneOneBlockFile(oldTip->GetBlockPos().nFile);
    UnlinkPrunedFiles({oldTip->GetBlockPos().nFile});

    // Verify ScanForWalletTransactions only picks transactions in the new block
    // file.
    {
        CWallet wallet(Params());
        AddKey(wallet, coinbaseKey);
        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        REQUIRE(oldTip ==
                wallet.ScanForWalletTransactions(oldTip, nullptr, reserver));
        REQUIRE(wallet.GetImmatureBalance() == 500 * COIN);
    }

    // Verify importmulti RPC returns failure for a key whose creation time is
    // before the missing block, and success for a key whose creation time is
    // after.
    {
        CWallet wallet(Params(), "dummy", CWalletDBWrapper::CreateDummy());
        AddWallet(&wallet);
        UniValue keys;
        keys.setArray();
        UniValue key;
        key.setObject();
        key.pushKV("scriptPubKey",
                   HexStr(GetScriptForRawPubKey(coinbaseKey.GetPubKey())));
        key.pushKV("timestamp", 0);
        key.pushKV("internal", UniValue(true));
        keys.push_back(key);
        key.clear();
        key.setObject();
        CKey futureKey;
        futureKey.MakeNewKey();
        key.pushKV("scriptPubKey",
                   HexStr(GetScriptForRawPubKey(futureKey.GetPubKey())));
        key.pushKV("timestamp",
                   newTip->GetBlockTimeMax() + TIMESTAMP_WINDOW + 1);
        key.pushKV("internal", UniValue(true));
        keys.push_back(key);
        JSONRPCRequest request;
        request.params.setArray();
        request.params.push_back(keys);

        UniValue response = importmulti(GetConfig(), request);
        REQUIRE(
            response.write() ==
            strprintf("[{\"success\":false,\"error\":{\"code\":-1,\"message\":"
                      "\"Rescan failed for key with creation timestamp %d. "
                      "There was an error reading a block from time %d, which "
                      "is after or within %d seconds of key creation, and "
                      "could contain transactions pertaining to the key. As a "
                      "result, transactions and coins using this key may not "
                      "appear in the wallet. This error could be caused by "
                      "pruning or data corruption (see bitcoind log for "
                      "details) and could be dealt with by downloading and "
                      "rescanning the relevant blocks (see -reindex and "
                      "-rescan options).\"}},{\"success\":true}]",
                      0, oldTip->GetBlockTimeMax(), TIMESTAMP_WINDOW));
        RemoveWallet(&wallet);
    }
}

// Check that GetImmatureCredit() returns a newly calculated value instead of
// the cached value after a MarkDirty() call.
//
// This is a regression test written to verify a bugfix for the immature credit
// function. Similar tests probably should be written for the other credit and
// debit functions.
TEST_CASE("coin_mark_dirty_immature_credit") {
    TestChain100Setup setup;
    CWallet wallet(Params());
    CKeyingMaterial km;
    wallet.CreateMasteyKey("bypass", km);
    wallet.SetMasterKey(km);
    CWalletTx wtx(&wallet, MakeTransactionRef(setup.coinbaseTxns.back()));
    LOCK2(cs_main, wallet.cs_wallet);
    wtx.hashBlock = chainActive.Tip()->GetBlockHash();
    wtx.nIndex = 0;

    // Call GetImmatureCredit() once before adding the key to the wallet to
    // cache the current immature credit amount, which is 0.
    REQUIRE(wtx.GetImmatureCredit() == Amount::zero());

    // Invalidate the cached value, add the key, and make sure a new immature
    // credit amount is calculated.
    wtx.MarkDirty();
    wallet.AddKeyPubKey(setup.coinbaseKey, setup.coinbaseKey.GetPubKey());
    REQUIRE(wtx.GetImmatureCredit() == 500 * COIN);
}

static int64_t AddTx(CWallet &wallet, uint32_t lockTime, int64_t mockTime,
                     int64_t blockTime) {
    CMutableTransaction tx;
    tx.nLockTime = lockTime;
    SetMockTime(mockTime);
    CBlockIndex *block = nullptr;
    if (blockTime > 0) {
        LOCK(cs_main);
        auto inserted = mapBlockIndex.emplace(GetRandHash(), new CBlockIndex);
        assert(inserted.second);
        const uint256 &hash = inserted.first->first;
        block = inserted.first->second;
        block->nTime = blockTime;
        block->phashBlock = &hash;
    }

    CWalletTx wtx(&wallet, MakeTransactionRef(tx));
    if (block) {
        wtx.SetMerkleBranch(block, 0);
    }
    wallet.AddToWallet(wtx);
    LOCK(wallet.cs_wallet);
    return wallet.mapWallet.at(wtx.GetId()).nTimeSmart;
}

// Simple test to verify assignment of CWalletTx::nSmartTime value. Could be
// expanded to cover more corner cases of smart time logic.
TEST_CASE("ComputeTimeSmart") {
    WalletTestingSetup setup;

    CWallet wallet(Params());

    // New transaction should use clock time if lower than block time.
    REQUIRE(AddTx(wallet, 1, 100, 120) == 100);

    // Test that updating existing transaction does not change smart time.
    REQUIRE(AddTx(wallet, 1, 200, 220) == 100);

    // New transaction should use clock time if there's no block time.
    REQUIRE(AddTx(wallet, 2, 300, 0) == 300);

    // New transaction should use block time if lower than clock time.
    REQUIRE(AddTx(wallet, 3, 420, 400) == 400);

    // New transaction should use latest entry time if higher than
    // min(block time, clock time).
    REQUIRE(AddTx(wallet, 4, 500, 390) == 400);

    // If there are future entries, new transaction should use time of the
    // newest entry that is no more than 300 seconds ahead of the clock time.
    REQUIRE(AddTx(wallet, 5, 50, 600) == 300);

    // Reset mock time for other tests.
    SetMockTime(0);
}

#ifdef DEBUG_THIS
// dest not valid is causing issue with this test

TEST_CASE("LoadReceiveRequests") {
    WalletTestingSetup setup;

    CTxDestination dest = CKeyID();
    LOCK(pwalletMain->cs_wallet);
    pwalletMain->AddDestData(dest, "misc", "val_misc");
    pwalletMain->AddDestData(dest, "rr0", "val_rr0");
    pwalletMain->AddDestData(dest, "rr1", "val_rr1");

    auto values = pwalletMain->GetDestValues("rr");
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == "val_rr0");
    REQUIRE(values[1] == "val_rr1");
}
#endif

class ListCoinsTestingSetup : public TestChain100Setup {
public:
    ListCoinsTestingSetup() {
        CreateAndProcessBlock({},
                              GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        ::bitdb.MakeMock();
        wallet.reset(new CWallet(
            Params(), std::unique_ptr<WalletDatabase>(
                          new WalletDatabase(&bitdb, "wallet_test.dat"))));
        bool firstRun;
        wallet->LoadWallet(firstRun);
        CKeyingMaterial km;
        wallet->CreateMasteyKey("bypass", km);
        wallet->SetMasterKey(km);
        AddKey(*wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet.get());
        reserver.reserve();
        wallet->ScanForWalletTransactions(chainActive.Genesis(), nullptr,
                                          reserver);
    }

    ~ListCoinsTestingSetup() {
        wallet.reset();
        ::bitdb.Flush(true);
        ::bitdb.Reset();
    }

    CWalletTx &AddTx(CRecipient recipient) {
        CTransactionRef tx;
        CReserveKey reservekey(wallet.get());
        Amount fee;
        int changePos = -1;
        std::string error;
        CCoinControl dummy;
        REQUIRE(wallet->CreateTransaction({recipient}, tx, reservekey, fee,
                                          changePos, error, dummy));
        CValidationState state;
        REQUIRE(wallet->CommitTransaction(tx, {}, {}, {}, reservekey, nullptr,
                                          state));
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            blocktx =
                CMutableTransaction(*wallet->mapWallet.at(tx->GetId()).tx);
        }
        CreateAndProcessBlock({CMutableTransaction(blocktx)},
                              GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK(wallet->cs_wallet);
        auto it = wallet->mapWallet.find(tx->GetId());
        REQUIRE(it != wallet->mapWallet.end());
        it->second.SetMerkleBranch(chainActive.Tip(), 1);
        return it->second;
    }

    std::unique_ptr<CWallet> wallet;
};

TEST_CASE("ListCoins") {
    ListCoinsTestingSetup setup;
    std::string coinbaseAddress =
        setup.coinbaseKey.GetPubKey().GetKeyID().ToString();

    // Confirm ListCoins initially returns 1 coin grouped under coinbaseKey
    // address.
    std::map<CTxDestination, std::vector<COutput>> list;
    {
      LOCK2(cs_main, setup.wallet->cs_wallet);
      list = setup.wallet->ListCoins();
    }
    REQUIRE(list.size() == 1);
    REQUIRE(std::get<CKeyID>(list.begin()->first).ToString() ==
            coinbaseAddress);
    REQUIRE(list.begin()->second.size() == 1);

    // Check initial balance from one mature coinbase transaction.
    REQUIRE(500 * COIN == setup.wallet->GetAvailableBalance());

    // Add a transaction creating a change address, and confirm ListCoins still
    // returns the coin associated with the change address underneath the
    // coinbaseKey pubkey, even though the change address has a different
    // pubkey.
#ifdef DEBUG_THIS
    // Currently has issue due to not having a valid HD chaing

    AddTx(CRecipient{GetScriptForRawPubKey({}), 1 * COIN,
                     false /* subtract fee */});
    LOCK2(cs_main, wallet->cs_wallet);
    list = wallet->ListCoins();
    REQUIRE(list.size() == 1);
    REQUIRE(std::get<CKeyID>(list.begin()->first).ToString() ==
            coinbaseAddress);
    REQUIRE(list.begin()->second.size() == 2);

    // Lock both coins. Confirm number of available coins drops to 0.
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::vector<COutput> available;
        wallet->AvailableCoins(available);
        REQUIRE(available.size() == 2);
    }
    for (const auto &group : list) {
        for (const auto &coin : group.second) {
            LOCK(wallet->cs_wallet);
            wallet->LockCoin(COutPoint(coin.tx->GetId(), coin.i));
        }
    }
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::vector<COutput> available;
        wallet->AvailableCoins(available);
        REQUIRE(available.size() == 0);
    }
    // Confirm ListCoins still returns same result as before, despite coins
    // being locked.
    LOCK2(cs_main, wallet->cs_wallet);
    list = wallet->ListCoins();
    REQUIRE(list.size() == 1);
    REQUIRE(std::get<CKeyID>(list.begin()->first).ToString() ==
            coinbaseAddress);
    REQUIRE(list.begin()->second.size() == 2);
#endif
}
