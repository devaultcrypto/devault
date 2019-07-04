// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/fees.h>

#include <config.h>
#include <policy/policy.h>
#include <txmempool.h>
#include <util.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

Amount GetRequiredFee(unsigned int nTxBytes) {
    return GetRequiredFeeRate().GetFee(nTxBytes);
}

Amount GetMinimumFee(unsigned int nTxBytes, const CCoinControl &coin_control,
                     const CTxMemPool &pool) {
    Amount nFeeNeeded =
        GetMinimumFeeRate(coin_control, pool).GetFee(nTxBytes);

    // But always obey the maximum.
    if (nFeeNeeded > maxTxFee) {
        nFeeNeeded = maxTxFee;
    }

    return nFeeNeeded;
}

CFeeRate GetRequiredFeeRate() {
    return GetConfig().GetMinFeePerKB();
}

CFeeRate GetMinimumFeeRate(const CCoinControl &coin_control,
                           const CTxMemPool &pool) {
    CFeeRate neededFeeRate =
        (coin_control.fOverrideFeeRate && coin_control.m_feerate)
            ? *coin_control.m_feerate
            : payTxFee;

    if (neededFeeRate == CFeeRate()) {
        neededFeeRate = pool.estimateFee();
        // ... unless we don't have enough mempool data for estimatefee, then
        // use fallbackFee.
        if (neededFeeRate == CFeeRate()) {
            neededFeeRate = CWallet::fallbackFee;
        }
    }

    // Prevent user from paying a fee below minRelayTxFee or minTxFee.
    return std::max(neededFeeRate, GetRequiredFeeRate());
}
