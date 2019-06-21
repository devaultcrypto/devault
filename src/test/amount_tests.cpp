// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

#include <array>

BOOST_FIXTURE_TEST_SUITE(amount_tests, BasicTestingSetup)

static void CheckAmounts(int64_t a_val, int64_t b_val) {
    int64_t aval = a_val * Amount::min_amount().toInt();
    int64_t bval = b_val * Amount::min_amount().toInt();
    Amount a(aval), b(bval);

    // Equality
    BOOST_CHECK_EQUAL(a == b, aval == bval);
    BOOST_CHECK_EQUAL(b == a, aval == bval);

    BOOST_CHECK_EQUAL(a != b, aval != bval);
    BOOST_CHECK_EQUAL(b != a, aval != bval);

    // Comparison
    BOOST_CHECK_EQUAL(a < b, aval < bval);
    BOOST_CHECK_EQUAL(b < a, bval < aval);

    BOOST_CHECK_EQUAL(a > b, aval > bval);
    BOOST_CHECK_EQUAL(b > a, bval > aval);

    BOOST_CHECK_EQUAL(a <= b, aval <= bval);
    BOOST_CHECK_EQUAL(b <= a, bval <= aval);

    BOOST_CHECK_EQUAL(a >= b, aval >= bval);
    BOOST_CHECK_EQUAL(b >= a, bval >= aval);

    // Unary minus
    BOOST_CHECK_EQUAL(-a, Amount(-aval));
    BOOST_CHECK_EQUAL(-b, Amount(-bval));

    // Addition and subtraction.
    BOOST_CHECK_EQUAL(a + b, b + a);
    BOOST_CHECK_EQUAL(a + b, Amount(aval + bval));

    BOOST_CHECK_EQUAL(a - b, -(b - a));
    BOOST_CHECK_EQUAL(a - b, Amount(aval - bval));

    // Multiplication
    BOOST_CHECK_EQUAL(aval * b, bval * a);
    BOOST_CHECK_EQUAL(aval * b, Amount(aval * bval));

    // Division by int
    if (b != Amount::zero()) {
        BOOST_CHECK_EQUAL(a / bval, Amount(aval / bval));
    }

    if (a != Amount::zero()) {
        BOOST_CHECK_EQUAL(b / aval, Amount(bval / aval));
    }

    // Modulus
    if (b != Amount::zero()) {
        BOOST_CHECK_EQUAL(a % b, a % bval);
        BOOST_CHECK_EQUAL(a % b, Amount(aval % bval));
    }

    if (a != Amount::zero()) {
        BOOST_CHECK_EQUAL(b % a, b % aval);
        BOOST_CHECK_EQUAL(b % a, Amount(bval % aval));
    }

    // OpAssign
    Amount v;
    BOOST_CHECK_EQUAL(v, Amount::zero());
    v += a;
    BOOST_CHECK_EQUAL(v, a);
    v += b;
    BOOST_CHECK_EQUAL(v, a + b);
    v += b;
    BOOST_CHECK_EQUAL(v, a + 2 * b);
    v -= 2 * a;
    BOOST_CHECK_EQUAL(v, 2 * b - a);
}

BOOST_AUTO_TEST_CASE(AmountTests) {
    std::array<int64_t, 8> values = {{-23, -1, 0, 1, 2, 3, 42, 99999999}};

    for (int64_t i : values) {
        for (int64_t j : values) {
            CheckAmounts(i, j);
        }
    }

    BOOST_CHECK_EQUAL(COIN + COIN, 2 * COIN);
    BOOST_CHECK_EQUAL(2 * COIN + COIN, 3 * COIN);
    BOOST_CHECK_EQUAL(-1 * COIN + COIN, Amount::zero());

    BOOST_CHECK_EQUAL(COIN - COIN, Amount::zero());
    BOOST_CHECK_EQUAL(COIN - 2 * COIN, -1 * COIN);
}

BOOST_AUTO_TEST_CASE(MoneyRangeTest) {
    BOOST_CHECK_EQUAL(MoneyRange(Amount(-Amount::min_amount())), false);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY + Amount::min_amount()), false);
    BOOST_CHECK_EQUAL(MoneyRange(Amount::min_amount()), true);
}

BOOST_AUTO_TEST_CASE(BinaryOperatorTest) {
    CFeeRate a, b;
    a = CFeeRate(1*Amount::min_amount());
    b = CFeeRate(2*Amount::min_amount());
    BOOST_CHECK(a < b);
    BOOST_CHECK(b > a);
    BOOST_CHECK(a == a);
    BOOST_CHECK(a <= b);
    BOOST_CHECK(a <= a);
    BOOST_CHECK(b >= a);
    BOOST_CHECK(b >= b);
    // a should be 0.00000002 BTC/kB now
    a += a;
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(ToStringTest) {
    CFeeRate feeRate;
    feeRate = CFeeRate(Amount::min_amount());
    BOOST_CHECK_EQUAL(feeRate.ToString(), "0.001 DVT/kB");
}

BOOST_AUTO_TEST_SUITE_END()
