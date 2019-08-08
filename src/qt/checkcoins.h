// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <interfaces/wallet.h>
std::string checkcoins(const interfaces::Wallet::CoinsList &coinList);
