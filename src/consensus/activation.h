// Copyright (c) 2018 The Bitcoin developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

class CBlockIndex;
class Config;

/** Check BLS related protocol upgrade has activated. */
bool IsBLSEnabled(const Config &config, const CBlockIndex *pindexPrev);

