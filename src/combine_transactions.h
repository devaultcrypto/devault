// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>

class CMutableTransaction;
CMutableTransaction combine_transactions(const std::vector<CMutableTransaction>& txVariants,
                                         bool check_coins=false);

 
