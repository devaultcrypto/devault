// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include "amount.h"
#include "uint256.h"

#include <map>
#include <set>
#include <string>

namespace Consensus {

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    /** Unix time used for MTP activation of 15 May 2019 12:00:00 UTC upgrade */
    int greatWallActivationTime;
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nBlocksPerYear;
    int64_t nInitialMiningRewardInCoins;
    int64_t minerCapSystemChangeHeight;

    // Rewards
    std::vector<int64_t> nPerCentPerYear;
    int64_t nMinRewardBlocks;
    Amount nMinRewardBalance;
    Amount nMinReward;
    Amount nMaxReward;
  
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    // Params for Zawy's LWMA difficulty adjustment algorithm.
    int64_t nZawyLwmaAveragingWindow;

};
} // namespace Consensus

#endif // DEVAULT_CONSENSUS_PARAMS_H
