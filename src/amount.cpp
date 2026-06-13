// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2022 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>

#include <tinyformat.h>

const std::string CURRENCY_UNIT = "DVT";

std::string Amount::ToString(bool trimTrailingZeros, bool trimTrailingDecimalPoint) const {
    std::string result{strprintf("%d", amount)};

    unsigned int start = result.front() == '-' ? 1u : 0u;
    if (result.length() - start <= 8u) result.insert(start, std::string(9u + start - result.length(), '0')); // if value is less than 1 coin, left-pad zeros
    result.insert(result.length() - 8u, "."); // decimal place in the correct location

    if (trimTrailingZeros) {
        result.resize(result.find_last_not_of('0') + 1u);
        if (result.back() == '.') { // if the decimal point would be the last character...
            if (trimTrailingDecimalPoint) result.resize(result.length() - 1u); // strip the decimal point as well
            else result += '0'; // we want to keep the decimal, so append back a single .0
        }
    }
    return result;
}
