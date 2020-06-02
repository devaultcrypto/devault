// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>

#include <bls/bls_functions.h>
#include <bls/signature.hpp>

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
testpriv:zzjeqe4uk6jrk7m9xgv3uvsn4pl8uz6hucm8vgsvhcmsfhzaxmm35d3pjszfc 2019-11-26T03:04:09Z reserve=1 #
    addr=blstest:rrw43xma6fqg76s2ru8c433sghd3kynypqay89cwn3,hdkeypath=m/44'/1'/0'/0/32
testpriv:zzxr3dnaadc733gtyeq04tl08hspp52xrmkwa6g3997yhw27zddzk06ee8m4k 2019-11-26T03:04:09Z reserve=1 #
    addr=blstest:rqw2h6gw5j9as7n53u6zt350057dq8js2cpvvsz00p,hdkeypath=m/44'/1'/0'/0/33
testpriv:zq96llmh3qm29a0ufmjglh6r3jh8fuqhg5ucu5g2lygnzpmmx0w9jnwvwnenh 2019-11-26T03:04:09Z reserve=1 #
    addr=blstest:rz099tn2lv5jg895gnm6j26ghf5h9ux9fgfdl49w3d,hdkeypath=m/44'/1'/0'/0/34
*/

// 32/33 from HD chain
static const std::string strSecret1 = "testpriv:zzjeqe4uk6jrk7m9xgv3uvsn4pl8uz6hucm8vgsvhcmsfhzaxmm35d3pjszfc";
static const std::string strSecret2 = "testpriv:zzxr3dnaadc733gtyeq04tl08hspp52xrmkwa6g3997yhw27zddzk06ee8m4k";
static const std::string strSecret3 = "testpriv:zq96llmh3qm29a0ufmjglh6r3jh8fuqhg5ucu5g2lygnzpmmx0w9jnwvwnenh";

//static const std::string addr1 = "blstest:dp8xwhskqe7n03rxkxtee5nulf3gfaeg2v3vqt27m4";
static const std::string addr1 = "blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun";
//static const std::string addr2 = "blstest:dp8nw8hsxhaza5y50j5q6j3unauss28f9ytmdnss38";
static const std::string addr2 = "blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr";
//static const std::string addr3 = "blstest:dp9jzxnyd8n2v2jda9um7sq2wse208sgpyv250vgyx";
static const std::string addr3 = "blstest:dz099tn2lv5jg895gnm6j26ghf5h9ux9fgujaaxu70";

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


// 32/33 from HD chain
//static const std::string Secret1 = "testpriv:zp8fm0ydzhdy83g25m2udmg3xwzd3rrwkfwvr6u6fcaahhvcn737ukkjhwcyn";
//static const std::string Secret2 = "testpriv:zp9jl2xe822rtf898rpuda4xj23xhtnglnfsnmhflrmln8lvn5jlc94grsrwc";

static const std::string Public1 = addr1;//"blstest:rzqz6nsxn9l2aqx8mgml3vcr3750vxcpdc46h4d0ax";
static const std::string Public2 = addr2;//"blstest:rrteymuq2vp8y76z0ludfz9j7ftgva5ensgnm4jy9f";

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

    bls::PublicKey Pk = bls::PublicKey::FromBytes(aggKeys.data());
    auto aggSig = bls::Aggregate(vecsigs);

    bool ok2 = bls::VerifySigForMessages(messages, aggSig, pubkeys);
    BOOST_CHECK(ok2);

  }
}

// BOOST_AUTO_TEST_SUITE_END()
