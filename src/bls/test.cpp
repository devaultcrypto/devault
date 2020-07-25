
// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define CATCH_CONFIG_RUNNER
#include <thread>

#include "bls.hpp"
#include "catch.hpp"
#include "relic.h"
#include "relic_test.h"
#include "schemes.hpp"
#include "test-utils.hpp"
#include "hkdf.hpp"
#include "hdkeys.hpp"
using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace bls;

void TestHKDF(string ikm_hex, string salt_hex, string info_hex, string prk_expected_hex, string okm_expected_hex, int L) {
    vector<uint8_t> ikm = Util::HexToBytes(ikm_hex);
    vector<uint8_t> salt = Util::HexToBytes(salt_hex);
    vector<uint8_t> info = Util::HexToBytes(info_hex);
    vector<uint8_t> prk_expected = Util::HexToBytes(prk_expected_hex);
    vector<uint8_t> okm_expected = Util::HexToBytes(okm_expected_hex);
    uint8_t prk[32];
    HKDF256::Extract(prk, salt.data(), salt.size(), ikm.data(), ikm.size());
    uint8_t okm[L];
    HKDF256::Expand(okm, L, prk, info.data(), info.size());

    REQUIRE(32 == prk_expected.size());
    REQUIRE(L == okm_expected.size());

    for (size_t i=0; i < 32; i++) {
        REQUIRE(prk[i] == prk_expected[i]);
    }
    for (size_t i=0; i < L; i++) {
        REQUIRE(okm[i] == okm_expected[i]);
    }
}


TEST_CASE("HKDF") {
    // https://tools.ietf.org/html/rfc5869 test vectors
    SECTION("Test case 2") {
        TestHKDF("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                 "000102030405060708090a0b0c",
                 "f0f1f2f3f4f5f6f7f8f9",
                 "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
                 "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
                 42
        );
    }
    SECTION("Test case 2") {
        TestHKDF("000102030405060708090a0b0c0d0e0f"
                 "101112131415161718191a1b1c1d1e1f"
                 "202122232425262728292a2b2c2d2e2f"
                 "303132333435363738393a3b3c3d3e3f"
                 "404142434445464748494a4b4c4d4e4f", // 80 octets
                 "0x606162636465666768696a6b6c6d6e6f"
                 "707172737475767778797a7b7c7d7e7f"
                 "808182838485868788898a8b8c8d8e8f"
                 "909192939495969798999a9b9c9d9e9f"
                 "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", // 80 octets
                 "0xb0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
                 "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
                 "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
                 "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
                 "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", // 80 octets
                 "0x06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244", // 32 octets
                 "0xb11e398dc80327a1c8e7f78c596a4934"
                 "4f012eda2d4efad8a050cc4c19afa97c"
                 "59045a99cac7827271cb41c65e590e09"
                 "da3275600c2f09b8367793a9aca3db71"
                 "cc30c58179ec3e87c14c01d5c1f3434f"
                 "1d87", // 82 octets
                 82
        );
    }
    SECTION("Test case 3") {
        TestHKDF("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                 "",
                 "",
                 "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04",
                 "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8",
                 42
        );
    }
    SECTION("Works with multiple of 32") {
        // This generates exactly 64 bytes. Uses a 32 byte key and 4 byte salt as in EIP2333.
        TestHKDF("8704f9ac024139fe62511375cf9bc534c0507dcf00c41603ac935cd5943ce0b4b88599390de14e743ca2f56a73a04eae13aa3f3b969b39d8701e0d69a6f8d42f",
                 "53d8e19b",
                 "",
                 "eb01c9cd916653df76ffa61b6ab8a74e254ebfd9bfc43e624cc12a72b0373dee",
                 "8faabea85fc0c64e7ca86217cdc6dcdc88551c3244d56719e630a3521063082c46455c2fd5483811f9520a748f0099c1dfcfa52c54e1c22b5cdf70efb0f3c676",
                 64
        );
    }
}

void TestEIP2333(string seedHex, string masterSkHex, string childSkHex, uint32_t childIndex) {
    auto seed = Util::HexToBytes(seedHex);
    auto masterSk = Util::HexToBytes(masterSkHex);
    auto childSk = Util::HexToBytes(childSkHex);

    PrivateKey master = PrivateKey::FromSeed(seed.data(), seed.size());
    PrivateKey child = HDKeys::DeriveChildSk(master, childIndex);

    uint8_t master_arr[32];
    master.Serialize(master_arr);
    auto calculatedMaster = master.Serialize();
    auto calculatedChild = child.Serialize();

    REQUIRE(calculatedMaster.size() == 32);
    REQUIRE(calculatedChild.size() == 32);
    for (int i=0; i<32; i++) {
        REQUIRE(calculatedMaster[i] == masterSk[i]);
    }
    for (int i=0; i<32; i++) {
        REQUIRE(calculatedChild[i] == childSk[i]);
    }
}

