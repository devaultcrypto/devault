// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <array>
#include <test/test_bitcoin.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

static void CheckAmounts(int64_t a_val, int64_t b_val) {
  int64_t aval = a_val * Amount::min_amount().toInt();
  int64_t bval = b_val * Amount::min_amount().toInt();
  Amount a(aval), b(bval);

  // Equality
  REQUIRE((a == b) == (aval == bval));
  REQUIRE((b == a) == (aval == bval));

  REQUIRE((a != b) == (aval != bval));
  REQUIRE((b != a) == (aval != bval));

  // Comparison
  REQUIRE((a < b) == (aval < bval));
  REQUIRE((b < a) == (bval < aval));

  REQUIRE((a > b) == (aval > bval));
  REQUIRE((b > a) == (bval > aval));

  REQUIRE((a <= b) == (aval <= bval));
  REQUIRE((b <= a) == (bval <= aval));

  REQUIRE((a >= b) == (aval >= bval));
  REQUIRE((b >= a) == (bval >= aval));

  // Unary minus
  REQUIRE(-a == Amount(-aval));
  REQUIRE(-b == Amount(-bval));

  // Addition and subtraction.
  REQUIRE(a + b == b + a);
  REQUIRE(a + b == Amount(aval + bval));

  REQUIRE((a - b) == (-(b - a)));
  REQUIRE((a - b) == Amount(aval - bval));

  // Multiplication
  REQUIRE((aval * b) == (bval * a));
  REQUIRE((aval * b) == Amount(aval * bval));

  // Division by int
  if (b != Amount::zero()) { REQUIRE((a / bval) == Amount(aval / bval)); }

  if (a != Amount::zero()) { REQUIRE((b / aval) == Amount(bval / aval)); }

  // Modulus
  if (b != Amount::zero()) {
    REQUIRE((a % b) == (a % bval));
    REQUIRE((a % b) == Amount(aval % bval));
  }

  if (a != Amount::zero()) {
    REQUIRE((b % a) == (b % aval));
    REQUIRE((b % a) == Amount(bval % aval));
  }

  // OpAssign
  Amount v;
  REQUIRE(v == Amount::zero());
  v += a;
  REQUIRE(v == a);
  v += b;
  REQUIRE(v == a + b);
  v += b;
  REQUIRE(v == a + 2 * b);
  v -= 2 * a;
  REQUIRE(v == 2 * b - a);
}

TEST_CASE("AmountTests") {
  std::array<int64_t, 8> values = {{-23, -1, 0, 1, 2, 3, 42, 99999999}};

  for (int64_t i : values) {
    for (int64_t j : values) {
      CheckAmounts(i, j);
    }
  }

  REQUIRE(COIN + COIN == 2 * COIN);
  REQUIRE(2 * COIN + COIN == 3 * COIN);
  REQUIRE(-1 * COIN + COIN == Amount::zero());

  REQUIRE(COIN - COIN == Amount::zero());
  REQUIRE(COIN - 2 * COIN == -1 * COIN);
}

TEST_CASE("MoneyRangeTest") {
  REQUIRE(MoneyRange(Amount(-Amount::min_amount())) == false);
  REQUIRE(MoneyRange(MAX_MONEY + Amount::min_amount()) == false);
  REQUIRE(MoneyRange(Amount::min_amount()) == true);
}

TEST_CASE("BinaryOperatorTest") {
  CFeeRate a, b;
  a = CFeeRate(1 * Amount::min_amount());
  b = CFeeRate(2 * Amount::min_amount());
  REQUIRE(a < b);
  REQUIRE(b > a);
  REQUIRE(a == a);
  REQUIRE(a <= b);
  REQUIRE(a <= a);
  REQUIRE(b >= a);
  REQUIRE(b >= b);
  // a should be 0.00000002 BTC/kB now
  a += a;
  REQUIRE(a == b);
}

TEST_CASE("ToStringTest") {
  CFeeRate feeRate;
  feeRate = CFeeRate(Amount::min_amount());
  REQUIRE(feeRate.ToString() == "0.001 DVT/kB");
}
