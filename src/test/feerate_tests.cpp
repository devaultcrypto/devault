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
    // DeVault: CFeeRate::GetFee / GetFeeCeiling round the fee UP to a whole spock
    // (0.001 DVT = 100000 sat), matching legacy DeVault's quantizing Amount. Fees are therefore
    // always spock-multiples (or zero), regardless of the rate or size.
    const Amount SPOCK = SPOCK_SATS * SATOSHI; // 100000 sat = 0.001 DVT

    CFeeRate feeRate, altFeeRate;

    // Zero rate -> always zero.
    feeRate = CFeeRate(Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(100000), Amount::zero());

    // A sub-spock rate charges nothing for an empty tx but rounds any non-empty tx up to one spock.
    feeRate = CFeeRate(1000 * SATOSHI);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), SPOCK);
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), SPOCK);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), SPOCK);
    BOOST_CHECK_EQUAL(feeRate.GetFee(9000), SPOCK);

    // Negative (sub-spock) rate -> -1 spock for any non-empty tx.
    feeRate = CFeeRate(-1000 * SATOSHI);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), -SPOCK);
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), -SPOCK);

    // The DeVault min-relay rate (COIN/2 = 0.5 DVT/kB = 500 spocks/kB) -> exact spock-multiples,
    // rounding up where the satoshi result is not a whole spock.
    feeRate = CFeeRate(COIN / 2);
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1000), 500 * SPOCK); // 0.5 DVT
    BOOST_CHECK_EQUAL(feeRate.GetFee(226), 113 * SPOCK);  // 11.30M sat -> exactly 113 spocks
    BOOST_CHECK_EQUAL(feeRate.GetFee(225), 113 * SPOCK);  // 11.25M sat -> rounds up to 113 spocks
    BOOST_CHECK_EQUAL(feeRate.GetFee(2000), 1000 * SPOCK); // 1.0 DVT
    BOOST_CHECK_EQUAL(feeRate.GetFeeCeiling(226), 113 * SPOCK);

    // Every positive result is a whole spock, and at least one spock for a non-empty tx.
    feeRate = CFeeRate(7 * COIN / 3); // an awkward (non-spock) rate
    for (size_t n : {size_t(1), size_t(50), size_t(226), size_t(1000), size_t(9000)}) {
        const Amount f = feeRate.GetFee(n);
        BOOST_CHECK_EQUAL(f % SPOCK, Amount::zero());
        BOOST_CHECK(f >= SPOCK);
    }

    // Alternate constructor is consistent.
    feeRate = CFeeRate(COIN / 2);
    altFeeRate = CFeeRate(feeRate);
    BOOST_CHECK_EQUAL(feeRate.GetFee(226), altFeeRate.GetFee(226));

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
