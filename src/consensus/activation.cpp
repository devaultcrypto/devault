// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/activation.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/params.h>
#include <util/system.h>

bool IsBLSEnabled(const Config &config, const CBlockIndex *pindexPrev) {
  if (pindexPrev == nullptr) {
    return false;
  }
  
  return pindexPrev->GetMedianTimePast() >=
  gArgs.GetArg(
               "-blsactivationtime",
               config.GetChainParams().GetConsensus().blsActivationTime);
}
