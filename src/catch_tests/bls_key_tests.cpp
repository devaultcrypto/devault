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
testpriv:zq22jyaar8jevm7jc3qtszpq9htps386tssms7rtpgsdq6f7kylm2zxz2raup 2020-07-29T02:03:02Z change=1 # addr=blstest:dpwcnj2udrmkrrwz9jnr3zmrqj4p2tf0x5723yqwlq,hdkeypath=m/44'/1'/1'/0/0
testpriv:zqudn85w5ldwk2katapm0gamw4se5ga2a2a2sekcyaj8u8svrdkjxklhse8k7 2020-07-29T02:03:02Z change=1 # addr=blstest:dql46rkc4lu9zmsallyy92urchjzelkz3s829w75cp,hdkeypath=m/44'/1'/1'/0/1
testpriv:zp5jmcx4mzguywgkcew77a6safl6rehxgtfxkflvnlcvn52s3etfy83y3890u 2020-07-29T02:03:02Z change=1 # addr=blstest:dp4daszrhql3zhyuksatlqt5yhn9l6yvfvl0kwghf0,hdkeypath=m/44'/1'/1'/0/2
testpriv:zp3j3lelhqc68h844jxdlqxjtqvwaswh7mvk42gfx805z6awjwsxxa59hd7v4 2020-07-29T02:03:02Z change=1 # addr=blstest:dq7elxaqtj7m8gys7jg9x6styc50mg9r2vy7j4e4c9,hdkeypath=m/44'/1'/1'/0/3
*/

// 32/33 from HD chain
static const std::string strSecret1 = "testpriv:zq22jyaar8jevm7jc3qtszpq9htps386tssms7rtpgsdq6f7kylm2zxz2raup";
static const std::string strSecret2 = "testpriv:zqudn85w5ldwk2katapm0gamw4se5ga2a2a2sekcyaj8u8svrdkjxklhse8k7";
static const std::string strSecret3 = "testpriv:zp5jmcx4mzguywgkcew77a6safl6rehxgtfxkflvnlcvn52s3etfy83y3890u";

static const std::string addr1 = "blstest:dpwcnj2udrmkrrwz9jnr3zmrqj4p2tf0x5723yqwlq";
static const std::string addr2 = "blstest:dql46rkc4lu9zmsallyy92urchjzelkz3s829w75cp";
static const std::string addr3 = "blstest:dp4daszrhql3zhyuksatlqt5yhn9l6yvfvl0kwghf0";

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
