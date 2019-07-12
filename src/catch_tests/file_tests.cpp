// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs_util.h>
#include <random.h>

#include <test/test_bitcoin.h>

#include <cstdint>
#include <vector>

#include "catch_unit.h"

// BOOST_FIXTURE_TEST_SUITE(file_tests, BasicTestingSetup)

TEST_CASE("test_DirIsWritable") {
  // Should be able to write to the system tmp dir.
  fs::path tmpdirname = fs::temp_directory_path();
  BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), true);

  // Should not be able to write to a non-existent dir.
  std::string hexrandom = "rand" + GetRandString(16);
  tmpdirname = fs::temp_directory_path() / hexrandom;
  BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), false);

  fs::create_directory(tmpdirname);
  // Should be able to write to it now.
  BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), true);
  fs::remove(tmpdirname);
}

// BOOST_AUTO_TEST_SUITE_END()
