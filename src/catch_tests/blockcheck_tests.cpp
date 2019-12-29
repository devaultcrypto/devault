// Copyright (c) 2013-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <config.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <validation.h>

#include <catch_tests/test_rand.h>

#include "catch_unit.h"

static void RunCheckOnBlockImpl(const GlobalConfig &config, const CBlock &block, CValidationState &state,
                                bool expected) {
  block.fChecked = false;
  bool fValid = CheckBlock(
                           config, block, state,
                           BlockValidationOptions(config).withCheckPoW(false).withCheckMerkleRoot(
                                                                                                  false));

  BOOST_CHECK_EQUAL(fValid, expected);
  BOOST_CHECK_EQUAL(fValid, state.IsValid());
}

static void RunCheckOnBlock(const GlobalConfig &config, const CBlock &block) {
  CValidationState state;
  RunCheckOnBlockImpl(config, block, state, true);
}

static void RunCheckOnBlock(const GlobalConfig &config, const CBlock &block, const std::string &reason) {
  CValidationState state;
  RunCheckOnBlockImpl(config, block, state, false);

  BOOST_CHECK_EQUAL(state.GetRejectCode(), REJECT_INVALID);
  BOOST_CHECK_EQUAL(state.GetRejectReason(), reason);
}

TEST_CASE("blockfail") {
  SelectParams(CBaseChainParams::MAIN);

  // Set max blocksize to default in case other tests left it dirty
  GlobalConfig config;
  config.SetMaxBlockSize(DEFAULT_MAX_BLOCK_SIZE);

  CBlock block;
  RunCheckOnBlock(config, block, "bad-cb-missing");

  CMutableTransaction tx;

  // Coinbase only.
  tx.vin.resize(1);
  tx.vin[0].scriptSig.resize(10);
  tx.vout.resize(1);
  tx.vout[0].nValue = Amount(42);
  auto coinbaseTx = CTransaction(tx);

  block.vtx.resize(1);
  block.vtx[0] = MakeTransactionRef(tx);
  RunCheckOnBlock(config, block);

  // No coinbase
  tx.vin[0].prevout = COutPoint(TxId(InsecureRand256()), 0);
  block.vtx[0] = MakeTransactionRef(tx);

  RunCheckOnBlock(config, block, "bad-cb-missing");

  // Invalid coinbase
  tx = CMutableTransaction(coinbaseTx);
  tx.vin[0].scriptSig.resize(0);
  block.vtx[0] = MakeTransactionRef(tx);

  RunCheckOnBlock(config, block, "bad-cb-length");

  // Oversize block.
  tx = CMutableTransaction(coinbaseTx);
  block.vtx[0] = MakeTransactionRef(tx);
  auto txSize = ::GetSerializeSize(tx, PROTOCOL_VERSION);
  auto maxTxCount = ((DEFAULT_MAX_BLOCK_SIZE - 1) / txSize) - 1;

  for (size_t i = 1; i < maxTxCount; i++) {
    tx.vin[0].prevout = COutPoint(TxId(InsecureRand256()), 0);
    block.vtx.push_back(MakeTransactionRef(tx));
  }

  // Check that at this point, we still accept the block.
  RunCheckOnBlock(config, block);

  // But reject it with one more transaction as it goes over the maximum
  // allowed block size.
  tx.vin[0].prevout = COutPoint(TxId(InsecureRand256()), 0);
  block.vtx.push_back(MakeTransactionRef(tx));
  RunCheckOnBlock(config, block, "bad-blk-length");
}
