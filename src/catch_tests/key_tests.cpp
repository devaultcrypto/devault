// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>

#include <cashaddrenc.h>
#include <chainparams.h>
#include <config.h>
#include <dstencode.h>
#include <script/script.h>
#include <catch_tests/test_bitcoin.h>
#include <uint256.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <iomanip>
#include <string>
#include <vector>

#include "catch_unit.h"

/*
testpriv:zzxr3dnaadc733gtyeq04tl08hspp52xrmkwa6g3997yhw27zddzk06ee8m4k
# addr=dvtest:qp8nw8hsxhaza5y50j5q6j3unauss28f9y3l5yx8h0,hdkeypath=m/44'/1'/0'/0/33
testpriv:zq96llmh3qm29a0ufmjglh6r3jh8fuqhg5ucu5g2lygnzpmmx0w9jnwvwnenh
# addr=dvtest:qp9jzxnyd8n2v2jda9um7sq2wse208sgpykwdc6lzw,hdkeypath=m/44'/1'/0'/0/34
testpriv:zzsug4madaqkz8demqh6u7paslstyafwmam2tzuasszl5mlqfq0cjhzpd70rq
# addr=dvtest:qrnekung3wsflx9nklq0rnjlnkhl4j9p4544pl0tgg,hdkeypath=m/44'/1'/0'/0/35
testpriv:zp2xqfjlsf46x9ter9r3cnvvj4wena9z5r7p60zwnze3au3sqqypcsn3yt5yw
# addr=dvtest:qqklsqvpfyezzqtzvh2rd9eknmlwksph0gfdwdwsx4,hdkeypath=m/44'/1'/0'/0/36
*/

// 32/33 from HD chain
static const std::string strSecret1 = "testpriv:zzxr3dnaadc733gtyeq04tl08hspp52xrmkwa6g3997yhw27zddzk06ee8m4k";
static const std::string strSecret2 = "testpriv:zq96llmh3qm29a0ufmjglh6r3jh8fuqhg5ucu5g2lygnzpmmx0w9jnwvwnenh";
static const std::string strSecret3 = "testpriv:zzsug4madaqkz8demqh6u7paslstyafwmam2tzuasszl5mlqfq0cjhzpd70rq";

static const std::string addr1 = "dvtest:qp8nw8hsxhaza5y50j5q6j3unauss28f9y3l5yx8h0";
static const std::string addr2 = "dvtest:qp9jzxnyd8n2v2jda9um7sq2wse208sgpykwdc6lzw";
static const std::string addr3 = "dvtest:qrnekung3wsflx9nklq0rnjlnkhl4j9p4544pl0tgg";

static const std::string strAddressBad = "dvtest:qqklsqvpfyezzqtzvh2rd9eknmlwksph0gfdwdwsx4";

// get r value produced by ECDSA signing algorithm
// (assumes ECDSA r is encoded in the canonical manner)
std::vector<uint8_t> get_r_ECDSA(std::vector<uint8_t> sigECDSA) {
  std::vector<uint8_t> ret(32, 0);
  assert(sigECDSA[2] == 2);
  int rlen = sigECDSA[3];
  assert(rlen <= 33);
  assert(sigECDSA[4 + rlen] == 2);
  if (rlen == 33) {
    assert(sigECDSA[4] == 0);
    std::copy(sigECDSA.begin() + 5, sigECDSA.begin() + 37, ret.begin());
  } else {
    std::copy(sigECDSA.begin() + 4, sigECDSA.begin() + (4 + rlen), ret.begin() + (32 - rlen));
  }
  return ret;
}

// BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

TEST_CASE("internal_test") {
  BasicTestingSetup setup;
  // test get_r_ECDSA (defined above) to make sure it's working properly
  BOOST_CHECK(get_r_ECDSA(ParseHex("3045022100c6ab5f8acfccc114da39dd5ad0b1ef4d39df6a721e824c22e00b7bc7944a1f7802206ff23"
                                   "df3802e241ee234a8b66c40c82e56a6cc37f9b50463111c9f9229b8f3b3")) ==
              ParseHex("c6ab5f8acfccc114da39dd5ad0b1ef4d39df6a721e824c22e00b7bc7944a1f78"));

  BOOST_CHECK(get_r_ECDSA(ParseHex("3045022046ab5f8acfccc114da39dd5ad0b1ef4d39df6a721e824c22e00b7bc7944a1f7802206ff23df"
                                   "3802e241ee234a8b66c40c82e56a6cc37f9b50463111c9f9229b8f3b3")) ==
              ParseHex("46ab5f8acfccc114da39dd5ad0b1ef4d39df6a721e824c22e00b7bc7944a1f78"));
}

