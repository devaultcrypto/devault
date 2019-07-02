// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <pow.h>
#include <random.h>
#include <test/test_bitcoin.h>
#include <util.h>

#include "catch_unit.h"

static CBlockIndex GetBlockIndex(CBlockIndex *pindexPrev, int64_t nTimeInterval, uint32_t nBits) {
  CBlockIndex block;
  block.pprev = pindexPrev;
  block.nHeight = pindexPrev->nHeight + 1;
  block.nTime = pindexPrev->nTime + nTimeInterval;
  block.nBits = nBits;

  block.nChainWork = pindexPrev->nChainWork + GetBlockProof(block);
  return block;
}

static void make_blocks(std::vector<CBlockIndex> &blocks, DummyConfig &config) {

  blocks.resize(100);
  const int interval = 2 * 60; // 2 minutes
  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  const arith_uint256 powLimit = UintToArith256(params.powLimit);
  arith_uint256 currentPow = powLimit >> 1;
  uint32_t initialBits = currentPow.GetCompact();

  // Genesis block.
  const uint256 zeroHash = uint256();
  blocks[0] = CBlockIndex();
  blocks[0].nHeight = 0;
  blocks[0].nTime = 1269211443;
  blocks[0].nBits = initialBits;
  blocks[0].nChainWork = GetBlockProof(blocks[0]);
  blocks[0].phashBlock = &zeroHash;

  // Pile up some blocks.
  for (size_t i = 1; i < 100; i++) {
    blocks[i] = GetBlockIndex(&blocks[i - 1], interval, initialBits);
    blocks[i].phashBlock = &zeroHash;
  }
}

// BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
TEST_CASE("get_next_work") {
  DummyConfig config(CBaseChainParams::MAIN);
  std::vector<CBlockIndex> blocks;
  make_blocks(blocks, config);
  CBlockHeader blkHeader = blocks[99].GetBlockHeader();
  BOOST_CHECK_EQUAL(GetNextWorkRequired(&blocks[99], &blkHeader, config), 0x1c7ffffe);
}

// TBD

/* Test the constraint on the upper bound for next work */
// TEST_CASE("get_next_work_pow_limit") {
/* Test the constraint on the lower bound for actual time taken */
// TEST_CASE("get_next_work_lower_limit_actual") {
/* Test the constraint on the upper bound for actual time taken */
// TEST_CASE("get_next_work_upper_limit_actual") {

TEST_CASE("GetBlockProofEquivalentTime_test") {
  DummyConfig config(CBaseChainParams::MAIN);

  std::vector<CBlockIndex> blocks(10000);
  for (int i = 0; i < 10000; i++) {
    blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
    blocks[i].nHeight = i;
    blocks[i].nTime = 1269211443 + i * config.GetChainParams().GetConsensus().nPowTargetSpacing;
    blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
    blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i]) : arith_uint256(0);
  }

  for (int j = 0; j < 1000; j++) {
    CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
    CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
    CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

    int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, config.GetChainParams().GetConsensus());
    BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
  }
}

TEST_CASE("retargeting_test") {
  DummyConfig config(CBaseChainParams::MAIN);

  const int long_interval = 24 * 60; // 24 minutes

  std::vector<CBlockIndex> blocks(200);

  const Consensus::Params &params = config.GetChainParams().GetConsensus();
  const arith_uint256 powLimit = UintToArith256(params.powLimit);
  arith_uint256 currentPow = powLimit >> 1;
  uint32_t initialBits = currentPow.GetCompact();

  // Genesis block.
  blocks[0] = CBlockIndex();
  blocks[0].nHeight = 0;
  blocks[0].nTime = 1269211443;
  blocks[0].nBits = initialBits;

  blocks[0].nChainWork = GetBlockProof(blocks[0]);

  // Pile up some blocks.
  for (size_t i = 1; i < 100; i++) {
    blocks[i] = GetBlockIndex(&blocks[i - 1], params.nPowTargetSpacing, initialBits);
  }

  CBlockHeader blkHeaderDummy;

  // Now we expect the difficulty to decrease.
  blocks[100] = GetBlockIndex(&blocks[99], long_interval, initialBits);
  BOOST_CHECK_EQUAL(GetNextWorkRequired(&blocks[100], &blkHeaderDummy, config), 0x1D009188);

  blocks[101] = GetBlockIndex(&blocks[100], long_interval, currentPow.GetCompact());
  BOOST_CHECK_EQUAL(GetNextWorkRequired(&blocks[101], &blkHeaderDummy, config), 0x1D00A2D3);

  // Drop down to minimum difficulty
  for (int i = 0; i < 33; i++) {
    blocks[102 + i] = GetBlockIndex(&blocks[101 + i], long_interval, powLimit.GetCompact());
  }
  // Once we reached the minimal difficulty, we stick with it.
  for (int i = 0; i < 32; i++) {
    blocks[135 + i] = GetBlockIndex(&blocks[134 + i], long_interval, powLimit.GetCompact());
    BOOST_CHECK_EQUAL(GetNextWorkRequired(&blocks[135 + i], &blkHeaderDummy, config), powLimit.GetCompact());
  }
}

// BOOST_AUTO_TEST_SUITE_END()
