// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsconstants.h>
#include <chainparamsseeds.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <netbase.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>

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
        CScript() << ScriptInt::fromIntUnchecked(486604799)
                  << CScriptNum::fromIntUnchecked(4)
                  << std::vector<uint8_t>((const uint8_t *)pszTimestamp,
                                          (const uint8_t *)pszTimestamp + strlen(pszTimestamp));
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
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210000;
        // 00000000000000ce80a7e057163a4db1d5ad7b20fb6f598c9597b9665c8fb0d4 -
        // April 1, 2012
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        consensus.BIP34Hash = BlockHash::fromHex(
            "000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S(
            "00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 2 * 60; // DeVault: 120-second blocks
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // Two days
        consensus.nASERTHalfLife = 2 * 24 * 60 * 60;

        // DeVault: keep minimum chain work low (the real chain has far more) and validate
        // all signatures during history parity. Tightened (real assumevalid checkpoint) in 1K.
        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = BlockHash();

        // --- DeVault-specific consensus parameters (ported from legacy v1.2.1, mainnet) ---
        // DeVault's 2020-10-10 upgrade MTP: enforcement of CHECKDATASIG_SIGOPS / SIGPUSHONLY /
        // CLEANSTACK flips on here (legacy name: blsActivationTime; BLS itself is dropped in V2).
        consensus.scriptUpgradeActivationTime = 1602345600; // 2020-10-10 16:00:00 UTC
        consensus.nBlocksPerYear = 30 * 24 * 365.25; // 262980
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.minerCapSystemChangeHeight = 0; // unused in legacy (never assigned)
        consensus.nPerCentPerYear = {15, 12, 9, 7, 5};
        consensus.nMinRewardBlocks = consensus.nBlocksPerYear / 12; // 21915 (monthly)
        consensus.vecMinRewardBalances = {
            std::tuple<int64_t, Amount>(109575, 1000 * COIN),
            std::tuple<int64_t, Amount>(2147483647 /* INT32_MAX */, 25000 * COIN)};
        consensus.nMinReward = 50 * COIN;
        consensus.nZawyLwmaAveragingWindow = 72;

        // August 1, 2017 hard fork
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2021 12:00:00 UTC protocol upgrade was 1621080000, but since this upgrade was for relay rules only,
        // we do not track this time (since it does not apply at all to the blockchain itself).

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // May 15, 2023 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // May 15, 2024 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // May 15, 2025 12:00:00 UTC protocol upgrade (this is one less than the first block mined under new rules)
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // May 15, 2026 12:00:00 UTC protocol upgrade
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // May 15, 2027 12:00:00 UTC tentative protocol upgrade
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes)
        consensus.nDefaultConsensusBlockSize = DEFAULT_CONSENSUS_BLOCK_SIZE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 50.0; // 50% of 32MB = 16MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // Anchor params: Note that the block after this height *must* also be checkpointed below.
        consensus.asertAnchorParams = Consensus::Params::ASERTAnchor{
            661647,       // anchor block height
            0x1804dafe,   // anchor block nBits
            1605447844,   // anchor block previous block timestamp
        };

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ false);
        // Ensure base ABLA state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA is *not* "fixed size" for mainnet
        assert( ! consensus.ablaConfig.IsFixedSize());

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
        m_assumed_blockchain_size = 2;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1559660400, 3423714883, 0x1d00ffff, 1,
                                     50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0000000038e62464371566f6a8d35c01aa54a7da351b2dbf85d92f"
                        "30357f3a90"));
        assert(genesis.hashMerkleRoot ==
               uint256S("95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2"
                        "dda326f35b"));

        // DeVault DNS seeders (may be stale; history-parity sync uses -addnode with the live
        // peers captured in parity/oracle/peers.txt). vFixedSeeds cleared below.
        vSeeds.emplace_back("seed.devault.cc");
        vSeeds.emplace_back("seed.exploredvt.com");
        vSeeds.emplace_back("dvtapi.com");
        vSeeds.emplace_back("seed.minedvt.com");
        vSeeds.emplace_back("seed.devault.online");
        vSeeds.emplace_back("seed.dvtapi.com");
        vSeeds.emplace_back("seed.proteanx.com");

        // base58 address prefixes are retained only for Base58 WIF dual-decode (secret key);
        // DeVault addresses are CashAddr-only ("devault:"). See WIF decision (Phase 2 wallet).
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 5);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        cashaddrPrefix = "devault";
        cashaddrSecretPrefix = "dvtpriv";
        nExtCoinType = 339; // DeVault [2C]: registered BIP44 coin_type (m/44'/339'/...)

        vFixedSeeds.clear(); // DeVault fixed seeds TBD; use -addnode for parity sync

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;

        checkpointData = {
            /* .mapCheckpoints = */ {
                // DeVault legacy in-code checkpoints (match the live chain; verified vs oracle).
                {5000, BlockHash::fromHex("000000000000000173c13a23fed27056b5a76912a27d62064cb988db13888907")},
                {50000, BlockHash::fromHex("00000000000000600f43cf743ca452b38d4cf175d588089c3c73caafbc0364cd")},
                {110068, BlockHash::fromHex("00000000000000ab518cf852a114ff655ae01580d26727552a584c62bcf40726")},
                {110420, BlockHash::fromHex("000000000000007edbec10fadbf144be667309ceb4eec9a377bb950716c4d4a1")},
                {131502, BlockHash::fromHex("00000000000000161d9356272df6aa2c551738634ecb6b4a16b7c8a6bf62c14c")},
                {1000000, BlockHash::fromHex("000000000000003e909eba597c683d4db683e0214875b399ad632b8be6e0c759")},
                {1500000, BlockHash::fromHex("000000000000023a70bd0a31909c75c8c961678b199a5c9e815a48fbae9f6a46")},
                {1696050, BlockHash::fromHex("0000000000002004569d6237faf25cbeb62e625bdb4bcb9450e2ca6d66ad1977")},
            }};

        // Data as of block
        // 0000000000003ef7702938208eb6ac2218fd0fd3f239f50b3268aa6b67195e02
        // (height 1696141).
        chainTxData = ChainTxData{
            // UNIX timestamp of last known number of transactions.
            1780273943,
            // Total number of transactions between genesis and that timestamp
            // (the tx=... number in the ChainStateFlushed debug.log lines)
            1800000,
            // Estimated number of transactions per second after that timestamp.
            0.008,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210000;
        // 00000000040b4e986385315e14bee30ad876d8b47f748025b26683116d21aa65
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        consensus.BIP34Hash = BlockHash::fromHex(
            "0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        // 00000000025e930139bac5c6c31a403776da130831ab85be56578f3fa75369bb
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S(
            "00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 2 * 15; // DeVault testnet: 30-second blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // One hour
        consensus.nASERTHalfLife = 60 * 60;

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = BlockHash();

        // --- DeVault-specific consensus parameters (legacy v1.2.1, testnet) ---
        consensus.scriptUpgradeActivationTime = 1595895427;
        consensus.nBlocksPerYear = 30 * 24 * 365.25;
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.minerCapSystemChangeHeight = 0;
        consensus.nPerCentPerYear = {1500, 1200, 900, 7, 5};
        consensus.nMinRewardBlocks = consensus.nBlocksPerYear / 12;
        consensus.vecMinRewardBalances = {
            std::tuple<int64_t, Amount>(2000, 1000 * COIN),
            std::tuple<int64_t, Amount>(2147483647 /* INT32_MAX */, 25000 * COIN)};
        consensus.nMinReward = 50 * COIN;
        consensus.nZawyLwmaAveragingWindow = 72;

        // August 1, 2017 hard fork
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // May 15, 2023 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // May 15, 2024 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // May 15, 2025 12:00:00 UTC protocol upgrade (this is one less than the first block mined under new rules)
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // May 15, 2026 12:00:00 UTC protocol upgrade
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // May 15, 2027 12:00:00 UTC tentative protocol upgrade
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes)
        consensus.nDefaultConsensusBlockSize = DEFAULT_CONSENSUS_BLOCK_SIZE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 50.0; // 50% of 32MB = 16MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // Anchor params: Note that the block after this height *must* also be checkpointed below.
        consensus.asertAnchorParams = Consensus::Params::ASERTAnchor{
            1421481,      // anchor block height
            0x1d00ffff,   // anchor block nBits
            1605445400,   // anchor block previous block timestamp
        };

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ true);
        // Ensure base abla state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA *is* "fixed size" for testnet3
        assert(consensus.ablaConfig.IsFixedSize());

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
        m_assumed_blockchain_size = 60;     // 43G
        m_assumed_chain_state_size = 2;     // 1.3G

        genesis =
            CreateGenesisBlock(1570974562, 3551570310, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("00000000797947527458fac580afda78e5274b3cd3c8ca9c0b53d6"
                        "53891eeed9"));
        assert(genesis.hashMerkleRoot ==
               uint256S("95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2"
                        "dda326f35b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // BCHD
        vSeeds.emplace_back("testnet-seed.bchd.cash");
        // Loping.net
        vSeeds.emplace_back("seed.tbch.loping.net");
        // Bitcoin Unlimited
        vSeeds.emplace_back("testnet-seed.bitcoinunlimited.info");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = "dvtest";
        cashaddrSecretPrefix = "testpriv";
        nExtCoinType = 1; // DeVault [2C]: BIP44 coin_type for test networks
        vFixedSeeds.clear();

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;

        checkpointData = {
            /* .mapCheckpoints = */ {
                {0, genesis.GetHash()}, // DeVault testnet (genesis only)
            }};

        // Data as of block
        // 000000001ff57d1ed3ee1d5e0a3c48ac4d8607ccd66602f9a4cdedf5e2cc131a
        // (height 1689425)
        chainTxData = ChainTxData{1766497032 /* time */, 64343360 /* numTx */, 0.00327 /* tx/sec */};
    }
};

/**
 * Testnet (v4)
 */
class CTestNet4Params : public CChainParams {
public:
    CTestNet4Params() {
        strNetworkID = CBaseChainParams::TESTNET4;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        // Note: Because BIP34Height is less than 17, clients will face an unusual corner case with BIP34 encoding.
        // The "correct" encoding for BIP34 blocks at height <= 16 uses OP_1 (0x81) through OP_16 (0x90) as a single
        // byte (i.e. "[shortest possible] encoded CScript format"), not a single byte with length followed by the
        // little-endian encoded version of the height as mentioned in BIP34. The BIP34 spec document itself ought to
        // be updated to reflect this.
        // https://github.com/bitcoin/bitcoin/pull/14633
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        consensus.BIP34Hash = BlockHash::fromHex("00000000b0c65b1e03baace7d5c093db0d6aac224df01484985ffd5e86a1a20c");
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // One hour
        consensus.nASERTHalfLife = 60 * 60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = ChainParamsConstants::TESTNET4_MINIMUM_CHAIN_WORK;

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = ChainParamsConstants::TESTNET4_DEFAULT_ASSUME_VALID;

        // August 1, 2017 hard fork
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        // Note: We must set this to 0 here because "historical" sigop code has
        //       been removed from the BCHN codebase. All sigop checks really
        //       use the new post-May2020 sigcheck code unconditionally in this
        //       codebase, regardless of what this height is set to. So it's
        //       "as-if" the activation height really is 0 for all intents and
        //       purposes. If other node implementations wish to use this code
        //       as a reference, they need to be made aware of this quirk of
        //       BCHN, so we explicitly set the activation height to zero here.
        //       For example, BU or other nodes do keep both sigop and sigcheck
        //       implementations in their execution paths so they will need to
        //       use 0 here to be able to synch to this chain.
        //       See: https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/issues/167
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // May 15, 2023 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // May 15, 2024 12:00:00 UTC protocol upgrade (this is one less than the upgrade block itself)
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // May 15, 2025 12:00:00 UTC protocol upgrade (this is one less than the first block mined under new rules)
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // May 15, 2026 12:00:00 UTC protocol upgrade
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // May 15, 2027 12:00:00 UTC tentative protocol upgrade
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes) (testnet4 is smaller at 2MB)
        consensus.nDefaultConsensusBlockSize = 2 * ONE_MEGABYTE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 100.0; // 100% of 2MB = 2MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // Anchor params: Note that the block after this height *must* also be checkpointed below.
        consensus.asertAnchorParams = Consensus::Params::ASERTAnchor{
            16844,        // anchor block height
            0x1d00ffff,   // anchor block nBits
            1605451779,   // anchor block previous block timestamp
        };

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ true);
        // Ensure base abla state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA *is* "fixed size" for testnet4
        assert(consensus.ablaConfig.IsFixedSize());

        diskMagic[0] = 0xcd;
        diskMagic[1] = 0x22;
        diskMagic[2] = 0xa7;
        diskMagic[3] = 0x92;
        netMagic[0] = 0xe2;
        netMagic[1] = 0xb7;
        netMagic[2] = 0xda;
        netMagic[3] = 0xaf;
        nDefaultPort = 28333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;      // 82M
        m_assumed_chain_state_size = 1;     // 12M

        genesis = CreateGenesisBlock(1597811185, 114152193, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // NOTE: BCH testnet4 is unused by DeVault. The shared genesis pszTimestamp was changed
        // to DeVault's, so the original BCH genesis-hash assert no longer holds; removed to avoid
        // an abort if this (defunct) net is ever constructed.

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet4-seed-bch.toom.im");
        // Loping.net
        vSeeds.emplace_back("seed.tbch4.loping.net");
        // Flowee
        vSeeds.emplace_back("testnet4-seed.flowee.cash");
        // Jason Dreyzehner
        vSeeds.emplace_back("testnet4.bitjson.com");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = "bchtest";
        vFixedSeeds.assign(std::begin(pnSeed6_testnet4), std::end(pnSeed6_testnet4));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            /* .mapCheckpoints = */ {
                {0, genesis.GetHash()},
                {5000, BlockHash::fromHex("000000009f092d074574a216faec682040a853c4f079c33dfd2c3ef1fd8108c4")},
                // Axion activation.
                {16845, BlockHash::fromHex("00000000fb325b8f34fe80c96a5f708a08699a68bbab82dba4474d86bd743077")},
                {38000, BlockHash::fromHex("000000000015197537e59f339e3b1bbf81a66f691bd3d7aa08560fc7bf5113fb")},

                // Upgrade 7 ("tachyon") era (actual activation block was in the past significantly before this)
                {54700, BlockHash::fromHex("00000000009af4379d87f17d0f172ee4769b48839a5a3a3e81d69da4322518b8")},
                {68117, BlockHash::fromHex("0000000000a2c2fc11a3b72adbd10a3f02a1f8745da55a85321523043639829a")},

                // Upgrade 8; May 15, 2022 (MTP time >= 1652616000), first upgrade block: 95465
                {95465, BlockHash::fromHex("00000000a77206a2265cabc47cc2c34706ba1c5e5a5743ac6681b83d43c91a01")},
                {115252, BlockHash::fromHex("00000000ae25e85d9e22cd6c8d72c2f5d4b0222289d801b7f633aeae3f8c6367")},
                {121428, BlockHash::fromHex("00000000002cf277337c504f7ce708cce851d5d20cad2936fedf3be95a9ca5eb")},
                {128070, BlockHash::fromHex("00000000044f34642fa3d91e34678737cc10a821a4696f50c187091c3df480c2")},

                // Upgrade 9; May 15, 2023 (MTP time >= 1684152000), first upgrade block: 148044
                {148044, BlockHash::fromHex("0000000008d96c4423ac92aa200af82819339435251736b08babde1ecaf8a5b6")},

                // Upgrade 10; May 15, 2024 (MTP time >= 1715774400), first block after upgrade: 200741
                {200741, BlockHash::fromHex("0000000007d8ccbb767c269551dd81c520463066bec8654a18f4106aa53dc816")},
                {206379, BlockHash::fromHex("000000001d44ca6c351af579f81703c0a175d1d4554db70ae7b7f7df2919eaf9")},
                {226116, BlockHash::fromHex("000000008416a726dd957835add86d7166ef331b74f2f2ca38928dcd56484f8e")},
                {232000, BlockHash::fromHex("0000000018ef632fc8a77dd4ff6a6e29ad2c7af405af63247c6f28a051b1b817")},

                // Upgrade 11; May 15, 2025 (MTP time >= 1747310400), first block mined with upgrade rules: 253319
                {253319, BlockHash::fromHex("00000000004605937a919cf6f636b68e8137b5cb0226ddbc9e00f386ef02999b")},
                {277407, BlockHash::fromHex("00000000fd6af07e5c4618de0cc36796f9470620eb4610584d9011b9f41d3400")},
            }};

        // Data as of block
        // 0000000040d07c4639ce52cb07deb89016bd4af30ccf5fcdde8fddb62c738e40
        // (height 278588)
        chainTxData = {1766497469 /* time */, 388175 /* numTx */, 0.0034 /* tx/sec */};
    }
};

