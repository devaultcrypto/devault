// Copyright (c) 2020 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls_functions.h>
#include <catch_tests/test_bitcoin.h>
#include <chainparams.h>
#include <checksigs.h>
#include <combine_transactions.h>
#include <config.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <dstencode.h>
#include <primitives/create_bls_transaction.h>
#include <validation.h>

#include <map>
#include <string>

#include "catch_unit.h"

void make_public_input_keys(int vals, std::vector<CKey> &input_keys, std::vector<bool> &p2pub) {

  input_keys.clear();
  p2pub.clear();
  for (int i = 0; i < vals; i++) {
    CKey k;
    k.MakeNewKey();
    input_keys.push_back(k);
    p2pub.push_back(true);
  }
}

// We dont check input vs output values, just signatures
CMutableTransaction SetupBLSTx(const CChainParams &chainParams, const std::vector<CKey> &input_keys,
                               const std::vector<bool> &p2pub, const std::vector<std::string> &addresses,
                               const std::vector<Amount> &amounts) {
  CMutableTransaction t0;

  assert(input_keys.size() == p2pub.size());
  assert(addresses.size() == amounts.size());

  t0.vin.clear();
  FastRandomContext rng;
  for (size_t i = 0; i < input_keys.size(); i++) {
    // value doesnt matter so all are the same
    TxId prevId = TxId(rng.rand256());
    COutPoint outpoint(prevId, i);
    CScript scriptPubKey;
    if (!p2pub[i]) {
      scriptPubKey << OP_DUP << OP_BLSKEYHASH
                   << ToByteVector(input_keys[i].GetPubKeyForBLS().GetKeyID())
                   << OP_EQUALVERIFY << OP_CHECKSIG;
    } else {
      scriptPubKey << ToByteVector(input_keys[i].GetPubKeyForBLS()) << OP_CHECKSIG;
    }
    t0.vin.emplace_back(outpoint, scriptPubKey, std::numeric_limits<uint32_t>::max() - 1);
  }

  t0.vout.resize(addresses.size());
  for (size_t i = 0; i < addresses.size(); i++) {
    CScript scriptPubKey = GetScriptForDestination(DecodeDestination(addresses[i], chainParams));
    
    if (IsValidDestinationString(addresses[i], chainParams)) {
      // Create outputs
      CTxOut vout;
      vout.nValue = amounts[i];
      vout.scriptPubKey = scriptPubKey;
      t0.vout[i] = vout;
    } else {
      throw std::runtime_error("Problem setting vout for address value\n");
    }
  }

  //  std::cout << "t0 hash before CreatePrivateTxWithSig = " << t0.GetHash().ToString() << "\n";
  auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
  if (strFail) {
    std::cout << "FAIL : " << strFail.value() << "\n";
    BOOST_ERROR_MESSAGE("Exception creating transactions in SetupBLSTx\n");
  }
  return t0;
}

std::vector<std::string> txes = {
    "0200000001ac6595023c5902d73e7cca76a0d9644a68f9f6e3665bfe19dafd50ab8b23a35f00000000f73098ac0231eba71e72959646d557"
    "bb9f7797d8b56abf4e5e94051f21c30d7b87f5aee994a27b7baddec6cf0921fa13c3243086515c06b2164302a1df788f6bfedeaa841c64dc"
    "03b6414e17f4a8d45bc3d3a7b565b1df1a8655225b499d129e08645430874cb2306f86b4fd72e2bbcf2c4df1c3bf857baa7fb9afcd93cdee"
    "0b2f7e6bc16c6047afa3d526863ca5168e0492f16e4c6190057c623d2a72af14e617eadd4bb547be67c82c71eb75ce12d7353c0ee65870a0"
    "6f21f362b88dbee63835390810c17f16a914c553641cd46623d65a0e54e946ec3e2013683040efd814101dd742c114273f3863515726d482"
    "5ed1d7f504789d01acfeffffff026018f9f4020000001976bd14ec4b6955021a75a50b82e74ec1226dd9829ecb5288ac001b23dd02000000"
    "1976bd147274fe2fc3c8386dce7fd06233fbf361ee787d0288ac3b070000",

    "020000000167efe96aff6c0b5f3940574be28d8817d61f70bb24f715061b26ecf1b27a24cc00000000f73001c7a7a8994ee92a2a8c8d1d7b0e"
    "25be5b06ae9ec119c3c596ef6d8d8cdea4f1f6b7eb117345f53dbdc2f8eefa30e4f930998dce5248a4073608d0c7b74f4361fa11965a29d768"
    "f276372cf3e9a8543eb265a04e8de518032fab46d402cb3c173b3019a9086955689215ffc4949c3582e81c660a95d244083cd906d356b34dd5"
    "07fe7376f017bc4a578d546431b3261d46de4c6197d972dde93383185fc0f0e24bc53affc0e0dc8c4401023f3d3408b522e9a1abf5697f7714"
    "28e604c1debc6cc4914bd714e5b75182bf6d17401aaea72b972080f90c39962cdcf46b3e2d8af598dade8a1b2f0f22d5891d27d10abbc8db4c"
    "49d101acfeffffff0200e40b54020000001976bd14c68dd538e76a7addc6d149a3406a6f2de250421c88ac00a52691070000001976bd14ee18"
    "ed070c0532a4a526c3f97fd6c3cbb15eb18788acff080000"};