TEST_CASE("EIP-2333 HD keys") {
    // The comments in the test cases correspond to  integers that are converted to
    // bytes using python int.to_bytes(32, "big").hex(), since the EIP spec provides ints, but c++
    // does not support bigint by default
    SECTION("EIP-2333 Test case 1"){
        TestEIP2333("3141592653589793238462643383279502884197169399375105820974944592",
                    // 36167147331491996618072159372207345412841461318189449162487002442599770291484
                    "4ff5e145590ed7b71e577bb04032396d1619ff41cb4e350053ed2dce8d1efd1c",
                    // 41787458189896526028601807066547832426569899195138584349427756863968330588237
                    "5c62dcf9654481292aafa3348f1d1b0017bbfb44d6881d26d2b17836b38f204d",
                    3141592653
        );
    }
    SECTION("EIP-2333 Test case 2"){
        TestEIP2333("0x0099FF991111002299DD7744EE3355BBDD8844115566CC55663355668888CC00",
                    // 13904094584487173309420026178174172335998687531503061311232927109397516192843
                    "1ebd704b86732c3f05f30563dee6189838e73998ebc9c209ccff422adee10c4b",
                    // 12482522899285304316694838079579801944734479969002030150864436005368716366140
                    "1b98db8b24296038eae3f64c25d693a269ef1e4d7ae0f691c572a46cf3c0913c",
                    4294967295
        );
    }
    SECTION("EIP-2333 Test case 3"){
        TestEIP2333("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
                    // 44010626067374404458092393860968061149521094673473131545188652121635313364506
                    "614d21b10c0e4996ac0608e0e7452d5720d95d20fe03c59a3321000a42432e1a",
                    // 4011524214304750350566588165922015929937602165683407445189263506512578573606
                    "08de7136e4afc56ae3ec03b20517d9c1232705a747f588fd17832f36ae337526",
                    42
        );
    }
    SECTION("EIP-2333 Test vector with intermediate values"){
        TestEIP2333("c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04",
                    // 5399117110774477986698372024995405256382522670366369834617409486544348441851
                    "0x0befcabff4a664461cc8f190cdd51c05621eb2837c71a1362df5b465a674ecfb",
                    // 11812940737387919040225825939013910852517748782307378293770044673328955938106
                    "1a1de3346883401f1e3b2281be5774080edb8e5ebe6f776b0f7af9fea942553a",
                    0
        );
    }
}

