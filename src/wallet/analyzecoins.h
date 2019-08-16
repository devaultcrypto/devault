// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <wallet/wallet.h>
std::vector<CInputCoin> analyzecoins(const std::map<CTxDestination, std::vector<COutput>> &coinList, double minPercent);
