// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>

class CBlockHeader;
class CBlockIndex;
class Config;
class uint256;

uint32_t GetNextWorkRequired(const CBlockIndex *pindexPrev,
                             const CBlockHeader *pblock, const Config &config);

/**
 * Check whether a block hash satisfies the proof-of-work requirement specified
 * by nBits
 */
bool CheckProofOfWork(uint256 hash, uint32_t nBits, const Config &config);

