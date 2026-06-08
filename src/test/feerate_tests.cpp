// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2018-2021 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>

BOOST_FIXTURE_TEST_SUITE(feerate_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(GetFeeTest) {
    // DeVault: CFeeRate::GetFee / GetFeeCeiling (a) round the fee UP to a whole spock (0.001 DVT =
    // 100000 sat) matching legacy DeVault's quantizing Amount, and (b) apply the legacy flat floor
    // MIN_FEE = COIN/5 = 0.2 DVT to every non-empty tx (legacy feerate.cpp: std::max(nFee, MIN_FEE)).
    const Amount SPOCK = SPOCK_SATS * SATOSHI; // 100000 sat = 0.001 DVT
    const Amount MIN_FEE = COIN / 5;           // 0.2 DVT = 200 spocks -- the flat per-tx floor

    CFeeRate feeRate, altFeeRate;

    // Empty tx -> zero. Any non-empty tx pays at least MIN_FEE, even at a zero rate.
    feeRate = CFeeRate(Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), MIN_FEE);

    // A sub-MIN_FEE rate -> the flat MIN_FEE floor for any non-empty tx.
    feeRate = CFeeRate(1000 * SATOSHI);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), MIN_FEE);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9000), MIN_FEE);

    // The DeVault min-relay rate (COIN/2 = 0.5 DVT/kB). Small txs hit the MIN_FEE floor; larger txs
    // pay the (spock-quantized) rate once it exceeds MIN_FEE (crossover ~400 bytes).
    feeRate = CFeeRate(COIN / 2);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(225), MIN_FEE);        // 113 spocks < 200 -> floored to 0.2 DVT
    BOOST_CHECK_EQUAL(feeRate.GetFee(226), MIN_FEE);        // 113 spocks < 200 -> floored
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), 500 * SPOCK);   // 0.5 DVT > MIN_FEE -> rate
    BOOST_CHECK_EQUAL(feeRate.GetFee(2000), 1000 * SPOCK);  // 1.0 DVT
    BOOST_CHECK_EQUAL(feeRate.GetFeeCeiling(225), MIN_FEE);

    // Every non-empty result is a whole spock and at least MIN_FEE.
    feeRate = CFeeRate(7 * COIN / 3); // an awkward (non-spock) rate
    for (size_t n : {size_t(1), size_t(50), size_t(226), size_t(1000), size_t(9000)}) {
        const Amount f = feeRate.GetFee(n);
        BOOST_CHECK_EQUAL(f % SPOCK, Amount::zero());
        BOOST_CHECK(f >= MIN_FEE);
    }

    // Alternate constructor is consistent.
    feeRate = CFeeRate(COIN / 2);
    altFeeRate = CFeeRate(feeRate);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), altFeeRate.GetFee(1000));

    // Full constructor (fee, bytes) computes the rate; the rate itself is not spock-quantized.
    BOOST_CHECK(CFeeRate(Amount::zero(), 0) == CFeeRate(Amount::zero()));
    BOOST_CHECK(CFeeRate(COIN / 2, 1000) == CFeeRate(COIN / 2));
    // Maximum size in bytes, should not crash.
    CFeeRate(MAX_MONEY, std::numeric_limits<size_t>::max() >> 1).GetFeePerK();
}

BOOST_AUTO_TEST_CASE(ToString) {
    BOOST_CHECK_EQUAL(CFeeRate{Amount::zero()}.ToString(), "0.00000000 BCH/kB");
    BOOST_CHECK_EQUAL(CFeeRate{SATOSHI}.ToString(), "0.00000001 BCH/kB");
    BOOST_CHECK_EQUAL(CFeeRate{Amount{123'456'000 * SATOSHI}}.ToString(), "1.23456000 BCH/kB");
    BOOST_CHECK_EQUAL(CFeeRate{Amount{1230 * COIN}}.ToString(), "1230.00000000 BCH/kB");
    //BOOST_CHECK_EQUAL(CFeeRate{Amount{-123'456'000 * SATOSHI}}.ToString(), "-1.23456000 BCH/kB");
    // test fails with current implementation! ("-1.-23456000")
    BOOST_CHECK_EQUAL(CFeeRate{Amount{-1230 * COIN}}.ToString(), "-1230.00000000 BCH/kB");
}

BOOST_AUTO_TEST_SUITE_END()
