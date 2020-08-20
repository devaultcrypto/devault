// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <cassert>
#include <arith_uint256.h>

static CBlock CreateGenesisBlock(const char *pszTimestamp,
                                 const CScript &genesisOutputScript,
                                 uint32_t nTime, uint32_t nNonce,
                                 uint32_t nBits, int32_t nVersion,
                                 const Amount genesisReward) {
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig =
        CScript() << 486604799 << CScriptNum(4)
                  << std::vector<uint8_t>((const uint8_t *)pszTimestamp,
                                          (const uint8_t *)pszTimestamp +
                                              strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation transaction
 * cannot be spent since it did not originally exist in the database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000,
 * hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893,
 * vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase
 * 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits,
                          int32_t nVersion, const Amount genesisReward) {
    const char *pszTimestamp =
        "BBC 06/03/2019 Tiananmen's tank man: The image that China forgot";
    const CScript genesisOutputScript =
        CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909"
                              "a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112"
                              "de5c384df7ba0b8d578a4c702b6bf11d5f")
                  << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce,
                              nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.powLimit = uint256S(
            "00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 2 * 60; // 2 minute block time
        consensus.nBlocksPerYear = 30 * 24 * 365.25;
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S(
            "0x00000000000000000000000000000000000000000000000000000000000000f");

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = uint256S(
            "000000000000000003aadeae9dee37b8cb4838a866dae19b54854a0f039b03e0");

        // Date and time (GMT): Saturday, October 10, 2020 4:00:00 PM
        consensus.blsActivationTime = 1602345600;

        /**
         * The message start string is designed to be unlikely to occur in
         * normal data. The characters are rarely used upper ASCII, not valid as
         * UTF-8, and produce a large 32-bit integer with any alignment.
         */
        diskMagic[0] = 0xc0;
        diskMagic[1] = 0xd2;
        diskMagic[2] = 0xe0;
        diskMagic[3] = 0xd1;
        netMagic[0] = 0xde;
        netMagic[1] = 0x3a;
        netMagic[2] = 0x9c;
        netMagic[3] = 0x03;
        nDefaultPort = 33039;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1559660400, 3423714883, 0x1d00ffff, 1,
                                     50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0000000038e62464371566f6a8d35c01aa54a7da351b2dbf85d92f"
                        "30357f3a90"));
        assert(genesis.hashMerkleRoot ==
               uint256S("95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2"
                        "dda326f35b"));

        // Note that of those which support the service bits prefix, most only
        // support a subset of possible options. This is fine at runtime as
        // we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all
        // service bits wanted by any release ASAP to avoid it where possible.
        // DeVault seeder
        vSeeds.emplace_back("seed.devault.cc");
        vSeeds.emplace_back("seed.exploredvt.com");
        vSeeds.emplace_back("dvtapi.com");
        vSeeds.emplace_back("seed.minedvt.com");
        vSeeds.emplace_back("seed.devault.online");
        vSeeds.emplace_back("seed.dvtapi.com");
        vSeeds.emplace_back("seed.proteanx.com");

        nExtCoinType = 339;
        cashaddrPrefix = "devault";
        cashaddrSecretPrefix = "dvtpriv";
        blsAddrPrefix = "dvt";

        // Rewards
        consensus.nPerCentPerYear = {15,12,9,7,5};
        consensus.nMinRewardBlocks = consensus.nBlocksPerYear/12; // every month
        consensus.vecMinRewardBalances = {std::tuple<int,Amount>(109575, 1000 * COIN),
                                          std::tuple<int,Amount>(std::numeric_limits<int32_t>::max(), 25000 * COIN)};
        consensus.nMinReward = 50 * COIN;
        consensus.nZawyLwmaAveragingWindow = 72;
        
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            .mapCheckpoints = {
                {5000, uint256S("000000000000000173c13a23fed27056b5a76912a27d62064cb988"
                                 "db13888907")},
                {50000, uint256S("00000000000000600f43cf743ca452b38d4cf175d588089c3c73ca"
                                 "afbc0364cd")},
                {110068, uint256S("00000000000000ab518cf852a114ff655ae01580d26727552a584c"
                                 "62bcf40726")},
                {110420, uint256S("000000000000007edbec10fadbf144be667309ceb4eec9a377bb95"
                                 "0716c4d4a1")},
                {131502, uint256S("00000000000000161d9356272df6aa2c551738634ecb6b4a16b7c8"
                                 "a6bf62c14c")},
            }};

        // Currently inaccurate but gives a better progress indicator
        chainTxData = ChainTxData{1579810314, 1587743, 0.0088};
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        // Reduce this difficult a lot to get started
        consensus.powLimit = uint256S(
            "00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 2 * 15; // 30 seconds
        //consensus.nBlocksPerYear = 30 * 24 * 365.25; 
        consensus.nBlocksPerYear = 3600;
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S(
            "00");

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = uint256S(
            "000000000000030bee568d677b6b99ee7d2d00b25d1fe95df5e73b484f00c322");

        // Date July 11, 2020, 4:00:00 pm GMT
        //consensus.blsActivationTime = 1594483200;
        consensus.blsActivationTime = 1595895427;
        diskMagic[0] = 0x0d;
        diskMagic[1] = 0x08;
        diskMagic[2] = 0x13;
        diskMagic[3] = 0x04;
        netMagic[0] = 0xf4;
        netMagic[1] = 0xe5;
        netMagic[2] = 0xf3;
        netMagic[3] = 0xf4;
        nDefaultPort = 39039;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1597883663, 48113, 0x1f00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

//#define DO_TESTNET_GENESIS
#ifdef DO_TESTNET_GENESIS        
        //---------------------------------------------------------------------------------------------------
        // Automatically calculate values if you change nTimestamp
        //---------------------------------------------------------------------------------------------------
        {
            std::cout << "recalculating params for testnet.\n";
            // deliberately empty for loop finds nonce value.
            arith_uint256 bnTarget;
            bool fNegative;
            bool fOverflow;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            if (bnTarget > UintToArith256(consensus.powLimit)) {
              std::cout << "bnTarget not big enough, change nBits\n";
              exit(0);
            }
            genesis.nNonce--;
            do {
              genesis.nNonce++;
            } while (UintToArith256(genesis.GetHash()) > bnTarget);
            std::cout << "new testnet genesis merkle root: " << genesis.hashMerkleRoot.ToString() << "\n";
            std::cout << "new testnet genesis nonce: " <<  genesis.nNonce << "\n";
            std::cout << "new testnet genesis hash: " <<  genesis.GetHash().ToString() << "\n";
            std::cout << "please update code with new values and re-run\n";
            std::cout << "Hash = " << genesis.GetHash().ToString() << "\n";
            exit(0);
        }
#endif
        //---------------------------------------------------------------------------------------------------
      
        assert(consensus.hashGenesisBlock ==
               uint256S("0000094212f2c4678e08b36188f290b2969f2e429a7f5506764dc11e8894410f"));
        assert(genesis.hashMerkleRoot ==
               uint256S("95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2dda326f35b"));
        
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // DeVault seeder
        vSeeds.emplace_back("69.172.229.236");
        nExtCoinType = 1;
        cashaddrPrefix = "dvtest";
        cashaddrSecretPrefix = "testpriv"; // Shouldn't matter that "dv" isn't indicated
        blsAddrPrefix = "blstest";

        // Rewards
        consensus.nPerCentPerYear = {1500,1200,900,700,500};
        consensus.nMinRewardBlocks = consensus.nBlocksPerYear/12; // every month
        consensus.vecMinRewardBalances = {std::tuple<int,Amount>(2000, 1000 * COIN),
                                          std::tuple<int,Amount>(std::numeric_limits<int32_t>::max(), 25000 * COIN)};
        consensus.nMinReward =  50 * COIN;

        consensus.nZawyLwmaAveragingWindow = 72;
  
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;

        checkpointData = {
            .mapCheckpoints = {
                {0, uint256S("00000000797947527458fac580afda78e5274b3cd3c8ca9c0b53d6"
                               "53891eeed9")},
            }};

        // Data as of block
        // 000000000005b07ecf85563034d13efd81c1a29e47e22b20f4fc6919d5b09cd6
        // (height 1223263)
        chainTxData = ChainTxData{1570974562, 100, 1};
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.powLimit = uint256S(
            "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 2* 60;
        consensus.nBlocksPerYear = 30 * 24 * 365.25;
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        // TBD
        consensus.blsActivationTime = 1999999999;

        diskMagic[0] = 0xfa;
        diskMagic[1] = 0xbf;
        diskMagic[2] = 0xb5;
        diskMagic[3] = 0xda;
        netMagic[0] = 0xda;
        netMagic[1] = 0xb5;
        netMagic[2] = 0xbf;
        netMagic[3] = 0xfa;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1559660400, 3, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("7f39501a21abfd9930011aaf76bed139f16d896ca9bc66f9f4770d"
                        "345459d08a"));
        assert(genesis.hashMerkleRoot ==
               uint256S("95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2"
                        "dda326f35b"));

        //!< Regtest mode doesn't have any DNS seeds.
        vSeeds.clear();

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {.mapCheckpoints = {
                              {0, uint256S("00000000d78ce705553c88affb299cdbcf7c8000c7038b422abe77"
                                           "850eee28a9")},
                          }};

        chainTxData = ChainTxData{0, 0, 0};

        nExtCoinType = 1;
        cashaddrPrefix = "dvreg";
        cashaddrSecretPrefix = "regpriv";
        blsAddrPrefix = "blsreg";

        // Rewards
        consensus.nPerCentPerYear = {15,12,9,7,5};
        consensus.nMinRewardBlocks = consensus.nBlocksPerYear/12; // every month
        consensus.vecMinRewardBalances = {std::tuple<int,Amount>(std::numeric_limits<int32_t>::max(), 1000 * COIN)};
        consensus.nMinReward = 50 * COIN;

        consensus.nZawyLwmaAveragingWindow = 72;

    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string &chain) {
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    }

    if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    }

    if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    }

    throw std::runtime_error(
        strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string &network) {
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
