// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/activation.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <util.h>

bool IsGreatWallEnabled(const Config &config, const CBlockIndex *pindexPrev) {
  if (pindexPrev == nullptr) {
    return false;
  }
  
  return pindexPrev->GetMedianTimePast() >=
  gArgs.GetArg(
               "-greatwallactivationtime",
               config.GetChainParams().GetConsensus().greatWallActivationTime);
}