/**
 * Scalenet
 */
class CScaleNetParams : public CChainParams {
public:
    CScaleNetParams() {
        strNetworkID = CBaseChainParams::SCALENET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        // Note: Because BIP34Height is less than 17, clients will face an unusual corner case with BIP34 encoding.
        // The "correct" encoding for BIP34 blocks at height <= 16 uses OP_1 (0x81) through OP_16 (0x90) as a single
        // byte (i.e. "[shortest possible] encoded CScript format"), not a single byte with length followed by the
        // little-endian encoded version of the height as mentioned in BIP34. The BIP34 spec document itself ought to
        // be updated to reflect this.
        // https://github.com/bitcoin/bitcoin/pull/14633
        consensus.BIP34Hash = BlockHash::fromHex("00000000c8c35eaac40e0089a83bf5c5d9ecf831601f98c21ed4a7cb511a07d8");
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // Two days
        consensus.nASERTHalfLife = 2 * 24 * 60 * 60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = ChainParamsConstants::SCALENET_MINIMUM_CHAIN_WORK;

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = ChainParamsConstants::SCALENET_DEFAULT_ASSUME_VALID;

        // August 1, 2017 hard fork
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        // Note: We must set this to 0 here because "historical" sigop code has
        //       been removed from the BCHN codebase. All sigop checks really
        //       use the new post-May2020 sigcheck code unconditionally in this
        //       codebase, regardless of what this height is set to. So it's
        //       "as-if" the activation height really is 0 for all intents and
        //       purposes. If other node implementations wish to use this code
        //       as a reference, they need to be made aware of this quirk of
        //       BCHN, so we explicitly set the activation height to zero here.
        //       For example, BU or other nodes do keep both sigop and sigcheck
        //       implementations in their execution paths so they will need to
        //       use 0 here to be able to synch to this chain.
        //       See: https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/issues/167
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // May 15, 2023 12:00:00 UTC protocol upgrade
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // May 15, 2024 12:00:00 UTC protocol upgrade
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // May 15, 2025 12:00:00 UTC protocol upgrade
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // May 15, 2026 12:00:00 UTC protocol upgrade
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // May 15, 2027 12:00:00 UTC tentative protocol upgrade
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes)
        consensus.nDefaultConsensusBlockSize = 256 * ONE_MEGABYTE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 6.25; // 6.25% of 256MB = 16MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // ScaleNet has no hard-coded anchor block because will be expected to
        // reorg back down to height 10,000 periodically.
        consensus.asertAnchorParams.reset();

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ false);
        // Ensure base abla state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA is *not* "fixed size" for scalenet
        assert( ! consensus.ablaConfig.IsFixedSize());

        diskMagic[0] = 0xba;
        diskMagic[1] = 0xc2;
        diskMagic[2] = 0x2d;
        diskMagic[3] = 0xc4;
        netMagic[0] = 0xc3;
        netMagic[1] = 0xaf;
        netMagic[2] = 0xe1;
        netMagic[3] = 0xa2;
        nDefaultPort = 38333;
        nPruneAfterHeight = 10000;
        m_assumed_blockchain_size = 250;    // 153G
        m_assumed_chain_state_size = 50;    // 16G

        genesis = CreateGenesisBlock(1598282438, -1567304284, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // NOTE: BCH scalenet is unused by DeVault. Genesis-hash/merkle asserts removed because the
        // shared genesis pszTimestamp was changed to DeVault's (avoids abort if ever constructed).

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("scalenet-seed-bch.toom.im");
        // Loping.net
        vSeeds.emplace_back("seed.sbch.loping.net");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = "bchtest";
        vFixedSeeds.assign(std::begin(pnSeed6_scalenet), std::end(pnSeed6_scalenet));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;

        checkpointData = {
            /* .mapCheckpoints = */ {
                {0, genesis.GetHash()},
                {45, BlockHash::fromHex("00000000d75a7c9098d02b321e9900b16ecbd552167e65683fe86e5ecf88b320")},
                // scalenet periodically reorgs to height 10,000
                {10000, BlockHash::fromHex("00000000b711dc753130e5083888d106f99b920b1b8a492eb5ac41d40e482905")},
            }};

        // Data as of block
        // 00000000a6791274f38bca28465236c4c02873037ec187d61c99b7eaa498033f
        // (height 36141)
        chainTxData = {1660124250 /* time */, 489847053 /* numTx */, 0.00001 /* tx/sec */};
    }
};