TEST_CASE("key_test1") {
  BasicTestingSetup setup(CBaseChainParams::TESTNET);

  // Secret Keys give Keys always compressed
  CKey key1 = DecodeSecret(strSecret1);
  CKey key2 = DecodeSecret(strSecret2);
  CKey key3 = DecodeSecret(strSecret3);

  CPubKey pubkey1 = key1.GetPubKey();
  CPubKey pubkey2 = key2.GetPubKey();
  CPubKey pubkey3 = key3.GetPubKey();

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

  BOOST_CHECK(DecodeDestination(addr1, chainParams) == CTxDestination(pubkey1.GetID()));
  BOOST_CHECK(DecodeDestination(addr2, chainParams) == CTxDestination(pubkey2.GetID()));
  BOOST_CHECK(DecodeDestination(addr3, chainParams) == CTxDestination(pubkey3.GetID()));

  for (int n = 0; n < 16; n++) {
    std::string strMsg = strprintf("Very secret message %i: 11", n);
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

    // normal ECDSA signatures

    std::vector<uint8_t> sign1, sign2, sign3;

    BOOST_CHECK(key1.SignECDSA(hashMsg, sign1));
    BOOST_CHECK(key2.SignECDSA(hashMsg, sign2));
    BOOST_CHECK(key3.SignECDSA(hashMsg, sign3));

    BOOST_CHECK(pubkey1.VerifyECDSA(hashMsg, sign1));
    BOOST_CHECK(pubkey2.VerifyECDSA(hashMsg, sign2));
    BOOST_CHECK(pubkey3.VerifyECDSA(hashMsg, sign3));

    BOOST_CHECK(!pubkey1.VerifyECDSA(hashMsg, sign2));
    BOOST_CHECK(!pubkey1.VerifyECDSA(hashMsg, sign3));

    BOOST_CHECK(!pubkey2.VerifyECDSA(hashMsg, sign1));
    BOOST_CHECK(!pubkey2.VerifyECDSA(hashMsg, sign3));

    BOOST_CHECK(!pubkey3.VerifyECDSA(hashMsg, sign1));
    BOOST_CHECK(!pubkey3.VerifyECDSA(hashMsg, sign2));

    // compact ECDSA signatures (with key recovery)

    std::vector<uint8_t> csign1, csign2, csign3;

    BOOST_CHECK(key1.SignCompact(hashMsg, csign1));
    BOOST_CHECK(key2.SignCompact(hashMsg, csign2));
    BOOST_CHECK(key3.SignCompact(hashMsg, csign3));

    CPubKey rkey1, rkey2, rkey3;

    BOOST_CHECK(rkey1.RecoverCompact(hashMsg, csign1));
    BOOST_CHECK(rkey2.RecoverCompact(hashMsg, csign2));
    BOOST_CHECK(rkey3.RecoverCompact(hashMsg, csign3));

    BOOST_CHECK(rkey1 == pubkey1);
    BOOST_CHECK(rkey2 == pubkey2);
    BOOST_CHECK(rkey3 == pubkey3);

    // Schnorr signatures

    std::vector<uint8_t> ssign1, ssign2, ssign3;

    BOOST_CHECK(key1.SignSchnorr(hashMsg, ssign1));
    BOOST_CHECK(key2.SignSchnorr(hashMsg, ssign2));
    BOOST_CHECK(key3.SignSchnorr(hashMsg, ssign3));

    BOOST_CHECK(pubkey1.VerifySchnorr(hashMsg, ssign1));
    BOOST_CHECK(!pubkey1.VerifySchnorr(hashMsg, ssign2));
    BOOST_CHECK(!pubkey1.VerifySchnorr(hashMsg, ssign3));

    BOOST_CHECK(!pubkey2.VerifySchnorr(hashMsg, ssign1));
    BOOST_CHECK(pubkey2.VerifySchnorr(hashMsg, ssign2));
    BOOST_CHECK(!pubkey2.VerifySchnorr(hashMsg, ssign3));

    BOOST_CHECK(!pubkey3.VerifySchnorr(hashMsg, ssign1));
    BOOST_CHECK(!pubkey3.VerifySchnorr(hashMsg, ssign2));
    BOOST_CHECK(pubkey3.VerifySchnorr(hashMsg, ssign3));

    // Extract r value from ECDSA and Schnorr. Make sure they are
    // distinct (nonce reuse would be dangerous and can leak private key).
    std::vector<uint8_t> rE1 = get_r_ECDSA(sign1);
    BOOST_CHECK(ssign1.size() == 64);
    std::vector<uint8_t> rS1(ssign1.begin(), ssign1.begin() + 32);
    BOOST_CHECK(rE1.size() == 32);
    BOOST_CHECK(rS1.size() == 32);
    BOOST_CHECK(rE1 != rS1);

    std::vector<uint8_t> rE2 = get_r_ECDSA(sign2);
    BOOST_CHECK(ssign2.size() == 64);
    std::vector<uint8_t> rS2(ssign2.begin(), ssign2.begin() + 32);
    BOOST_CHECK(rE2.size() == 32);
    BOOST_CHECK(rS2.size() == 32);
    BOOST_CHECK(rE2 != rS2);
  }

  // test deterministic signing expected values

  std::vector<uint8_t> detsig, detsigc;
  std::string strMsg = "Very deterministic message";
  uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
  // ECDSA
  BOOST_CHECK(key1.SignECDSA(hashMsg, detsig));
  BOOST_CHECK(key3.SignECDSA(hashMsg, detsigc));
  BOOST_CHECK(detsig != detsigc);

  BOOST_CHECK(key2.SignECDSA(hashMsg, detsig));
  // Compact
  BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
  BOOST_CHECK(key3.SignCompact(hashMsg, detsigc));

  BOOST_CHECK(key2.SignCompact(hashMsg, detsig));

  // Schnorr
  BOOST_CHECK(key1.SignSchnorr(hashMsg, detsig));
  BOOST_CHECK(key3.SignSchnorr(hashMsg, detsigc));

  BOOST_CHECK(key2.SignSchnorr(hashMsg, detsig));
}

// BOOST_AUTO_TEST_SUITE_END()
