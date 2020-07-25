// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>

#include <bls/bls_functions.h>

#include <catch_tests/test_bitcoin.h>
#include <chainparams.h>
#include <config.h>
#include <dstencode.h>
#include <iomanip>
#include <script/script.h>
#include <string>
#include <uint256.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <vector>

#include "catch_unit.h"

/*
testpriv:zqcruq3qe2r75w87xgm253e4jye3smq28uutz6npd62zp04yxkzzq5hmt3fpn 2020-07-25T06:11:35Z reserve=1 # 
addr=blstest:dzyk0nk9xl2cufd958rgdsx4uzzjwpql5quvnkxma6,hdkeypath=m/44'/1'/1'/0/0
testpriv:zqf4rtsl344wcdnd9l6ke8m9sts7at34pyqh36jj3fg6pezw4yt6zvce6gex2 2020-07-25T06:12:29Z reserve=1 # 
addr=blstest:drj0mmla4raxh2eqlxz3tz7qwqk0xgau55jyjrq2kf,hdkeypath=m/44'/1'/1'/0/1
testpriv:zqkwdedurxrs6gu3gme7500qtc9af3rulaxdjg7as97x05vvsexgshlfnpee9 2020-07-25T06:12:39Z reserve=1 # 
addr=blstest:dp3etlfddmzutjufm6c7x8ayg7nc9v05vct2pxpl2n,hdkeypath=m/44'/1'/1'/0/2
*/

// 32/33 from HD chain
static const std::string strSecret1 = "testpriv:zqcruq3qe2r75w87xgm253e4jye3smq28uutz6npd62zp04yxkzzq5hmt3fpn";
static const std::string strSecret2 = "testpriv:zqf4rtsl344wcdnd9l6ke8m9sts7at34pyqh36jj3fg6pezw4yt6zvce6gex2";
static const std::string strSecret3 = "testpriv:zqkwdedurxrs6gu3gme7500qtc9af3rulaxdjg7as97x05vvsexgshlfnpee9";

static const std::string addr1 = "blstest:dzyk0nk9xl2cufd958rgdsx4uzzjwpql5quvnkxma6";
static const std::string addr2 = "blstest:drj0mmla4raxh2eqlxz3tz7qwqk0xgau55jyjrq2kf";
static const std::string addr3 = "blstest:dp3etlfddmzutjufm6c7x8ayg7nc9v05vct2pxpl2n";

static const std::string strAddressBad = "=blstest:rpwqz8mgp97ak2a369q7t0hczvpt05upa5alt22rj8";

// BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)


TEST_CASE("bls_key_test1") {
  BasicTestingSetup setup(CBaseChainParams::TESTNET);

  // Secret Keys give Keys always compressed
  CKey key1 = DecodeSecret(strSecret1);
  CKey key2 = DecodeSecret(strSecret2);
  CKey key3 = DecodeSecret(strSecret3);

  CPubKey pubkey1 = key1.GetPubKeyForBLS();
  CPubKey pubkey2 = key2.GetPubKeyForBLS();
  CPubKey pubkey3 = key3.GetPubKeyForBLS();

  BOOST_CHECK(key1.VerifyPubKey(pubkey1));
  BOOST_CHECK(!key1.VerifyPubKey(pubkey3));
  BOOST_CHECK(!key1.VerifyPubKey(pubkey2));

  BOOST_CHECK(!key3.VerifyPubKey(pubkey1));
  BOOST_CHECK(key3.VerifyPubKey(pubkey3));
  BOOST_CHECK(!key3.VerifyPubKey(pubkey2));

  BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
  BOOST_CHECK(!key2.VerifyPubKey(pubkey3));
  BOOST_CHECK(key2.VerifyPubKey(pubkey2));
  BOOST_CHECK(!key2.VerifyPubKey(pubkey3));

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();

  BOOST_CHECK(DecodeDestination(addr1, chainParams) == CTxDestination(pubkey1.GetBLSKeyID()));
  BOOST_CHECK(DecodeDestination(addr2, chainParams) == CTxDestination(pubkey2.GetBLSKeyID()));
  BOOST_CHECK(DecodeDestination(addr3, chainParams) == CTxDestination(pubkey3.GetBLSKeyID()));

  for (int n = 0; n < 16; n++) {
    std::string strMsg = strprintf("Very secret message %i: 11", n);
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

    // BLS signatures
    CPubKey bpubkey1 = key1.GetPubKeyForBLS();
    CPubKey bpubkey2 = key2.GetPubKeyForBLS();
    CPubKey bpubkey3 = key3.GetPubKeyForBLS();

    std::vector<uint8_t> ssign1, ssign2, ssign3;

    BOOST_CHECK(key1.SignBLS(hashMsg, ssign1));
    BOOST_CHECK(key2.SignBLS(hashMsg, ssign2));
    BOOST_CHECK(key3.SignBLS(hashMsg, ssign3));

    BOOST_CHECK(bpubkey1.VerifyBLS(hashMsg, ssign1));
    BOOST_CHECK(!bpubkey1.VerifyBLS(hashMsg, ssign2));
    BOOST_CHECK(!bpubkey1.VerifyBLS(hashMsg, ssign3));

    BOOST_CHECK(!bpubkey2.VerifyBLS(hashMsg, ssign1));
    BOOST_CHECK(bpubkey2.VerifyBLS(hashMsg, ssign2));
    BOOST_CHECK(!bpubkey2.VerifyBLS(hashMsg, ssign3));

    BOOST_CHECK(!bpubkey3.VerifyBLS(hashMsg, ssign1));
    BOOST_CHECK(!bpubkey3.VerifyBLS(hashMsg, ssign2));
    BOOST_CHECK(bpubkey3.VerifyBLS(hashMsg, ssign3));
  }

  // test deterministic signing expected values

  std::vector<uint8_t> detsig, detsigc;
  std::string strMsg = "Very deterministic message";
  uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

  // BLS
  BOOST_CHECK(key1.SignBLS(hashMsg, detsig));
  BOOST_CHECK(key3.SignBLS(hashMsg, detsigc));

  BOOST_CHECK(key2.SignBLS(hashMsg, detsig));
}


