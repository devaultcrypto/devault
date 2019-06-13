// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "serialize.h"

#include <cstdlib>
#include <ostream>
#include <string>
#include <type_traits>

const int64_t MIN_COIN = 100000;
const int64_t COIN_PRECISION = 100000000;

constexpr int64_t QuantizeAmount(int64_t am) {
  int64_t residual = (am % MIN_COIN);
  int64_t amount = (am - residual);
  if (residual > 0) amount += MIN_COIN;
  return amount;
}

struct Amount {
private:
    int64_t amount;
  

public:
    explicit constexpr Amount(int64_t _amount) : amount(QuantizeAmount(_amount)) {}
    constexpr Amount() : amount(0) {}
    constexpr Amount(const Amount &_camount) : amount(QuantizeAmount(_camount.amount)) {}

    static constexpr Amount zero() { return Amount(0); }
    static constexpr Amount satoshi() { return Amount(1); }
    static constexpr Amount min_coin() { return Amount(MIN_COIN); }

    /**
     * Implement standard operators
     */
    Amount &operator+=(const Amount a) {
        amount += a.amount;
        return *this;
    }
    Amount &operator-=(const Amount a) {
        amount -= a.amount;
        return *this;
    }

    /**
     * Equality
     */
    friend constexpr bool operator==(const Amount a, const Amount b) {
        return a.amount == b.amount;
    }
    friend constexpr bool operator!=(const Amount a, const Amount b) {
        return !(a == b);
    }

    /**
     * Comparison
     */
    friend constexpr bool operator<(const Amount a, const Amount b) {
        return a.amount < b.amount;
    }
    friend constexpr bool operator>(const Amount a, const Amount b) {
        return b < a;
    }
    friend constexpr bool operator<=(const Amount a, const Amount b) {
        return !(a > b);
    }
    friend constexpr bool operator>=(const Amount a, const Amount b) {
        return !(a < b);
    }

    /**
     * Unary minus
     */
    constexpr Amount operator-() const { return Amount(-amount); }

    /**
     * Addition and subtraction.
     */
    friend constexpr Amount operator+(const Amount a, const Amount b) {
        return Amount(a.amount + b.amount);
    }
    friend constexpr Amount operator-(const Amount a, const Amount b) {
        return a + -b;
    }

    /**
     * Multiplication
     */
    friend constexpr Amount operator*(const int64_t a, const Amount b) {
        return Amount(a * b.amount);
    }
    friend constexpr Amount operator*(const int a, const Amount b) {
        return Amount(a * b.amount);
    }
    constexpr Amount operator*(const Amount b) const {
        return Amount(amount * b.amount);
    }

    /**
     * Division
     */
    constexpr Amount operator/(const Amount b) const {
        return Amount(amount / b.amount);
    }
    constexpr Amount operator/(const int64_t b) const {
        return Amount(amount / b);
    }
    constexpr Amount operator/(const int b) const { return Amount(amount / b); }
    Amount &operator/=(const int64_t n) {
        amount /= n;
        return *this;
    }

    /**
     * Modulus
     */
    constexpr Amount operator%(const Amount b) const {
        return Amount(amount % b.amount);
    }
    constexpr Amount operator%(const int64_t b) const {
        return Amount(amount % b);
    }
    constexpr Amount operator%(const int b) const { return Amount(amount % b); }

    /**
     * Do not implement double ops to get an error with double and ensure
     * casting to integer is explicit.
     */
    friend constexpr Amount operator*(const double a, const Amount b) = delete;
    constexpr Amount operator/(const double b) const = delete;
    constexpr Amount operator%(const double b) const = delete;

    // ostream support
    friend std::ostream &operator<<(std::ostream &stream, const Amount &ca) {
        return stream << ca.amount;
    }

    std::string ToString() const;
    int64_t toInt() const { return amount; }

    // serialization support
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(amount);
    }
};

static constexpr Amount SATOSHI = Amount::satoshi();
static constexpr Amount MINCOIN = Amount::min_coin();
static constexpr Amount COIN(COIN_PRECISION);
static constexpr Amount CENT(COIN_PRECISION/100);

extern const std::string CURRENCY_UNIT;

/**
 * No amount larger than this (in satoshi) is valid.
 *
 * Note that this constant is *not* the total money supply, which in DeVault
 * should be between 4 & 5 billion (TBD), but rather a sanity check. It's set
 * at 2 Billion due to arithmetic integer limit.
 * As this sanity check is used by consensus-critical validation code, the exact
 * value of the MAX_MONEY constant is consensus critical; in unusual circumstances
 * like a(nother) overflow bug that allowed for the creation of coins out of thin
 * air modification could lead to a fork.
 */
static const Amount MAX_MONEY = 2000000000 * COIN; // 2 Billion
inline bool MoneyRange(const Amount nValue) {
    return nValue >= Amount::zero() && nValue <= MAX_MONEY;
}

