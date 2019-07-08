// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include <checkpoints.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/validation.h>
#include <streams.h>
#include <uint256.h>
#include <utilstrencodings.h>
#include <validation.h>

#include <test/test_bitcoin.h>
#include <uint256.h>

#include "catch_unit.h"

BOOST_AUTO_TEST_CASE(sanity) {
  const auto params = CreateChainParams(CBaseChainParams::MAIN);
  const CCheckpointData &checkpoints = params->Checkpoints();
  uint256 p5000 = uint256S("000000000000000173c13a23fed27056b5a76912a27d62064cb988db13888907");
  uint256 p0 = uint256S("0000000038e62464371566f6a8d35c01aa54a7da351b2dbf85d92f30357f3a90");
  uint256 p1 = uint256S("173c13a23fed27056b5a76912a27d62064cb988");
  uint256 p2 = uint256S("38e62464371566f6a8d35c01aa54a7da351b2dbf85d92f");

  BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 5000, p5000));
  BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 0, p0));

  // Wrong hashes at checkpoints should fail:
  BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 5000, p1));
  BOOST_CHECK(!Checkpoints::CheckBlock(checkpoints, 0, p2));

  // ... but any hash not at a checkpoint should succeed:
  BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 1, p1));
  BOOST_CHECK(Checkpoints::CheckBlock(checkpoints, 5000 + 1, p2));
}

TEST_CASE("ban_fork_at_genesis_block") {
  DummyConfig config;

  // Sanity check that a checkpoint exists at the genesis block
  auto &checkpoints = config.GetChainParams().Checkpoints().mapCheckpoints;
  assert(checkpoints.find(0) != checkpoints.end());

  // Another precomputed genesis block (with differing nTime) should conflict
  // with the regnet genesis block checkpoint and not be accepted or stored
  // in memory.
  CBlockHeader header = CreateGenesisBlock(1296688603, 2, 0x207fffff, 1, 50 * COIN);

  // Header should not be accepted
  CValidationState state;
  CBlockHeader invalid;
  const CBlockIndex *pindex = nullptr;
  BOOST_CHECK(!ProcessNewBlockHeaders(config, {header}, state, &pindex, &invalid));
  BOOST_CHECK(state.IsInvalid());
  BOOST_CHECK(pindex == nullptr);
  BOOST_CHECK(invalid.GetHash() == header.GetHash());

  // Sanity check to ensure header was not saved in memory
  {
    LOCK(cs_main);
    BOOST_CHECK(LookupBlockIndex(header.GetHash()) == nullptr);
  }
}

class ChainParamsWithCheckpoints : public CChainParams {
  public:
  ChainParamsWithCheckpoints(const CChainParams &chainParams, CCheckpointData &checkpoints)
      : CChainParams(chainParams) {
    checkpointData = checkpoints;
  }
};

class MainnetConfigWithTestCheckpoints : public DummyConfig {
  public:
  MainnetConfigWithTestCheckpoints() : DummyConfig(createChainParams()) {}

  static std::unique_ptr<CChainParams> createChainParams() {
    CCheckpointData checkpoints = {.mapCheckpoints = {
                                       {2, uint256S("000000006a625f06636b8bb6ac7b960a8d0"
                                                    "3705d1ace08b1a19da3fdcc99ddbd")},
                                   }};
    const auto mainParams = CreateChainParams(CBaseChainParams::MAIN);
    return std::make_unique<ChainParamsWithCheckpoints>(*mainParams, checkpoints);
  }
};

/**
 * This test has first 4 blocks mined ontop of the genesis block:
 * checkpoint is wrong
 */

