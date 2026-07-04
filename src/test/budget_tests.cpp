// Copyright (c) 2026 The DeVault developers (V2 port)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/budget.h>

#include <amount.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <validation.h> // GetBlockSubsidy

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <set>
#include <vector>

// Mainnet params guard the real (consensus) payout table; a separate regtest suite below guards the
// V2-only dvreg: budget column that makes regtest superblocks mineable.
struct MainNetSetup : public BasicTestingSetup {
    MainNetSetup() : BasicTestingSetup(CBaseChainParams::MAIN) {}
};

BOOST_FIXTURE_TEST_SUITE(budget_tests, MainNetSetup)

// The miner's FillSuperBlockBudget must build a coinbase that the validator CheckSuperBlockBudget
// accepts — block production and validation must agree on the superblock budget. Both go through the
// same ComputeSuperBlockPayments, so this also guards the CheckSuperBlockBudget refactor (3D.7).
BOOST_AUTO_TEST_CASE(fill_matches_check_at_superblock) {
    const CChainParams &params = Params();
    const int superHeight = 21915; // first superblock (nBlocksPerYear / 12)
    const Amount subsidy = GetBlockSubsidy(superHeight, params.GetConsensus());

    // Coinbase with just the miner output; let the miner append the budget.
    CMutableTransaction cb;
    cb.vin.resize(1);
    cb.vin[0].prevout = COutPoint();
    cb.vout.resize(1);
    cb.vout[0] = CTxOut(subsidy, CScript() << OP_TRUE);

    const size_t before = cb.vout.size();
    const bool isSuper = FillSuperBlockBudget(cb, superHeight, subsidy, params);
    BOOST_CHECK(isSuper);                 // 21915 is a superblock
    BOOST_CHECK(cb.vout.size() > before); // budget outputs were appended

    // The validator must accept exactly the coinbase the miner produced.
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(cb));
    Amount nBudgetReward = -SATOSHI;
    const bool ok = CheckSuperBlockBudget(block, superHeight, subsidy, params, nBudgetReward);
    BOOST_CHECK(ok);                             // fill == check
    BOOST_CHECK(nBudgetReward > Amount::zero()); // a non-zero budget was paid

    // Tampering with a budget payout must make the validator reject (sanity that the check has teeth).
    CMutableTransaction tampered(cb);
    tampered.vout.back().nValue += COIN;
    CBlock badBlock;
    badBlock.vtx.push_back(MakeTransactionRef(tampered));
    Amount ignored;
    BOOST_CHECK(!CheckSuperBlockBudget(badBlock, superHeight, subsidy, params, ignored));
}

// A non-superblock height adds nothing and validates as a zero budget.
BOOST_AUTO_TEST_CASE(non_superblock_is_noop) {
    const CChainParams &params = Params();
    const int height = 21916; // not a multiple of nBlocksPerYear / 12
    const Amount subsidy = GetBlockSubsidy(height, params.GetConsensus());

    CMutableTransaction cb;
    cb.vin.resize(1);
    cb.vin[0].prevout = COutPoint();
    cb.vout.resize(1);
    cb.vout[0] = CTxOut(subsidy, CScript() << OP_TRUE);

    const bool isSuper = FillSuperBlockBudget(cb, height, subsidy, params);
    BOOST_CHECK(!isSuper);                     // not a superblock
    BOOST_CHECK_EQUAL(cb.vout.size(), size_t(1)); // coinbase untouched

    CBlock block;
    block.vtx.push_back(MakeTransactionRef(cb));
    Amount nBudgetReward = -SATOSHI;
    BOOST_CHECK(CheckSuperBlockBudget(block, height, subsidy, params, nBudgetReward));
    BOOST_CHECK_EQUAL(nBudgetReward / SATOSHI, int64_t(0));
}

BOOST_AUTO_TEST_SUITE_END()

// Regtest budget addresses (V2-only dvreg: column). Legacy had none — its dvtest: strings do not
// decode under the dvreg: prefix, so every regtest budget payout script came out EMPTY and the first
// regtest superblock (21915) could neither be mined nor validated. Guard that the dvreg: column
// yields real, distinct scripts and that mine==validate holds on regtest like the other nets.
struct RegTestSetup : public BasicTestingSetup {
    RegTestSetup() : BasicTestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(budget_regtest_tests, RegTestSetup)

BOOST_AUTO_TEST_CASE(regtest_superblock_fill_matches_check) {
    const CChainParams &params = Params();
    const int superHeight = 21915; // first superblock (regtest keeps nBlocksPerYear / 12)
    const Amount subsidy = GetBlockSubsidy(superHeight, params.GetConsensus());

    CMutableTransaction cb;
    cb.vin.resize(1);
    cb.vin[0].prevout = COutPoint();
    cb.vout.resize(1);
    cb.vout[0] = CTxOut(subsidy, CScript() << OP_TRUE);

    BOOST_CHECK(FillSuperBlockBudget(cb, superHeight, subsidy, params));
    BOOST_CHECK_EQUAL(cb.vout.size(), size_t(1 + 6)); // 6 payees in the first epoch

    // The old failure mode: undecodable addresses -> empty scripts (which also collide with each
    // other, double-counting in the validator). Every payout script must be non-empty and distinct.
    std::set<std::vector<uint8_t>> scripts;
    for (size_t o = 1; o < cb.vout.size(); ++o) {
        const CScript &s = cb.vout[o].scriptPubKey;
        BOOST_CHECK(!s.empty());
        scripts.emplace(s.begin(), s.end());
    }
    BOOST_CHECK_EQUAL(scripts.size(), size_t(6));

    CBlock block;
    block.vtx.push_back(MakeTransactionRef(cb));
    Amount nBudgetReward = -SATOSHI;
    BOOST_CHECK(CheckSuperBlockBudget(block, superHeight, subsidy, params, nBudgetReward));
    BOOST_CHECK(nBudgetReward > Amount::zero());
}

BOOST_AUTO_TEST_SUITE_END()