static const std::string Public1 = addr1;
static const std::string Public2 = addr2;

TEST_CASE("bls agg sig test") {
  BasicTestingSetup setup(CBaseChainParams::TESTNET);

  // Secret Keys give Keys always compressed
  CKey key1 = DecodeSecret(strSecret1);
  CKey key2 = DecodeSecret(strSecret2);

  //std::cout << "Key1 Hex = " << HexStr(key1.begin(), key1.end()) << "\n";
  //std::cout << "Key2 Hex = " << HexStr(key2.begin(), key2.end()) << "\n";

  CPubKey pubkey1 = key1.GetPubKeyForBLS();
  CPubKey pubkey2 = key2.GetPubKeyForBLS();

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  
  
  BOOST_CHECK(DecodeDestination(Public1, chainParams) == CTxDestination(pubkey1.GetBLSKeyID()));
  BOOST_CHECK(DecodeDestination(Public2, chainParams) == CTxDestination(pubkey2.GetBLSKeyID()));

  for (int n = 0; n < 1; n++) { // 1 for NOW

    std::vector<std::vector<uint8_t>> pubkeys;
    std::vector<std::vector<uint8_t>> vecsigs;

    std::string strMsg = "Very secret message"; // strprintf("Very secret message %i: 11", n);
    std::vector<uint8_t> message(strMsg.begin(), strMsg.end());
    std::vector<std::vector<uint8_t> > messages;

    std::vector<uint8_t> sign1;
    std::vector<uint8_t> sign2;

    BOOST_CHECK(bls::SignBLS(key1, message, sign1));

    vecsigs.push_back(sign1);
    messages.push_back(message);
    pubkeys.push_back(ToByteVector(pubkey1));

    // std::cout << "key1 = " << HexStr(ToByteVector(pubkey1)) << "\n";
    // std::cout << "sig1 = " << HexStr(sign1) << "\n";

    BOOST_CHECK(bls::SignBLS(key2, message, sign2));

    // std::cout << "key2 = " << HexStr(ToByteVector(pubkey2)) << "\n";
    // std::cout << "sig2 = " << HexStr(sign2) << "\n";

    vecsigs.push_back(sign2);
    messages.push_back(message);
    pubkeys.push_back(ToByteVector(pubkey2));

    //BOOST_CHECK(pubkey1.VerifyBLS(hashMsg, sign1));
    //BOOST_CHECK(!pubkey1.VerifyBLS(hashMsg, sign2));

    //BOOST_CHECK(!bpubkey2.VerifyBLS(hashMsg, sign1));
    //BOOST_CHECK(bpubkey2.VerifyBLS(hashMsg, sign2));

    // Verify Aggregate Signature here
    std::vector<uint8_t> aggKeys = bls::AggregatePubKeys(pubkeys); // breaks for even 1 key

    auto aggSig = bls::Aggregate(vecsigs);

    bool ok2 = bls::VerifySigForMessages(messages, aggSig, pubkeys);
    BOOST_CHECK(ok2);

  }
}

// BOOST_AUTO_TEST_SUITE_END()
