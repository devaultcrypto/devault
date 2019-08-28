// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compressor.h>
#include <catch_tests/test_bitcoin.h>
#include <util.h>

#include <cstdint>

#include "catch_unit.h"

// amounts 0.00000001 .. 0.00100000
#define NUM_MULTIPLES_UNIT 100000

// amounts 0.01 .. 100.00
#define NUM_MULTIPLES_CENT 10000

// amounts 1 .. 10000
#define NUM_MULTIPLES_1BCH 10000

// amounts 50 .. 21000000
#define NUM_MULTIPLES_50BCH 420000

static bool TestEncode(Amount in) {
  return in.toInt() == CTxOutCompressor::DecompressAmount(CTxOutCompressor::CompressAmount(in.toInt()));
}

static bool TestDecode(uint64_t in) {
  return in == CTxOutCompressor::CompressAmount(CTxOutCompressor::DecompressAmount(in));
}

static bool TestPair(Amount dec, uint64_t enc) {
  return CTxOutCompressor::CompressAmount(dec.toInt()) == enc && CTxOutCompressor::DecompressAmount(enc) == dec.toInt();
}

TEST_CASE("compress_amounts") {
  BOOST_CHECK(TestPair(Amount::zero(), 0x0));
  BOOST_CHECK(TestPair(Amount::min_amount(), 0x6));
  BOOST_CHECK(TestPair(CENT, 0x7));
  BOOST_CHECK(TestPair(COIN, 0x9));
  BOOST_CHECK(TestPair(50 * COIN, 0x32));
  BOOST_CHECK(TestPair(21000000 * COIN, 0x1406f40));

  for (int64_t i = 1; i <= NUM_MULTIPLES_UNIT; i++) {
    BOOST_CHECK(TestEncode(Amount(i)));
  }

  for (int64_t i = 1; i <= NUM_MULTIPLES_CENT; i++) {
    BOOST_CHECK(TestEncode(i * CENT));
  }

  for (int64_t i = 1; i <= NUM_MULTIPLES_1BCH; i++) {
    BOOST_CHECK(TestEncode(i * COIN));
  }

  for (int64_t i = 1; i <= NUM_MULTIPLES_50BCH; i++) {
    BOOST_CHECK(TestEncode(i * 50 * COIN));
  }

  for (int64_t i = 0; i < 100000; i++) {
    BOOST_CHECK(TestDecode(i));
  }
}