TEST_CASE("basic_transaction_check") {

  BasicTestingSetup setup("test");
  std::vector<CMutableTransaction> txs(txes.size());

  for (unsigned int idx = 0; idx < txes.size(); idx++) {
    DecodeHexTx(txs[idx], txes[idx]);
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(txs[idx]), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  }
}

TEST_CASE("Check bls transaction and signatures, 1 input, 1 output, first with public key input, then pkh") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;

  make_public_input_keys(1, input_keys, p2pub);

  for (int i=0;i<2;i++) {
    p2pub[0] = i;

    std::vector<std::string> out_addresses = {"blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr"};
    std::vector<Amount> amounts = {10 * COIN};

    const Config &config = GetConfig();
    const CChainParams &chainParams = config.GetChainParams();
    CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);
    CTransaction t_ref_in(t0);

    try {
      auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
      if (strFail) { BOOST_ERROR_MESSAGE("Failure creating transaction in 1 input/1 output case\n"); }
      CValidationState state;
      BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                          "Simple deserialized transaction should be valid.");
    } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transaction in 1 input/1 output case\n"); }
    
    CTransaction t_ref_out(t0);
    bool check = CheckPrivateSigs(t_ref_out);
    BOOST_CHECK(check);
    
    for (const auto &v : t0.vin)   BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
    for (const auto &v : t0.vout)  BOOST_CHECK(v.IsBLS());
    // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
    
    // Mess with an output value and make sure check fails
    {
      t0.vout[0].nValue = COIN;
      CTransaction tmp(t0);
      bool check_bad_output_val = CheckPrivateSigs(tmp);
      BOOST_CHECK(!check_bad_output_val);
    }
    
    // Mess with an output address and make sure check fails
    {
      // Restart with good Transaction
      CMutableTransaction t1(t_ref_out);
      // Change Output Address
      t1.vout[0].scriptPubKey =
        GetScriptForDestination(DecodeDestination("blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun", chainParams));
      
      // Should also fail
      CTransaction tmp(t1);
      bool check_bad_output_address = CheckPrivateSigs(tmp);
      BOOST_CHECK(!check_bad_output_address);
    }

    // Change 1st input script - fail
    {
      CMutableTransaction t1(t_ref_out);
      t1.vin[0].scriptSig = CScript();
      CTransaction tmp(t1);
      bool check_bad_input_script = CheckPrivateSigs(tmp);
      BOOST_CHECK(!check_bad_input_script);
    }
  }
}


TEST_CASE("Check bls transaction and signatures, 2 input, 2 outputs, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(2, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
                                            "blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr"};

  std::vector<Amount> amounts = {10 * COIN, 20 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  for (const auto &v : t0.vin)     BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout)    BOOST_CHECK(v.IsBLS());
  // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  // Use different input script for 1st input
  {
    CMutableTransaction t1(t0);
    t1.vin[0].scriptSig = CScript();
    CTransaction tmp(t1);
    bool check_bad_input_script = CheckPrivateSigs(tmp);
    BOOST_CHECK(!check_bad_input_script);
  }
}

TEST_CASE("Check bls transaction and signatures, 3 input, 3 outputs, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(3, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
                                            "blstest:dpcds8e73c2ym43r6qgl37t9hvz26jj665urhj9zy7",
                                            "blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr"};

  std::vector<Amount> amounts = {10 * COIN, 15 * COIN, 20 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)    BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout)   BOOST_CHECK(v.IsBLS());
  // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
}

TEST_CASE("Check bls transaction and signatures, 3 input, 2 outputs, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(3, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
                                            "blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr"};

  std::vector<Amount> amounts = {10 * COIN, 20 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)     BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout)    BOOST_CHECK(v.IsBLS());
  // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
}

TEST_CASE("Check bls transaction and signatures, 3 input, 1 output, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(3, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun"};
  std::vector<Amount> amounts = {10 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)    BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout)   BOOST_CHECK(v.IsBLS());
  // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
}

TEST_CASE("Check bls transaction and signatures, 4 input, 1 output, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(4, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun"};
  std::vector<Amount> amounts = {10 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)    BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout)   BOOST_CHECK(v.IsBLS());
  // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
}

TEST_CASE("Check bls transaction and signatures, 5 input, 1 output, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(5, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun"};
  std::vector<Amount> amounts = {10 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)  BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout) BOOST_CHECK(v.IsBLS());
}

