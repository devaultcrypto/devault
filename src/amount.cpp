// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"

#include "tinyformat.h"

std::string Amount::ToString() const {
    // Note: not using straight sprintf here because we do NOT want localized number formatting.
    return strprintf("%d.%03d", this->toIntCoins(), (*this % COIN).toInt()/Amount::MIN_AMOUNT);
}

