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
    return GetConfig().GetMinFeePerKB().GetFee(nTxBytes);
}

Amount GetMinimumFee(unsigned int nTxBytes, const CCoinControl &coinControl,
                     const CTxMemPool &pool) {
    Amount targetFee = (coinControl.fOverrideFeeRate && coinControl.m_feerate)
                           ? coinControl.m_feerate->GetFee(nTxBytes)
                           : payTxFee.GetFee(nTxBytes);

    Amount nFeeNeeded = targetFee;
    if (nFeeNeeded == Amount::zero()) {
        nFeeNeeded = pool.estimateFee().GetFee(nTxBytes);
        // ... unless we don't have enough mempool data for estimatefee, then
        // use fallbackFee.
        if (nFeeNeeded == Amount::zero()) {
            nFeeNeeded = CWallet::fallbackFee.GetFee(nTxBytes);
        }
    }

    // Prevent user from paying a fee below minRelayTxFee or minTxFee.
    nFeeNeeded = std::max(nFeeNeeded, GetRequiredFee(nTxBytes));

    // But always obey the maximum.
    if (nFeeNeeded > maxTxFee) {
        nFeeNeeded = maxTxFee;
    }

    return nFeeNeeded;
}
