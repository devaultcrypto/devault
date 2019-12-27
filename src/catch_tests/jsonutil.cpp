// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch_tests/jsonutil.h>

#include <catch2/catch.hpp>
#include <iostream>

#define BOOST_ERROR(A) std::cout << (A) << "\n";

UniValue read_json(const std::string &jsondata) {
  UniValue v;

  if (!v.read(jsondata) || !v.isArray()) {
    BOOST_ERROR("Parse error.");
    return UniValue(UniValue::VARR);
  }
  return v.get_array();
}
