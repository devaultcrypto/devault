// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config.h>
#include <chainparams.h>
#include <consensus/consensus.h> // DEFAULT_MAX_BLOCK_SIZE
#include <globals.h>

bool IsSuperBlock(int nBlockHeight) {
  return GetConfig().GetChainParams().GetConsensus().IsSuperBlock(nBlockHeight);
}

GlobalConfig::GlobalConfig() : nMaxBlockSize(DEFAULT_MAX_BLOCK_SIZE) {}

bool GlobalConfig::SetMaxBlockSize(uint64_t maxBlockSize) {
    // Do not allow maxBlockSize to be set below historic 1MB limit
    // It cannot be equal either because of the "must be big" UAHF rule.
    if (maxBlockSize <= LEGACY_MAX_BLOCK_SIZE) {
        return false;
    }

    nMaxBlockSize = maxBlockSize;
    return true;
}

uint64_t GlobalConfig::GetMaxBlockSize() const {
    return nMaxBlockSize;
}

const CChainParams &GlobalConfig::GetChainParams() const {
    return Params();
}

static GlobalConfig gConfig;

const Config &GetConfig() {
    return gConfig;
}

DummyConfig::DummyConfig()
    : chainParams(CreateChainParams(CBaseChainParams::REGTEST)) {}

DummyConfig::DummyConfig(std::string net)
    : chainParams(CreateChainParams(net)) {}

DummyConfig::DummyConfig(std::unique_ptr<CChainParams> chainParamsIn)
    : chainParams(std::move(chainParamsIn)) {}

void DummyConfig::SetChainParams(std::string net) {
    chainParams = CreateChainParams(net);
}

void GlobalConfig::SetExcessUTXOCharge(Amount fee) {
    excessUTXOCharge = fee;
}

Amount GlobalConfig::GetExcessUTXOCharge() const {
    return excessUTXOCharge;
}

void GlobalConfig::SetMinFeePerKB(CFeeRate fee) {
    feePerKB = fee;
}

CFeeRate GlobalConfig::GetMinFeePerKB() const {
    return feePerKB;
}

void GlobalConfig::SetRPCUserAndPassword(std::string userAndPassword) {
    rpcUserAndPassword = userAndPassword;
}

std::string GlobalConfig::GetRPCUserAndPassword() const {
    return rpcUserAndPassword;
}

void GlobalConfig::SetRPCCORSDomain(std::string corsDomain) {
    rpcCORSDomain = corsDomain;
}

std::string GlobalConfig::GetRPCCORSDomain() const {
    return rpcCORSDomain;
}