TEST_CASE("Schemes")
{

    SECTION("Basic Scheme")
    {
        uint8_t seed1[5] = {1, 2, 3, 4, 5};
        uint8_t seed2[6] = {1, 2, 3, 4, 5, 6};
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = PrivateKey::FromSeed(seed1, sizeof(seed1));
        G1Element pk1 = BasicSchemeMPL::SkToG1(sk1);
        vector<uint8_t> pk1v = BasicSchemeMPL::SkToPk(sk1);
        G2Element sig1 = BasicSchemeMPL::SignNative(sk1, msg1);
        vector<uint8_t> sig1v = BasicSchemeMPL::Sign(sk1, msg1);

        REQUIRE(BasicSchemeMPL::Verify(pk1, msg1, sig1));
        REQUIRE(BasicSchemeMPL::Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, sizeof(seed2));
        G1Element pk2 = BasicSchemeMPL::SkToG1(sk2);
        vector<uint8_t> pk2v = BasicSchemeMPL::SkToPk(sk2);
        G2Element sig2 = BasicSchemeMPL::SignNative(sk2, msg2);
        vector<uint8_t> sig2v = BasicSchemeMPL::Sign(sk2, msg2);

        // Wrong signature
        REQUIRE(BasicSchemeMPL::Verify(pk1, msg1, sig2) == false);
        REQUIRE(BasicSchemeMPL::Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(BasicSchemeMPL::Verify(pk1, msg2, sig1) == false);
        REQUIRE(BasicSchemeMPL::Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(BasicSchemeMPL::Verify(pk2, msg1, sig1) == false);
        REQUIRE(BasicSchemeMPL::Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = BasicSchemeMPL::Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = BasicSchemeMPL::Aggregate({sig1v, sig2v});
        REQUIRE(BasicSchemeMPL::AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(BasicSchemeMPL::AggregateVerify({pk1v, pk2v}, msgs, aggsigv));
    }

    SECTION("Aug Scheme")
    {
        uint8_t seed1[5] = {1, 2, 3, 4, 5};
        uint8_t seed2[6] = {1, 2, 3, 4, 5, 6};
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = PrivateKey::FromSeed(seed1, sizeof(seed1));
        G1Element pk1 = AugSchemeMPL::SkToG1(sk1);
        vector<uint8_t> pk1v = AugSchemeMPL::SkToPk(sk1);
        G2Element sig1 = AugSchemeMPL::SignNative(sk1, msg1);
        vector<uint8_t> sig1v = AugSchemeMPL::Sign(sk1, msg1);

        REQUIRE(AugSchemeMPL::Verify(pk1, msg1, sig1));
        REQUIRE(AugSchemeMPL::Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, sizeof(seed2));
        G1Element pk2 = AugSchemeMPL::SkToG1(sk2);
        vector<uint8_t> pk2v = AugSchemeMPL::SkToPk(sk2);
        G2Element sig2 = AugSchemeMPL::SignNative(sk2, msg2);
        vector<uint8_t> sig2v = AugSchemeMPL::Sign(sk2, msg2);

        // Wrong signature
        REQUIRE(AugSchemeMPL::Verify(pk1, msg1, sig2) == false);
        REQUIRE(AugSchemeMPL::Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(AugSchemeMPL::Verify(pk1, msg2, sig1) == false);
        REQUIRE(AugSchemeMPL::Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(AugSchemeMPL::Verify(pk2, msg1, sig1) == false);
        REQUIRE(AugSchemeMPL::Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = AugSchemeMPL::Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = AugSchemeMPL::Aggregate({sig1v, sig2v});
        REQUIRE(AugSchemeMPL::AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(AugSchemeMPL::AggregateVerify({pk1v, pk2v}, msgs, aggsigv));
    }

    SECTION("Pop Scheme")
    {
        uint8_t seed1[5] = {1, 2, 3, 4, 5};
        uint8_t seed2[6] = {1, 2, 3, 4, 5, 6};
        vector<uint8_t> msg1 = {7, 8, 9};
        vector<uint8_t> msg2 = {10, 11, 12};
        vector<vector<uint8_t>> msgs = {msg1, msg2};

        PrivateKey sk1 = PrivateKey::FromSeed(seed1, sizeof(seed1));
        G1Element pk1 = PopSchemeMPL::SkToG1(sk1);
        vector<uint8_t> pk1v = PopSchemeMPL::SkToPk(sk1);
        G2Element sig1 = PopSchemeMPL::SignNative(sk1, msg1);
        vector<uint8_t> sig1v = PopSchemeMPL::Sign(sk1, msg1);

        REQUIRE(PopSchemeMPL::Verify(pk1, msg1, sig1));
        REQUIRE(PopSchemeMPL::Verify(pk1v, msg1, sig1v));

        PrivateKey sk2 = PrivateKey::FromSeed(seed2, sizeof(seed2));
        G1Element pk2 = PopSchemeMPL::SkToG1(sk2);
        vector<uint8_t> pk2v = PopSchemeMPL::SkToPk(sk2);
        G2Element sig2 = PopSchemeMPL::SignNative(sk2, msg2);
        vector<uint8_t> sig2v = PopSchemeMPL::Sign(sk2, msg2);

        // Wrong signature
        REQUIRE(PopSchemeMPL::Verify(pk1, msg1, sig2) == false);
        REQUIRE(PopSchemeMPL::Verify(pk1v, msg1, sig2v) == false);
        // Wrong msg
        REQUIRE(PopSchemeMPL::Verify(pk1, msg2, sig1) == false);
        REQUIRE(PopSchemeMPL::Verify(pk1v, msg2, sig1v) == false);
        // Wrong pk
        REQUIRE(PopSchemeMPL::Verify(pk2, msg1, sig1) == false);
        REQUIRE(PopSchemeMPL::Verify(pk2v, msg1, sig1v) == false);

        G2Element aggsig = PopSchemeMPL::Aggregate({sig1, sig2});
        vector<uint8_t> aggsigv = PopSchemeMPL::Aggregate({sig1v, sig2v});
        REQUIRE(PopSchemeMPL::AggregateVerify({pk1, pk2}, msgs, aggsig));
        REQUIRE(PopSchemeMPL::AggregateVerify({pk1v, pk2v}, msgs, aggsigv));

        // PopVerify
        G2Element proof1 = PopSchemeMPL::PopProveNative(sk1);
        vector<uint8_t> proof1v = PopSchemeMPL::PopProve(sk1);
        REQUIRE(PopSchemeMPL::PopVerify(pk1, proof1));
        REQUIRE(PopSchemeMPL::PopVerify(pk1v, proof1v));

        // FastAggregateVerify
        // We want sk2 to sign the same message
        G2Element sig2_same = PopSchemeMPL::SignNative(sk2, msg1);
        vector<uint8_t> sig2v_same = PopSchemeMPL::Sign(sk2, msg1);
        G2Element aggsig_same = PopSchemeMPL::Aggregate({sig1, sig2_same});
        vector<uint8_t> aggsigv_same =
            PopSchemeMPL::Aggregate({sig1v, sig2v_same});
        REQUIRE(
            PopSchemeMPL::FastAggregateVerify({pk1, pk2}, msg1, aggsig_same));
        REQUIRE(PopSchemeMPL::FastAggregateVerify(
            {pk1v, pk2v}, msg1, aggsigv_same));
    }
}

int main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