TEST_CASE("Check bls transaction and signatures, 6 input, 1 output, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;
  make_public_input_keys(6, input_keys, p2pub);

  std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun"};
  std::vector<Amount> amounts = {10 * COIN};

  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();
  CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

  try {
    auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
    if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }

  CTransaction rx(t0);
  bool check = CheckPrivateSigs(rx);
  BOOST_CHECK(check);

  for (const auto &v : t0.vin)  BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
  for (const auto &v : t0.vout) BOOST_CHECK(v.IsBLS());
}


TEST_CASE("Check bls transaction and signatures, 200-1 inputs, 2 outputs, inputs using public key") {
  BasicTestingSetup setup("test");
  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;

  std::vector<std::string> out_addresses = {
                                            "blstest:drteymuq2vp8y76z0ludfz9j7ftgva5ensavea3k2t",
                                            "blstest:dpcds8e73c2ym43r6qgl37t9hvz26jj665urhj9zy7"};
  std::vector<Amount> amounts;

  const int sizes[] = {501,401,301,201,101,40,20,10,4,3,2,1};
  
  for (int k=0;k<2;k++) {
    // Do for a variety of input sizes
    for (auto& i : sizes) {
      input_keys.clear();
      p2pub.clear();
      make_public_input_keys(i, input_keys, p2pub);

      if (k==1) for (int kk=0;kk<i;kk++) p2pub[kk] = false;
      
      amounts.clear();
      for (size_t j=0;j<out_addresses.size();j++) { amounts.push_back( int(j+1) * COIN ); }

      if (k == 0) {
        std::cout << "For " << input_keys.size() << " public key inputs with " << out_addresses.size() << " outputs, ";
      } else {
        std::cout << "For " << input_keys.size() << " PKH inputs with " << out_addresses.size() << " outputs, ";
      }
      const Config &config = GetConfig();
      const CChainParams &chainParams = config.GetChainParams();
      CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);
      
      try {
        auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
        if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
        CValidationState state;
        BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                            "Simple deserialized transaction should be valid.");
      } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
      
      CTransaction rx(t0);
      bool check = CheckPrivateSigs(rx);
      BOOST_CHECK(check);
      
      for (const auto &v : t0.vin)  BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
      for (const auto &v : t0.vout)  BOOST_CHECK(v.IsBLS());
      //std::cout << "last vin script size = " << t0.vin[input_keys.size() - 1].scriptSig.size() << " ";
      std::cout << "Transaction size = " << rx.GetTotalSize() << "\n";
    }
  }
}