/**
 * Chipnet (activates the next upgrade earier than the other networks)
 */
class CChipNetParams : public CChainParams {
public:
    CChipNetParams() {
        strNetworkID = CBaseChainParams::CHIPNET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        // Note: Because BIP34Height is less than 17, clients will face an unusual corner case with BIP34 encoding.
        // The "correct" encoding for BIP34 blocks at height <= 16 uses OP_1 (0x81) through OP_16 (0x90) as a single
        // byte (i.e. "[shortest possible] encoded CScript format"), not a single byte with length followed by the
        // little-endian encoded version of the height as mentioned in BIP34. The BIP34 spec document itself ought to
        // be updated to reflect this.
        // https://github.com/bitcoin/bitcoin/pull/14633
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        consensus.BIP34Hash = BlockHash::fromHex("00000000b0c65b1e03baace7d5c093db0d6aac224df01484985ffd5e86a1a20c");
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // One hour
        consensus.nASERTHalfLife = 60 * 60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = ChainParamsConstants::CHIPNET_MINIMUM_CHAIN_WORK;

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = ChainParamsConstants::CHIPNET_DEFAULT_ASSUME_VALID;

        // August 1, 2017 hard fork
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        // Note: We must set this to 0 here because "historical" sigop code has
        //       been removed from the BCHN codebase. All sigop checks really
        //       use the new post-May2020 sigcheck code unconditionally in this
        //       codebase, regardless of what this height is set to. So it's
        //       "as-if" the activation height really is 0 for all intents and
        //       purposes. If other node implementations wish to use this code
        //       as a reference, they need to be made aware of this quirk of
        //       BCHN, so we explicitly set the activation height to zero here.
        //       For example, BU or other nodes do keep both sigop and sigcheck
        //       implementations in their execution paths so they will need to
        //       use 0 here to be able to synch to this chain.
        //       See: https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/issues/167
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // November 15, 2022 12:00:00 UTC; protocol upgrade activates 6 months early
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // November 15, 2023 12:00:00 UTC; protocol upgrade activates 6 months early
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // November 15, 2024 12:00:00 UTC; protocol upgrade activates 6 months early
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // November 15, 2025 12:00:00 UTC; protocol upgrade activates 6 months early
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // November 15, 2026 12:00:00 UTC; tentative protocol upgrade activates 6 months early
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes) (chipnet is like testnet4 in that it is smaller at 2MB)
        consensus.nDefaultConsensusBlockSize = 2 * ONE_MEGABYTE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 100.0; // 100% of 2MB = 2MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // Anchor params: Note that the block after this height *must* also be checkpointed below.
        consensus.asertAnchorParams = Consensus::Params::ASERTAnchor{
            16844,        // anchor block height
            0x1d00ffff,   // anchor block nBits
            1605451779,   // anchor block previous block timestamp
        };

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ false);
        // Ensure base abla state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA is *not* "fixed size" for chipnet
        assert( ! consensus.ablaConfig.IsFixedSize());

        diskMagic[0] = 0xcd;
        diskMagic[1] = 0x22;
        diskMagic[2] = 0xa7;
        diskMagic[3] = 0x92;
        netMagic[0] = 0xe2;
        netMagic[1] = 0xb7;
        netMagic[2] = 0xda;
        netMagic[3] = 0xaf;
        nDefaultPort = 48333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;      // 242M
        m_assumed_chain_state_size = 1;     // 15M

        genesis = CreateGenesisBlock(1597811185, 114152193, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // NOTE: BCH chipnet is unused by DeVault. Genesis-hash assert removed because the shared
        // genesis pszTimestamp was changed to DeVault's (avoids abort if ever constructed).

        vFixedSeeds.clear();
        vSeeds.clear();
        // Jason Dreyzehner
        vSeeds.emplace_back("chipnet.bitjson.com");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = "bchtest";
        vFixedSeeds.assign(std::begin(pnSeed6_chipnet), std::end(pnSeed6_chipnet));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            /* .mapCheckpoints = */ {
                {0, genesis.GetHash()},
                {5000, BlockHash::fromHex("000000009f092d074574a216faec682040a853c4f079c33dfd2c3ef1fd8108c4")},
                // Axion activation.
                {16845, BlockHash::fromHex("00000000fb325b8f34fe80c96a5f708a08699a68bbab82dba4474d86bd743077")},
                {38000, BlockHash::fromHex("000000000015197537e59f339e3b1bbf81a66f691bd3d7aa08560fc7bf5113fb")},

                // Upgrade 7 ("tachyon") era (actual activation block was in the past significantly before this)
                {54700, BlockHash::fromHex("00000000009af4379d87f17d0f172ee4769b48839a5a3a3e81d69da4322518b8")},
                {68117, BlockHash::fromHex("0000000000a2c2fc11a3b72adbd10a3f02a1f8745da55a85321523043639829a")},

                // Upgrade 8; May 15, 2022 (MTP time >= 1652616000), first upgrade block: 95465
                {95465, BlockHash::fromHex("00000000a77206a2265cabc47cc2c34706ba1c5e5a5743ac6681b83d43c91a01")},

                // Fork block for chipnet
                {115252, BlockHash::fromHex("00000000040ba9641ba98a37b2e5ceead38e4e2930ac8f145c8094f94c708727")},
                {115510, BlockHash::fromHex("000000006ad16ee5ee579bc3712b6f15cdf0a7f25a694e1979616794b73c5122")},

                // Upgrade9 - first block mined under upgrade9 rules for chipnet (Nov. 15, 2022)
                {121957, BlockHash::fromHex("0000000056087dee73fb66178ca70da89dfd0be098b1a63cf6fe93934cd04c78")},
                {122396, BlockHash::fromHex("000000000363cd56e49a46684cec1d99854c4aae662a6faee0df4c9a49dc8a33")},
                {128042, BlockHash::fromHex("0000000010e506eeb528dd8238947c6fcdf8d752ece66517eea778650600edae")},
                {148000, BlockHash::fromHex("000000009788ecce39b046caab3cf0f72e8c5409df23454679dbdcae2bd4dded")},

                // A block after Upgrade 10 activated (Nov. 15, 2023), first block after upgrade: 174520
                {178140, BlockHash::fromHex("000000003c37cc0372a5b9ccacca921786bbfc699722fc41e9fdbb1de4146ef1")},
                {206364, BlockHash::fromHex("00000000146a073b9d4e172adbee5252014a8b4d75c56cce36858311565ae251")},

                // A block after Upgrade 11 activated (Nov. 15, 2024), first block after upgrade: 227229
                {228000, BlockHash::fromHex("00000000144b00db5736b33bd572b3a3a52aa9b4c26ba59fc212aeb68a9b7a20")},
                {232000, BlockHash::fromHex("0000000017d92f88ed2c81885c57f999184860a042250510be06b3edd12e0dc5")},
                {255000, BlockHash::fromHex("000000008654e310c090d6846459500c3a6531044fa9339865865575375db624")},

                // A block after Upgrade 12 activated (Nov. 15, 2025), first block after upgrade: 279792
                {284827, BlockHash::fromHex("000000007af65a6e8853f858b3f2a8e7edabe41e6264410276705b324267d7d8")},
            }};

        // Data as of block
        // 00000000020bed0ca19e9b72ce4b2ac3def4beda35f825bb573c51a7783db3e8
        // (height 285268)
        chainTxData = {1766497373 /* time */, 971240 /* numTx */, 0.0087 /* tx/sec */};
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        // always enforce P2SH BIP16 on regtest
        consensus.BIP16Height = 0; // DeVault: P2SH always-on (see GetNextBlockScriptFlags)
        // BIP34 has not activated on regtest (far in the future so block v1 are
        // not rejected in tests)
        consensus.BIP34Height = 1; // DeVault: coinbase-height enforced from height 1
        consensus.BIP34Hash = BlockHash();
        // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP65Height = 0; // DeVault: CLTV always-on
        // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 0; // DeVault: DERSIG always-on
        // CSV activated on regtest (Used in rpc activation tests)
        consensus.CSVHeight = 0; // DeVault: CSV always-on
        consensus.powLimit = uint256S(
            "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 2 * 60; // DeVault: 120-second blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;

        // The half life for the ASERT DAA. For every (nASERTHalfLife) seconds behind schedule the blockchain gets,
        // difficulty is cut in half. Doubled if blocks are ahead of schedule.
        // Two days. Note regtest has no DAA checks, so this unused parameter is here merely for completeness.
        consensus.nASERTHalfLife = 2 * 24 * 60 * 60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = BlockHash();

        // --- DeVault-specific consensus parameters (legacy v1.2.1, regtest) ---
        consensus.scriptUpgradeActivationTime = 1999999999; // effectively disabled on regtest
        consensus.nBlocksPerYear = 30 * 24 * 365.25;
        consensus.nInitialMiningRewardInCoins = 500;
        consensus.minerCapSystemChangeHeight = 0;
        consensus.nPerCentPerYear = {15, 12, 9, 7, 5};
        // Regtest testability: lower the cold-reward eligibility window + minimum payout so functional
        // tests can exercise the engine and cold-reward mining without the ~21915-block (monthly) wait
        // that mainnet/testnet use. IsSuperBlock still keys off nBlocksPerYear/12, so superblocks stay
        // at 21915 — out of the way of short cold-reward tests. (Mainnet keeps nBlocksPerYear/12 and
        // 50*COIN; see the mainnet param block above.) The reward math (CalculateReward) is unchanged.
        consensus.nMinRewardBlocks = 144;
        consensus.vecMinRewardBalances = {
            std::tuple<int64_t, Amount>(2147483647 /* INT32_MAX */, 1000 * COIN)};
        consensus.nMinReward = COIN;
        consensus.nZawyLwmaAveragingWindow = 72;

        // UAHF is always enabled on regtest.
        consensus.uahfHeight = 0; // DeVault: post-UAHF (always-on)

        // November 13, 2017 hard fork is always on on regtest.
        consensus.daaHeight = 0; // DeVault: post-DAA (difficulty = LWMA)

        // November 15, 2018 hard fork is always on on regtest.
        consensus.magneticAnomalyHeight = 0; // DeVault: CTOR enforced from genesis

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 2000000000; // DeVault: DISABLED (no Schnorr-multisig/minimaldata); Phase-3 TBD

        // May 15, 2020 12:00:00 UTC protocol upgrade
        consensus.phononHeight = 2000000000; // DeVault: DISABLED (classic sigops, not sigchecks); Phase-3 TBD

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionActivationTime = 1605441600;

        // May 15, 2022 12:00:00 UTC protocol upgrade
        consensus.upgrade8Height = 2000000000; // DeVault: DISABLED (no 64-bit/introspection); Phase-3 TBD

        // May 15, 2023 12:00:00 UTC protocol upgrade
        consensus.upgrade9Height = 2000000000; // DeVault: DISABLED (no CashTokens/p2sh32); Phase-3 TBD

        // May 15, 2024 12:00:00 UTC protocol upgrade
        consensus.upgrade10Height = 2000000000; // DeVault: DISABLED (fixed 32MB, no ABLA/VM-limits); Phase-3 TBD

        // May 15, 2025 12:00:00 UTC protocol upgrade
        consensus.upgrade11Height = 2000000000; // DeVault: DISABLED (no BigInt); Phase-3 TBD
        // DeVault Upgrade 1 (DU1) — the V2 hard fork (DNFTs/tokens/introspection/64-bit/p2sh32).
        // Height decided near launch; overridable for tests via -du1activationheight.
        consensus.du1Height = 2000000000;    // DISABLED until the fork height is set
        consensus.ftForkHeight = 2000000000; // FT system deferred (DEVAULT_NFT_SPEC.md §10.8)

        // May 15, 2026 12:00:00 UTC protocol upgrade
        consensus.upgrade12ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // May 15, 2027 12:00:00 UTC tentative protocol upgrade
        consensus.upgrade2027ActivationTime = 9999999999; // DeVault: DISABLED; Phase-3 TBD

        // Default limit for block size (in bytes)
        consensus.nDefaultConsensusBlockSize = DEFAULT_CONSENSUS_BLOCK_SIZE;

        // Chain-specific default for mining block size, in percent of excessive block size (conf: -percentblockmaxsize)
        consensus.nDefaultGeneratedBlockSizePercent = 50.0; // 50% of 32MB = 16MB

        assert(consensus.nDefaultGeneratedBlockSizePercent >= 0.0
               && consensus.nDefaultGeneratedBlockSizePercent <= 100.0);
        assert(consensus.GetDefaultGeneratedBlockSizeBytes() <= consensus.nDefaultConsensusBlockSize);

        // ABLA config -- upgrade 10 adjustable block limit algorithm
        consensus.ablaConfig = abla::Config::MakeDefault(consensus.nDefaultConsensusBlockSize, /* fixedSize = */ false);
        // Ensure base abla state yields same limit as pre-activation.
        assert(abla::State(consensus.ablaConfig, 0).GetBlockSizeLimit() == consensus.nDefaultConsensusBlockSize);
        // Ensure ABLA is *not* "fixed size" for regtest
        assert( ! consensus.ablaConfig.IsFixedSize());

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
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1559660400, 3, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0x7f39501a21abfd9930011aaf76bed139f16d896ca9bc66f9f4770d"
                        "345459d08a"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x95d9f62f327ebae0d88f38c72224407e5dde5157f952cdb70921c2"
                        "dda326f35b"));

        //! Regtest mode doesn't have any fixed seeds.
        vFixedSeeds.clear();
        //! Regtest mode doesn't have any DNS seeds.
        vSeeds.clear();

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            /* .mapCheckpoints = */ {
                {0, genesis.GetHash()}, // DeVault regtest (genesis only)
            }};

        chainTxData = ChainTxData{0, 0, 0};

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = "dvreg";
        cashaddrSecretPrefix = "regpriv";
        nExtCoinType = 1; // DeVault [2C]: BIP44 coin_type for test networks
    }
};


static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string &chain) {
    if (chain == CBaseChainParams::MAIN) {
        return std::make_unique<CMainParams>();
    }

    if (chain == CBaseChainParams::TESTNET) {
        return std::make_unique<CTestNetParams>();
    }

    if (chain == CBaseChainParams::TESTNET4) {
        return std::make_unique<CTestNet4Params>();
    }

    if (chain == CBaseChainParams::REGTEST) {
        return std::make_unique<CRegTestParams>();
    }

    if (chain == CBaseChainParams::SCALENET) {
        return std::make_unique<CScaleNetParams>();
    }

    if (chain == CBaseChainParams::CHIPNET) {
        return std::make_unique<CChipNetParams>();
    }

    throw std::runtime_error(
        strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string &network) {
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

SeedSpec6::SeedSpec6(const char *pszHostPort)
{
    const CService service = LookupNumeric(pszHostPort, 0);
    if (!service.IsValid() || service.GetPort() == 0)
        throw std::invalid_argument(strprintf("Unable to parse numeric-IP:port pair: %s", pszHostPort));
    if (!service.IsRoutable())
        throw std::invalid_argument(strprintf("Not routable: %s", pszHostPort));
    *this = SeedSpec6(service);
}
