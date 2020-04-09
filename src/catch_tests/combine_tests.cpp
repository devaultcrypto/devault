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

#include <core_io.h>
#include <univalue.h>

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
      scriptPubKey << OP_DUP << OP_BLSKEYHASH << 20
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
    std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
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

TEST_CASE("Check combining 2 bls transactions with 1 input, and 2 outputs each, 1 output is exact same in the 2 transactions. inputs using public key") {
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
    std::vector<std::string> out_addresses = {"blstest:drw43xma6fqg76s2ru8c433sghd3kynypqgm9dmuun",
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


TEST_CASE("Check combining 3 bls transactions with 1 input, and 2 outputs each, inputs using public key") {
  BasicTestingSetup setup("test");
  const Config &config = GetConfig();
  const CChainParams &chainParams = config.GetChainParams();

  std::vector<CMutableTransaction> txs;
  CMutableTransaction t0;
  CMutableTransaction t1;
  CMutableTransaction t2;

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
    
    std::vector<Amount> amounts = {10 * COIN, 20 * COIN};

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


  {
    std::vector<CKey> input_keys;
    std::vector<bool> p2pub;
    make_public_input_keys(1, input_keys, p2pub);
    std::vector<std::string> out_addresses = {"blstest:dqdypj6ehyjgs5yfssttrt7ucs4sjasvqgxyl0azwy",
                                              "blstest:dza0ygw8yh3a3fnl7jf8asqew529xkegmyamppmkas"};

    std::vector<Amount> amounts = {11 * COIN, 23 * COIN};

    /*
    std::vector<std::string> out_addresses = {"blstest:drt2l2hu3js729t7n76p08lmawtnxnjavsx5ctjsrp",
                                              "blstest:dz7vq76l06xjj6uaq6m7nv7v8upcahtl8yakj3nz4n"};
    std::vector<Amount> amounts = {15 * COIN, 22 * COIN};
*/
    t2 = SetupBLSTx(chainParams, input_keys, p2pub, out_addresses, amounts);
    CTransaction t_ref_in(t2);

    // Create Private Tx with Signatures and retain map of keys + hashes
    try {
      CreatePrivateTxWithSig(input_keys, p2pub, t2);
      CValidationState state;
      BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(t2), state) && state.IsValid()),
                          "Simple deserialized transaction should be valid.");
    } catch (...) { BOOST_ERROR_MESSAGE("Exception creating transaction in 1 input/1 output case\n"); }

    CTransaction t_ref_out(t2);

    // Check Private Tx with Signatures and Extract Hashes, Pub Keys and Aggregate Signature
    bool check_t2_sig = CheckPrivateSigs(t_ref_out);
    BOOST_CHECK(check_t2_sig);

  }

  txs.push_back(t0);
  txs.push_back(t1);
  txs.push_back(t2);

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
/*
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
*/