TEST_CASE("Check bls transaction and signatures, 1 input, 1-16 outputs, inputs using public key") {
  BasicTestingSetup setup("test");

  std::vector<CKey> input_keys;
  std::vector<bool> p2pub;

  std::vector<std::string> out_addresses = {
      "blstest:drteymuq2vp8y76z0ludfz9j7ftgva5ensavea3k2t",  "blstest:dpcds8e73c2ym43r6qgl37t9hvz26jj665urhj9zy7",
      "blstest:dpkpw5f8kqedhf4yz2e34jjmvmhwlf9f0ycedfwss2",  "blstest:dqdypj6ehyjgs5yfssttrt7ucs4sjasvqgxyl0azwy",
      "blstest:dza0ygw8yh3a3fnl7jf8asqew529xkegmyamppmkas",  "blstest:dplta8wyhwcmtzxs9ckgnqlsddjcd3qmmqj0hx7yt8",
      "blstest:dzkzt808x9wdnn4xe6tvxga3z3yh8jhc5gwppgnen5",  "blstest:dz0l9wkgf8tc70xyq38dgj8t5fcx2ml9jv2n4exu4m",
      "blstest:drsh4vgs7a45hyp2yg64qt3tx6s2uldkgg7qv87467",  "blstest:dzu7vpfmaf5nm2v8729vl8r2ay80w4x6qurq44jex0",
      "blstest:dr5szjcyxr0g80qxzcxj538l59s3ce4svyj3lqad3v",  "blstest:drset9hk38h5a3l5tqq7kw54wsskct7gk5xjchg4an",
      "blstest:drecr8q88twummg7jqmstjhgrkwy0xmry5rslhxxl4",  "blstest:drfjz83pgwhrfuvt8mupacwjcs2clpfa9usx9t9hnz",
      "blstest:dzycfmal4jlhjjqhnv34zumc5pz749w4sgu0mkg5yx",  "blstest:dp4y6k2075ft96pwnr7autrvme694saexg74jqc8gc",
      "blstest:dpy558tskylf64vmz4l3d67ttfr50q6ncy0h9fpj3e",  "blstest:dzp3hf686fyymgu9r3aqxvw7kdvegldsrcdpwpeyvq",
      "blstest:dpslw4kj7q83zrd562y7sczw0s97aswhnccemcxxa4",  "blstest:dqhss0zcd87ngs9f9vlw9dgv6xxrjtjkjvve5fq4hu",
      "blstest:dzeaptz69k3g6pz2s3f9tkmg4qekyq60rs2mtcyxwl",  "blstest:dr3k8rlnvkjt80sugen20fxsqqs3axwdsgsrn6uzm5",
      "blstest:dp9nnjvqsnd7fpp5llvpyufh9hs9kcfmhguv6eps08",  "blstest:drtwr80hvhjz2hruyq4mqxdzlan57x0qs5pltvlqyw",
      "blstest:dzezcp2lxmna74w8zqgemvtrwaxnwq6t9s3rk9c5ma",  "blstest:dpt8a8lqgz3e9rmty6yrzshg82lggp62vyqz4hwz0r",
      "blstest:dr2v72wjj0h05rvxh0jj6mrftrt6cj4quuzph6aje4",  "blstest:dz2u9t6h0eff7vmk3xr96t25hajjjs37xqd9jsja8u",
      "blstest:dqx5mum45xv7ce5zum975rmdjhl8g97nvq5y7sqs4v",  "blstest:dr9ftcz7jqfcw8kuuu0dzs354t9hd05x4yl6l4c4v3",
      "blstest:dz9rt2vg78nn0fya58vewkveaecremcu2v9cz6tsne",  "blstest:dprkz5ll9w89emgrs2746hj5tcyg3lz3qu5kw9q4c4",
      "blstest:dqfpwllcr86arakqn6a8az9xtc7ddnyz8qemhxxgze",  "blstest:dz8cvj7js5c0p2h5zjwwjd8hhw83tkjf7gwha228xt",
      "blstest:dr77dr0k95n7lwdt0pmhmhcf947sjc5h6y4m64mx69",  "blstest:dzhgmx6xxhylwj6xjxzgtsry8jj5qqyl0qak9r8ezv",
      "blstest:dr2d6wqewl7nu8784vhppa8kad669l36hsg0wgjftu",  "blstest:dp5mykqyu5fn0gkjfr96swgudntnsr39fveqzlkscy",
      "blstest:drfdrjz5cnsz93jz4586mf7esyfn36xuh56cewsyyf",  "blstest:dz4mjen6j68jrlk5qka0t2selez3f4ufcclamgcvxl",
      "blstest:dr6edeye5gvemc0tllxy69svptrrwjhsccyxf9y8vw",  "blstest:dz0zgd3ua5ghxrn394vsfz2e5hpzejvvzsknp8zvh4",
      "blstest:dphpgh8j40dtvsnyutmdz65cxg8wzrx0yqqacv3st0",  "blstest:dz544x2r6yw9jssfg03d4uaceesaj4djqvfyrrchec",
      "blstest:dr4hl4t59zccz50ejmmwmkhpu3d9alryly3zcphtyv",  "blstest:dpz0aarxasnfkew2c0vtelz7820lrqfueg8jwn5krh",
      "blstest:dqj48c8ku5tj9jhajlvh7sg923ru6peedcja49q6vs",  "blstest:dzgskamez2sk3ppvggdmew5cpawm2449d5vjjf3v0a",
      "blstest:dqvmzf9tyeu3zak5drp3zd5q37wjh4upksfsm6sx7f",  "blstest:dze5lqnngukdrqcevjt3a3xfdn9zs6eu3sz2tf62w6",
      "blstest:dzrtmj5ucz2ek2u7pc7uw9jrrzktm0rrxyxcghvnda",  "blstest:dzkcjz53p04fapxm0wesh05p5aysdntc5urms4c755",
      "blstest:dzjuklmn7z0v2cqxy5fwu6s3y5gke9pk8vw3nr6yjz",  "blstest:dz22gqx9246607ayy42nafje9auwwtv9tqn4h93men",
      "blstest:dp7ldpuj0dy3n8pap95anfg22fg659sckgp8hhv4jw",  "blstest:dq3lwmnm98xr5cpt32aj9n9xrrq3lc7gj53dt40ut6",
      "blstest:dzrz89ppt0pdsrtqar8g4ap0dwdrqr4n2grywmxtsr",  "blstest:dp6dncguz5cf5p6qs63clpd8p9mmggghagn9rkje4u",
      "blstest:dq5hmqe53e7s09wmp5dqnkwr5yxq6q5mputhrksff9",  "blstest:dps4h7kv4lppc2jxkhv4hsnypw7hzz5ugsx924jkq5",
      "blstest:dqnj8w4dywmszwx0kyn6xqmaa4ruyhfxksxpevqjzf",  "blstest:dqfs70y28ytl2qhtsk4k584ce7klgj6x0chte8dkdv",
      "blstest:dzcn7s90apq4mn5d9p0hzttmamf8v38405wnq9sahk",  "blstest:dzmy0l2qx6rte8ydg6ee0kh6atl9rc5u0yryrkn7vv",
      "blstest:dzrgq3sv6t44qtf5nv4kwpchelt45893gun8j9qvmc",  "blstest:dqlnh22azqg24t2craxw0xcdzh8tsrndtcxyu2ykv3",
      "blstest:dqjwr3dadyhyz50zh9jknnfzgy9lw5ntl5ff2kngn6",  "blstest:dqwh3glj4qyg2dyh5au0nfrzwg879c2qxcckfpcpve",
      "blstest:drfe5aqfedmj6vsyeswjzwndt6m68sfjxce6agytk0",  "blstest:dpxsyg0gf35pa5axv0kec39nftv0wzqumvgm8hmwj6",
      "blstest:drn7wkdwf0sjkjq4c3mzxf7z8leq44x0xcc988h8vn",  "blstest:dr9cvgfjrllazz48c7tkqah2f2q9lkcnqv84c9ta5p",
      "blstest:dz4s27knscgw0xfyk285m64ucs2pm5kc8shy5z2c9z",  "blstest:dzdd6e9whcxq8cuxym2e23wqv9svnyuyxv78352ahe",
      "blstest:dzeayjwgn6jlkdk20wu7ktet5x3reke5mclp0fskhr",  "blstest:dryrx0an063acwsp88hgce8ev3rp92c5tv3zfrx46k",
      "blstest:dqss2u5muetv4va3nlfqfkw4cceat8guqv2mtpsnsu",  "blstest:dqdkmt50dvtru3flr9lrt4vmtfsvplencqpzcvqsvl",
      "blstest:drt2l2hu3js729t7n76p08lmawtnxnjavsx5ctjsrp",  "blstest:dz7vq76l06xjj6uaq6m7nv7v8upcahtl8yakj3nz4n",
      "blstest:dr5xlkd3q8rxg6vfl2yewsvys4d9444yzysdzrlhut",  "blstest:dpeexyljx3l6d88kpmcmzyvd3dfnhlc58yu3z5ltya",
      "blstest:drwysmvffur74cvxhkmm80as8y7p86p5xqnrm8fugl",  "blstest:dz2tpepxapqge3zjavdr87rcya8d82q6vgek7m7jqg",
      "blstest:dpwmp8d6zkp73nrd4yrjauz8jhm5mzyxyugdh26nr9",  "blstest:dzuf9mg02e3utjcsdq5hwu90vym4f7snvy5ryzh4ad",
      "blstest:dzw6tggfvw6qj4qf3nmqa42kp0r4gl539sk0gjg3am",  "blstest:drg5yqz38qvu6ag99ayt0fn5vdww00p4hu5t6y0g5s",
      "blstest:dzy8pdnvq9v5v3s96rnlskvknzmrqxjgqsp84kv6ak",  "blstest:dq4xs96xunsc7y9fpff73wsv40rzjvw6t5e2hhwp4q",
      "blstest:dqqg9caylc3lcul8mlc7dej0pa7mfk5auguafkksda",  "blstest:dpkfcnxh54cpmws0aeqydln9l5zt0d3dt5709p98wc",
      "blstest:dp0wqhycmsm0qk2vhvcz2aca83nksaux9sy56qlt5r",  "blstest:dpjft9mqdwmr9n0lvupqtphf9lguqk76z5wlfm7260",
      "blstest:dz9xynwu2c684px069evy0jyzpqxqua4egg64kc3nz",  "blstest:dqffxt47fwk4njek6zq8nerc7rk7z4kyygalgje9nh",
      "blstest:dz6wqqh0rs5un5l2lhnw69k5xm3jcdlxec0n6jsdrs",  "blstest:dzqz59jar3kg3drvw7j683jz7dvtml6egu0nsjd0p4",
      "blstest:dqxyzrhl30z57q2mmfs2v8ksvd570hrt0cpfzwkwsc",  "blstest:dz20zpnzs5ruw2e3gcmvgyu8dvtrrux0zcttqry6de",
      "blstest:dru3uc62wfmfpsdpaernrzpd7u2u3uum5vd60qzckz",  "blstest:dz9n5myvaclgx4kn90c9vkytwp232pgnlsp9ljtuv9",
      "blstest:dz9g9zksws9a06zrym58w92q47e68t0t8yrv50aqtf",  "blstest:dpdhgsfjqm54y9u2yxh73g7ge3hu3wyuh5a6lzppqf",
      "blstest:dp4nsmtxsvkfhl72lswhqu9t5ued9dulfshwadl5kt",  "blstest:dqh9x50mwswwvytg06gxdam6e23tv3cxny3hkmswpn",
      "blstest:dp6ycgfzpq40rscg4nu5m5azu9yg4af8tcvqvm9jqz",  "blstest:dr4uw4e97xtcx627getyfr3r3arsug5wqumhj7wfxr",
      "blstest:dzaqhkum4tl9u84vghu5twwgsmfj9fqqwgw5cvstqh",  "blstest:dpl4ku0y42zk47au7u7zdrq9wday5vgexuulueqpue",
      "blstest:dr0656tg865gk78duvulcevr5vn2qpf6xv0p2sknvf", "blstest:dz6ezzlftk4se8g9rz2k8k9lg7e75gssmc4ucgh5c9",
      "blstest:dptvhde4rpay4lryztq08u03kslhtdz3sup0rll2ud", "blstest:dzqz6nsxn9l2aqx8mgml3vcr3750vxcpdcq94awajy",
      "blstest:dzxp00n0sj7tl75heugmtwh48e63ru3jlsxvzn5z9z", "blstest:dz0ag2c0vmzla2xcdq9t0yrm0s4kya04mcje8umfz4",
      "blstest:dr6afhkgt6efnwcpsarjyseeajunmwm0cusz3eekp0", "blstest:dpfnq9ck0gcg52j64glf5xq4y8j3wx3w2q3u0wrved",
      "blstest:dr7m7s7usqumdvtev4u5qwpnjlzkkkmltclyfypvwu", "blstest:dz79045ttur9tt4840vh6n6005g2hx3ewy7lmf97ph",
      "blstest:dpqvlvc2ckqydw8ax7yd6d6a9ekp2t0cwqgzenx7jj", "blstest:dpegz9uu9nkj3emfgudwfwlyuu6pfr4t6cf4cywgjp",
      "blstest:dr7nmf8snt0vw9addzl4kwmdmpxya9venqkll4dutr", "blstest:dq8ezca8jpr2xres2zwwpnsml7zcdd7h8caxlt9f4m",
      "blstest:dpq5zt5pp889agexqdgdvzunwx93cqv79q72hwqery", "blstest:dppvg2g4ax78w0u76jq80xxryz9yctchsv8ueujmtp",
      "blstest:drs7lfv5avwlxdcx5x3jk6aq549vh3q2e5pvshvzs4", "blstest:dpysgm3t3t64xyk8uhz54eudc29qy5nrp5y5a52mtc",
      "blstest:dzta5u8sepcw6daugudsqw6ramfrvlwl9slf04twq0", "blstest:drfjtfwh6rza43h0kzl5wwawcuhtg2y79g7pz4e356",
      "blstest:drd3vry25xc84fgczjvvzv3skpnsykngz5flp6u837", "blstest:dp5rclhl9uyfpzzjkswrvd0t5jf3nw39ev578fg83y",
      "blstest:dqvpql60qvpgr674azg0t8xvxcmtq0375q2xyh2p2x", "blstest:dpynyuvjutrjkh8maj59ukyxg3tmce048qpsdd9l00",
      "blstest:dpmprj2mxqexd8eng3cmds6a8cvz7zqleyqns7u709", "blstest:dphm0jkhk0qwdn0hc4en8yunm9kz4nurzv2x9jsdw7",
      "blstest:dprvyuq43crx2ezzsmzplp7vnwkdnjmds5phq38ks8", "blstest:dppsv07vlurl5p5jpw7udsgcscdespv84y2ahx5ymu",
      "blstest:dq5yypa7munl3sm4ghydywst33xa8tq90qp2z6yzgx", "blstest:dqdxk8gmhsk0ck39yf7ef7p0029gn9uuycdg2r82q9",
      "blstest:dp9pa6s2xc7649exkaz7w4kfln9ltgmzuyq076edp7", "blstest:drkyk624qgd8tfgtstn5asfzdhvc98kt2gf98aq4pr",
      "blstest:dzkfgdqh6yalczh0cjddvwhfwfw3ycc33ywys2yeul", "blstest:drhp3mg8pszn9f99ympljl7kc09mzh43sudtx7dy5f",
      "blstest:dp92t46tum7e9wa69zwnhmg3f8jg43j4jgg542d3g7", "blstest:dzfwjat22znsrurz5pmj8ykn9hm6nsxmtqjuhdsjfd",
      "blstest:dza8xnjqzcddrg9z406f77hrf0x4j9s8xv8xtu365y", "blstest:dphkqeu2fsaqlazvngyh8dqdrt7uyvvzwutrfe4x5p",
      "blstest:drfxp6g4055ccag264rz4qa3qvwt7em6qvqneftuxw", "blstest:drddpxyhpaxj4yxp9z84uw8nz4gcqvsvuye6xjt8fz",
      "blstest:dzu0n6mzx0ke8u9q8utxhthfyqzezfxyg5dggk3av6", "blstest:drcupnjn7zdq372frs2cenxd75zyyr72zyrwa73lrx",
      "blstest:dqpjkje7dwavmjm7sfw6v23yt8cj6j683ctwyhee5r", "blstest:dzy5l549g7df5a5syuzjgfecnkzznaeuwc3pugdz93",
      "blstest:dqs9cp9dqj8pjw7ngjc8rdxsmcu2n9y3355kgf8vjh", "blstest:dr0uvty2s4xlnwhz6kdlsl6q5pljgpjcd5pv8xhedd",
      "blstest:dr7jxz9deu4v8g7cqt70w7eztehu2zr22g6c6qjrau", "blstest:dzy268ah8ra7kzecaeznlf99edc6x0mz5ssg6u3qhg",
      "blstest:drkz6hmup2r8gld3x8mdyy5kq9eduy4e8ca4hh5rgx", "blstest:dpa7geqz888p7wmdkgkpsunv4hvamuujjy8hp6908n",
      "blstest:drpnudtjj5nldlmnu4356ncgk6j6l2gvuctqlzhhna", "blstest:dpctzmutuvuake9cvhdxjvt228mgdmemcvt98qap6g",
      "blstest:dzjxn6gq8l0gn50vt2ee29amtzqf8ve9ay86ye88df", "blstest:dqr0lhymrh5qdxgy26tmt7dr29hj8dazhg3tunzyxv",
      "blstest:dqzzvxpz9q0th5y8uy8mgcdv6m5frkjzhqpeuapwhu", "blstest:dpdakg5906afn9my9vuepnwfr5nawshnp5g3976kjj",
      "blstest:dzjsa6uk4tdmnur2u978vhkmhavavvucgya3ghrlfg", "blstest:dz97u7uvemlx38lazjyv9h4kvqfw7sayjg3hyaepcc",
      "blstest:dz5mrcay72vvwuecmn4m0kuqxa0wvju49q38llqfl6", "blstest:dq77ant4vkspaear2s3234qwq9pcq9xvwy5j9zz8s4",
      "blstest:dp4ptj0p8ymyngq76skx8m8t5h2u82wwt5e3m7wu5h", "blstest:dz32mhke4r8mqm300pch83hzjl5f6gnyjvy3r5wdhq",
      "blstest:dpscfe63hny9hgzx3jwsnpl03yf8qak0qq9w5aguma", "blstest:dqfyv54usgraw7qlnnfh7jmnzlgrpkp2j5yaxgk9yr",
      "blstest:dzxe8m0fjrjvrtljqz3c5eqrdj6avkfdlspm8xfaex", "blstest:dzgst35nhh4rzh6ejj9rcstfs3r6hsdqkqjgpgypfh",
      "blstest:dpgs2vhrzlqfgj00hg7ehhj65nxh3t4js59lyfn973", "blstest:dqvdt4d8tu3u8ekearmxvqvx227d0pvm3ymjjxqf7f",
      "blstest:dqn029rljyct0m7l9ug37r4hv9g7srm9zycmwtfkee", "blstest:drttqlrhz4jnj6svpktthc62f0z9c7addsjcwfgqun",
      "blstest:dq3yz8q5wme0ehwexk6lekjrlm284ax7tyzteyd4ey", "blstest:dp06ya8rq5ela7xlf8aktm0n76hhsswergdvmef3en",
      "blstest:dzlppq8fwshextx88epmx8mzzhlf0qcp4c03e8gkh7", "blstest:dqh3ev3p3kh7dfh9glkm7vejju2cts5casdxxylpju",
      "blstest:drk2ktldl5s945r947ezlz5na86xcvr8svysp73h4y", "blstest:dqjjfcut3ncx4hezc8dlvrvcnh4nk6mfdvxcx00m80",
      "blstest:dq0j0xa2xqkatua70t4g4fsrpxygsnrkvuglycdug0", "blstest:dq53dn9yuq85u3mc4xvzj2nflu89hmfyrs6pdwgn0z",
      "blstest:dq94wj03gy4y39ucgpwfuwetrlavcsgt4vr820err3", "blstest:dp2lvhssunmup49dqctehc6puc6v8kuv4s7jq4sahw",
      "blstest:drga8pdxzqh80dtck2xuch0zvxnvg9p5ect3qt6v44", "blstest:drnzehrwufwkmyulsxeulge7c76zhgawg5u8h0z87m",
      "blstest:dr40rff7eumppwxfhrfgqr7dmjz5m65qycquu369jf", "blstest:dradszle3jealhw6sakp8hvjnuw6mu7mlchrtlyfte",
      "blstest:drzz2w57acc3kw028j60p299frvgk3xutu0ruwzkmj", "blstest:drpcljnjen5r5gx2pg4wl6rjnuar2v3csu2lwjtg0t",
      "blstest:dqx4wj7rdyhnw6k28su3tndzhnyq2vy8sgc4r8u3zf", "blstest:dqusqp93npfy4cqd2r3jh7ck25psw4wy0g6jjfur3t",
      "blstest:dph6v3vcgpjv277kk7rvywfd5meug4ydxgs6kk6jkn", "blstest:dprru7mx8h4tmt8lavf778nc6n3nytqqk5d949f70z"};

  std::vector<Amount> amounts;

  make_public_input_keys(1, input_keys, p2pub);

  // Do for a variety of output sizes
  for (int i = 0; i < 20; i++) {
    for (int k=0;k<2;k++) {
      p2pub[0] = k;
      if (k==0) {
        if ((i>0) && (i<10)) {
          for (int j=0;j<20;j++) out_addresses.pop_back();
        } else if (i>0) {
          if (i==10) for (int j=0;j<10;j++) out_addresses.pop_back();
          else {
            out_addresses.pop_back();
          }
        }
      }
    
      amounts.clear();
      for (size_t j=0;j<out_addresses.size();j++) { amounts.push_back( int(j+1) * COIN ); }

      if (k==0) {
        std::cout << "For " << input_keys.size() << " public key input with " << out_addresses.size() << " outputs, ";
      } else {
        std::cout << "For " << input_keys.size() << "        PKH input with " << out_addresses.size() << " outputs, ";
      }
      const Config &config = GetConfig();
      const CChainParams &chainParams = config.GetChainParams();
      CMutableTransaction t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);
      
      try {
        auto strFail = CreatePrivateTxWithSig(input_keys, p2pub, t0);
        if (strFail) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
        CValidationState state;
        BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                            "Simple deserialized transaction should be valid.");
      } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transactions\n"); }
      
      CTransaction rx(t0);
      bool check = CheckPrivateSigs(rx);
      BOOST_CHECK(check);
      
      for (const auto &v : t0.vin)  BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
      for (const auto &v : t0.vout) BOOST_CHECK(v.IsBLS());
      //std::cout << "last vin script size = " << t0.vin[input_keys.size() - 1].scriptSig.size() << " ";
      std::cout << "Transaction size = " << rx.GetTotalSize() << "\n";
    }
  }
}

