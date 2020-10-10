// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Bitcoin developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/system.h>

unsigned int LwmaCalculateNextWorkRequired(const CBlockIndex* pindexPrev, const CBlockHeader *pblock, const Config &config) 
{
    const Consensus::Params &params = config.GetChainParams().GetConsensus();

    // This cannot handle the genesis block and early blocks in general.
    assert(pindexPrev);
    
    // Special difficulty rule for testnet:
    // If the new block's timestamp is more than 2* 10 minutes then allow
    // mining of a min-difficulty block.
    if (params.fPowAllowMinDifficultyBlocks &&
        (pblock->GetBlockTime() >
         pindexPrev->GetBlockTime() + 10 * params.nPowTargetSpacing)) {
        return UintToArith256(params.powLimit).GetCompact();
    }
  
    const int nHeight = pindexPrev->nHeight + 1;
  
    // Don't adjust difficult until we have a full window worth
    // this means we should also start the starting value
    // to a reasonable level !
    if (nHeight <= params.nZawyLwmaAveragingWindow) {
      return UintToArith256(params.powLimit).GetCompact();
    }
  
    const int64_t T = params.nPowTargetSpacing;
    const int N = params.nZawyLwmaAveragingWindow;
    const int k = (N+1) * T / 2;  // ignore adjust 0.9989^(500/N) from python code
    const int dnorm = 10;

    arith_uint256 sum_target;
    int t = 0, j = 0;

    // Loop through N most recent blocks.
    for (int i = nHeight - N; i < nHeight; i++) {
        const CBlockIndex* block = pindexPrev->GetAncestor(i);
        const CBlockIndex* block_Prev = block->GetAncestor(i - 1);
        int64_t solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();

        solvetime = std::min(6*T, solvetime);

        j++;
        t += solvetime * j;  // Weighted solvetime sum.

        // Target sum divided by a factor, (k N^2).
        // The factor is a part of the final equation. However we divide sum_target here to avoid
        // potential overflow.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sum_target += target / (k * N * N);
    }
    // Keep t reasonable in case strange solvetimes occurred.
    if (t < N * k / dnorm) {
        t = N * k / dnorm;
    }

    const arith_uint256 pow_limit = UintToArith256(params.powLimit);
    arith_uint256 next_target = t * sum_target;
    if (next_target > pow_limit) {
        next_target = pow_limit;
    }

    return next_target.GetCompact();
}

uint32_t GetNextWorkRequired(const CBlockIndex *pindexPrev,
                             const CBlockHeader *pblock, const Config &config) {
    const Consensus::Params &params = config.GetChainParams().GetConsensus();

    // GetNextWorkRequired should never be called on the genesis block
    assert(pindexPrev != nullptr);

    // Special rule for regtest: we never retarget.
    if (params.fPowNoRetargeting) {
        return pindexPrev->nBits;
    }

    return LwmaCalculateNextWorkRequired(pindexPrev, pblock, config);
}

bool CheckProofOfWork(uint256 hash, uint32_t nBits, const Config &config) {
  bool fNegative;
  bool fOverflow;
  arith_uint256 bnTarget;
  
  bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
  
  // Check range
  if (fNegative || bnTarget == 0 || fOverflow ||
      bnTarget >
      UintToArith256(config.GetChainParams().GetConsensus().powLimit)) {
    return false;
  }
  
  // Check proof of work matches claimed amount
  if (UintToArith256(hash) > bnTarget) {
    return false;
  }
  
  return true;
}