/*
std::vector<std::string> tx2 = {
                                "0300000001edc9fa42156a249da79fd50cb06b5d8a8ff0846c46163faa799473b7faf3200500000000f730927f269d18236fe5534597c3d990faf1f9acc315d66393bcad3dfe6bcd28f98e9e25648f33604982f11caa5e85fb1f21300103e4903396cd36b2626dc111761307859efd9385c3183b378cda002bb9836760ec402b1eda7e13f4e326b37379689c3083a7b35e7f7b4336774c0308bdcf3edded958544ba83cf4e033d80ae4df0795119ea69aa8f33e415323cfee6837b5eaf4c6196f1ec7aa870ab786267ccb551fc2a90e798368baf9ad0e407aafe05e402362b0f79b09a246a89dcb306b3d187ba348805905d5d5213989eb1875c81ac269260f285492058be72abbc7d525301b021f820fdd34fe15f9324821b3ddcba0b911d01acfeffffff0200752b7d000000001976bd14c77c2aa33d954186fb4b923f71eda08ceb32233a88ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888acfe1c0000",
                                "0300000001d95924a2a8918369ddba982271c095cd398945716897a0850381950d0dc004ea00000000f7301611445ca8c86e4c60c271a53a882656134af531a2f5f1de4b692b244c0b02adbb64e282ce9b70db1c65f7aa33afc883308e021ff956296fb2a13c0262c8664497eff518d31af8a9ddf8a7d14d467ba020c80f97cb1d529038ce9a9cbc3493f7303097ab9394aff0288d450492abd07573fd6bcb3f9a7102d0ca3ce40eb6d318b1a9d096f219e60843b4c233fc9aa8735f204c6195137c7858d11d61cefe8eb95a85f205e1c1e42b229b142f9ffcafa1e2b7001fb514784930c264060232483e0201b4030f41a8df7821b7e1fbd6e0301122cb9d241d7cb53b7970be94cc86ab5ef57ea97e6a7e813f4757120ad3c460757ff06001acfeffffff0200752b7d000000001976bd14d0c1c37030ac38ec2373d2ea30422feb6f6a28c488ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888acfe1c0000",
                                "0300000001690369b4649fef9812c91fca43c95faeabae5e52e32c18fae6ebe5c66779479100000000f73088a8b09f8cdbafd8d2813d547ad9a30dd7415b3dda757258b76521e6b200836b3f633a64f1881bd8daa8b70ab7e0fc0130175570c4f3c16b502ec768901b708c598c0a3c14ce4547e853dd05a996e86e61173e5ee71346d4c7326533a3c136652630181a41a17250eacb80ca7034556f47db3ed51f8da5263947f4dcdeb900d0cfe6c57e2616dbb7a261a9e9a2c71792cf584c6117a5a01fca5301b0ce2be8916d75e1f57f93dbb98be4b3e9ea097c314eadbc3582d1944ea69a26416607d3e0d68032c417abbf18d3ec5bcbb066b7d35bb42ec6d5e49d5523784ba936ca0957c28a29553ccdbcad0b93700f700a1d371c7ed38501acfeffffff0200752b7d000000001976bd141c5059e8484ca695371e94d2f31452098cd8514088ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888acd21c0000"};

                                
std::string Combined = "0300000003690369b4649fef9812c91fca43c95faeabae5e52e32c18fae6ebe5c66779479100000000313088a8b09f8cdbafd8d2813d547ad9a30dd7415b3dda757258b76521e6b200836b3f633a64f1881bd8daa8b70ab7e0fc01feffffffd95924a2a8918369ddba982271c095cd398945716897a0850381950d0dc004ea0000000031301611445ca8c86e4c60c271a53a882656134af531a2f5f1de4b692b244c0b02adbb64e282ce9b70db1c65f7aa33afc883feffffffedc9fa42156a249da79fd50cb06b5d8a8ff0846c46163faa799473b7faf3200500000000fdbb0130927f269d18236fe5534597c3d990faf1f9acc315d66393bcad3dfe6bcd28f98e9e25648f33604982f11caa5e85fb1f2130175570c4f3c16b502ec768901b708c598c0a3c14ce4547e853dd05a996e86e61173e5ee71346d4c7326533a3c136652630181a41a17250eacb80ca7034556f47db3ed51f8da5263947f4dcdeb900d0cfe6c57e2616dbb7a261a9e9a2c71792cf58308e021ff956296fb2a13c0262c8664497eff518d31af8a9ddf8a7d14d467ba020c80f97cb1d529038ce9a9cbc3493f7303097ab9394aff0288d450492abd07573fd6bcb3f9a7102d0ca3ce40eb6d318b1a9d096f219e60843b4c233fc9aa8735f20300103e4903396cd36b2626dc111761307859efd9385c3183b378cda002bb9836760ec402b1eda7e13f4e326b37379689c3083a7b35e7f7b4336774c0308bdcf3edded958544ba83cf4e033d80ae4df0795119ea69aa8f33e415323cfee6837b5eaf4c6118d95c8d09eabc9ba230cb25c8952ccd10932fb6270d50843ae932eb87eb6fd78dc55d71b005164fd0b795c53be96bb10644875420989f14b7e28fe304eb83cbd9a0e3d92bba21162dbce5554d82f59a8f97a3415259924020519e5a7afa199e01acfeffffff0600752b7d000000001976bd141c5059e8484ca695371e94d2f31452098cd8514088ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888ac00752b7d000000001976bd14d0c1c37030ac38ec2373d2ea30422feb6f6a28c488ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888ac00752b7d000000001976bd14c77c2aa33d954186fb4b923f71eda08ceb32233a88ac00e40b54020000001976bd14e05af50d6585e90e974f147adfd5ca0e5e63489888ac00000000";

TEST_CASE("basic combined transaction check with more pre-canned Txes") {

  BasicTestingSetup setup("test");
  std::vector<CMutableTransaction> txs(tx2.size()); //tx6.size());

  std::vector<uint256> msgs;
  std::vector<std::vector<uint8_t>> pubkeys;
  std::vector<std::vector<uint8_t>> aggSigs;

  std::string addresses = {"blstest:dzfzktqx7ppyru4eqvjr9vp233r2lm4uuuszfqrj92",
                           "blstest:dqjqts9y8eg326falshj7pejjmxfrkezeqhk96y4wf"};

  for (unsigned int idx = 0; idx < txs.size(); idx++) {
    DecodeHexTx(txs[idx], tx2[idx]);

#ifdef EXP
    {
      UniValue result(UniValue::VOBJ);
      CTransaction temp(txs[idx]);
      TxToUniv(temp, uint256(), result, false);
      std::cout << "Result = " << result.write(2,4) << "\n"; 
    }
#endif
    
    CValidationState state;
    BOOST_CHECK_MESSAGE((CheckRegularTransaction(CTransaction(txs[idx]), state) && state.IsValid()),
                        "Simple deserialized transaction should be valid.");
    CTransaction c(txs[idx]);
    bool check_individual = CheckPrivateSigs(c);
    BOOST_CHECK(check_individual);
    std::vector<CScript> input_scripts;
    std::vector<uint256> input_hashes;
    std::vector<std::vector<uint8_t>> input_pubkeys;
    std::vector<uint8_t> aggSig;
    for (auto &t : c.vin) input_scripts.push_back(t.scriptSig);
    SetupCheckPrivateSigs(c, input_scripts, input_hashes, input_pubkeys, aggSig);
    for (auto& m : input_hashes) msgs.push_back(m);
    for (auto& m : input_pubkeys) pubkeys.push_back(m);
    aggSigs.push_back(aggSig);
    auto recheck = bls::VerifySigForMessages(input_hashes, aggSig, input_pubkeys);
    BOOST_CHECK(recheck);
  }

  // Now get combined Sig 
  std::vector<uint8_t> combined_sigs = bls::MakeAggregateSigsForMessages(
                                                                         msgs,
                                                                         aggSigs,
                                                                         pubkeys);

  auto check_agg = bls::VerifySigForMessages(msgs, combined_sigs, pubkeys);
    
  std::cout << "combined sig : " << HexStr(combined_sigs) << "\n";
    

  BOOST_CHECK(check_agg);

 
  
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
*/