TEST_CASE("Check combining 2 bls transactions with 1 input, and 2 outputs each, inputs using public key") {
  BasicTestingSetup setup("test");
  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();

  std::vector<CMutableTransaction> txs;
  CMutableTransaction t0;
  CMutableTransaction t1;

  {
    std::vector<CKey> input_keys;
    std::vector<bool> p2pub;
    make_public_input_keys(1, input_keys, p2pub);
    std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
                                              "blstest:dqw2h6gw5j9as7n53u6zt350057dq8js2c5nwcpaqr"};

    std::vector<Amount> amounts = {10 * COIN, 20 * COIN};

    t0 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);
    CTransaction t_ref_in(t0);

    // Create Private Tx with Signatures and retain map of keys + hashes
    try {
      CreatePrivateTxWithSig(input_keys, p2pub, t0);
      CValidationState state;
      BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t0), state) && state.IsValid()),
                          "Simple deserialized transaction should be valid.");
    } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transaction in 1 input/1 output case\n"); }

    CTransaction t_ref_out(t0);

    // Check Private Tx with Signatures and Extract Hashes, Pub Keys and Aggregate Signature
    bool check_t0_sig = CheckPrivateSigs(t_ref_out);
    BOOST_CHECK(check_t0_sig);

    for (const auto &v : t0.vin)       BOOST_CHECK(IsValidBLSScriptSize(v.scriptSig));
    for (const auto &v : t0.vout)      BOOST_CHECK(v.IsBLS());
    // for (const auto& v : t0.vin) std::cout << "script size = " << v.scriptSig.size() << "\n";
  }

  // DO SAME AS ABOVE FOR ANOTHER TRANSACTION
  {
    std::vector<CKey> input_keys;
    std::vector<bool> p2pub;
    make_public_input_keys(1, input_keys, p2pub);
    std::vector<std::string> out_addresses = {"blstest:dqdypj6ehyjgs5yfssttrt7ucs4sjasvqgxyl0azwy",
                                              "blstest:dza0ygw8yh3a3fnl7jf8asqew529xkegmyamppmkas"};

    std::vector<Amount> amounts = {10 * COIN, 30 * COIN};

    t1 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);

    try {
      CreatePrivateTxWithSig(input_keys, p2pub, t1);
      CValidationState state;
      BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t1), state) && state.IsValid()),
                          "Simple deserialized transaction should be valid.");
    } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transaction in 1 input/1 output case\n"); }

    CTransaction t_ref_out(t1);
    bool check_t1_sig = CheckPrivateSigs(t_ref_out);
    BOOST_CHECK(check_t1_sig);
  }

  txs.push_back(t0);
  txs.push_back(t1);

  try {
    auto combined = combine_transactions(txs);
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(combined), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");

    CTransaction c(combined);
    bool check_combined = CheckPrivateSigs(c);
    BOOST_CHECK(check_combined);
  } catch (...) { BOOST_ERROR_MESSAGE("Exception combining transactions\n"); }
}

TEST_CASE("basic combined transaction check with pre-canned Txes") {

  BasicTestingSetup setup("test");
  std::vector<CMutableTransaction> txs(txes.size());

  for (unsigned int idx = 0; idx < txes.size(); idx++) {
    DecodeHexTx(txs[idx], txes[idx]);
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(txs[idx]), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
  }

  try {
    auto combined = combine_transactions(txs);
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(combined), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");

    CTransaction c(combined);
    bool check_combined = CheckPrivateSigs(c);
    BOOST_CHECK(check_combined);
  } catch (...) { BOOST_ERROR_MESSAGE("Exception combining transactions\n"); }
}