TEST_CASE("ban_fork_prior_to_and_at_checkpoints") {
  MainnetConfigWithTestCheckpoints config;

  CValidationState state;
  CBlockHeader invalid;
  const CBlockIndex *pindex = nullptr;

  // Start with mainnet genesis block
  CBlockHeader headerG = config.GetChainParams().GenesisBlock();
  BOOST_CHECK(headerG.GetHash() == uint256S("0000000038e62464371566f6a8d35c01aa54a7da351b2dbf85d92f30357f3a90"));
  BOOST_CHECK(ProcessNewBlockHeaders(config, {headerG}, state, &pindex, &invalid));
  pindex = nullptr;

  CBlockHeader headerA, headerB, headerAA, headerAB;
  CDataStream stream = CDataStream(
      ParseHex("00000020903a7f35302fd985bf2d1b35daa754aa015cd3a8f66615376424e63800000000a1f0872be30c0b6c625b5b613ad85d5"
               "64ecf73dfd7c911e143f2c17a95f95d86b28ff65cffff001d1caf48790101000000010000000000000000000000000000000000"
               "000000000000000000000000000000ffffffff21010100feccfe0c00fe9d390e000963676d696e6572343208000000000000000"
               "000ffffffff0100743ba40b0000001976a91480d057b26c1768ef56420a5c752ca7d3ed4a378288ac00000000"),

      SER_NETWORK, PROTOCOL_VERSION);
  stream >> headerA;
  BOOST_CHECK(headerA.GetHash() == uint256S("00000000a2daf649e9d0c0453caba957f02b441bae82117627a723f399818c9e"));
  BOOST_CHECK(headerA.hashPrevBlock == headerG.GetHash());

  stream = CDataStream(
      ParseHex("000000209e8c8199f323a727761182ae1b442bf057a9ab3c45c0d0e949f6daa200000000f410091944a3c14563ff204c7731cb1"
               "fbc15cc748ec96f847a4cfbdd0372c2c1b68ff65cffff001d45270cfe0101000000010000000000000000000000000000000000"
               "000000000000000000000000000000ffffffff21010200fed1fe0c00fed2d306000963676d696e6572343208010000000000000"
               "000ffffffff0100743ba40b0000001976a91480d057b26c1768ef56420a5c752ca7d3ed4a378288ac00000000"),
      SER_NETWORK, PROTOCOL_VERSION);
  stream >> headerAA;
  BOOST_CHECK(headerAA.GetHash() == uint256S("00000000cff2a1fe12fb1ce2c85da0acb2a4a135935fe0a6df26626980770c8f"));
  BOOST_CHECK(headerAA.hashPrevBlock == headerA.GetHash());

  stream = CDataStream(
      ParseHex("000000208f0c7780696226dfa6e05f9335a1a4b2aca05dc8e21cfb12fea1f2cf00000000b32819c89b3ad00d447543a10c52cb0"
               "4032b365711992cd69369c79e4fcec701ba8ff65cffff001d18f22dbf0101000000010000000000000000000000000000000000"
               "000000000000000000000000000000ffffffff21010300fed4fe0c00fe4b920e000963676d696e6572343208000000000000000"
               "000ffffffff0100743ba40b0000001976a91480d057b26c1768ef56420a5c752ca7d3ed4a378288ac00000000"),
      SER_NETWORK, PROTOCOL_VERSION);
  stream >> headerB;
  BOOST_CHECK(headerB.hashPrevBlock == headerAA.GetHash());

  stream = CDataStream(
      ParseHex("00000020d4b1d0f18751ff1f3d34780e972a2e7adcefef5cfa6658bc78b887430000000008687b9c35cab2e933bfb406dfec5df"
               "9a39fb4ac8fbd862087538a4e85dd68b4bb8ff65cffff001d63e9f9f80101000000010000000000000000000000000000000000"
               "000000000000000000000000000000ffffffff21010400fed6fe0c00fea6fd06000963676d696e6572343208010000000000000"
               "000ffffffff0100743ba40b0000001976a91480d057b26c1768ef56420a5c752ca7d3ed4a378288ac00000000"),
      SER_NETWORK, PROTOCOL_VERSION);
  stream >> headerAB;
  BOOST_CHECK(headerAB.hashPrevBlock == headerB.GetHash());

  // Headers A should be accepted
  BOOST_CHECK(ProcessNewBlockHeaders(config, {headerA}, state, &pindex, &invalid));
  BOOST_CHECK(state.IsValid());
  BOOST_CHECK(pindex != nullptr);
  pindex = nullptr;
  BOOST_CHECK(invalid.IsNull());

  // Header AA should be rejected - checkpoint mismatch
  BOOST_CHECK(!ProcessNewBlockHeaders(config, {headerAA}, state, &pindex, &invalid));
  BOOST_CHECK(state.IsInvalid());
  BOOST_CHECK(state.GetRejectCode() == REJECT_CHECKPOINT);
  BOOST_CHECK(state.GetRejectReason() == "checkpoint mismatch");
  BOOST_CHECK(pindex == nullptr);

  // Sanity check to ensure header was not saved in memory
  {
    LOCK(cs_main);
    BOOST_CHECK(LookupBlockIndex(headerB.GetHash()) == nullptr);
  }

  // Header AB should be rejected since checkpoint mismatch means prev block not stored
  BOOST_CHECK(!ProcessNewBlockHeaders(config, {headerAB}, state, &pindex, &invalid));
  BOOST_CHECK(state.IsInvalid());
  BOOST_CHECK(state.GetRejectCode() == 0);
  BOOST_CHECK(state.GetRejectReason() == "prev-blk-not-found");
  BOOST_CHECK(pindex == nullptr);
  BOOST_CHECK(invalid.GetHash() == headerAB.GetHash());

  // Sanity check to ensure header was not saved in memory
  {
    LOCK(cs_main);
    BOOST_CHECK(LookupBlockIndex(headerAB.GetHash()) == nullptr);
  }
}

// BOOST_AUTO_TEST_SUITE_END()
