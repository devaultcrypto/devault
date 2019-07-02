// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <test/test_bitcoin.h>

#include "catch_unit.h"

#include <array>

// BOOST_FIXTURE_TEST_SUITE(feerate_tests, BasicTestingSetup)

TEST_CASE("GetFeeTest") {
  CFeeRate feeRate, altFeeRate;

  feeRate = CFeeRate(Amount::zero());
  // Must always return 0
  BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
  BOOST_CHECK_EQUAL(feeRate.GetFee(1e5), Amount::zero());

  feeRate = CFeeRate(Amount::min_amount());
  // Must always just return the arg
  BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
  // BOOST_CHECK_EQUAL(feeRate.GetFee(1), SATOSHI);
  BOOST_CHECK_EQUAL(feeRate.GetFee(121 * 1000), 121 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(999 * 1000), 999 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(1000 * 1000), 1000 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(9000 * 1000), 9000 * Amount::min_amount());

  feeRate = CFeeRate(3 * Amount::min_amount());
  // Truncates the result, if not integer
  BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
  // Special case: returns 1 instead of 0
  BOOST_CHECK_EQUAL(feeRate.GetFee(8), Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(9), Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(121), Amount(14));
  BOOST_CHECK_EQUAL(feeRate.GetFee(122), Amount(15));

  BOOST_CHECK_EQUAL(feeRate.GetFee(999), 3 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(1000), 3 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(9000), 27 * Amount::min_amount());

  // Check ceiling results
  feeRate = CFeeRate(18 * Amount::min_amount());
  // Truncates the result, if not integer
  BOOST_CHECK_EQUAL(feeRate.GetFee(0), Amount::zero());
  BOOST_CHECK_EQUAL(feeRate.GetFee(100), 2 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(200), 4 * Amount::min_amount());
  BOOST_CHECK_EQUAL(feeRate.GetFee(1000), 18 * Amount::min_amount());

  // Check alternate constructor
  feeRate = CFeeRate(1000 * Amount::min_amount());
  altFeeRate = CFeeRate(feeRate);
  BOOST_CHECK_EQUAL(feeRate.GetFee(100), altFeeRate.GetFee(100));

  // Check full constructor
  // default value
  BOOST_CHECK(CFeeRate(Amount::zero(), 1000) == CFeeRate(Amount::zero()));
  BOOST_CHECK(CFeeRate(Amount::min_amount(), 1000) == CFeeRate(Amount::min_amount()));
  // lost precision (can only resolve satoshis per kB)
  BOOST_CHECK(CFeeRate(Amount::min_amount(), 1001) == CFeeRate(Amount::min_amount()));
  BOOST_CHECK(CFeeRate(2 * Amount::min_amount(), 1001) == CFeeRate(2 * Amount::min_amount()));

  // some more integer checks
  BOOST_CHECK(CFeeRate(26 * Amount::min_amount(), 789) == CFeeRate(33 * Amount::min_amount()));
  BOOST_CHECK(CFeeRate(27 * Amount::min_amount(), 789) == CFeeRate(35 * Amount::min_amount()));
  // Maximum size in bytes, should not crash
  CFeeRate(MAX_MONEY, std::numeric_limits<size_t>::max() >> 1).GetFeePerK();
}

// BOOST_AUTO_TEST_SUITE_END()
