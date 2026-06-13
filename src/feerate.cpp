// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <feerate.h>

#include <amount.h>
#include <tinyformat.h>

CFeeRate::CFeeRate(const Amount nFeePaid, size_t nBytes_) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0) {
        nSatoshisPerK = 1000 * nFeePaid / nSize;
    } else {
        nSatoshisPerK = Amount::zero();
    }
}

// DeVault: the flat per-transaction minimum fee (legacy feerate.h `MIN_FEE`). The legacy applies it
// inside every CFeeRate::GetFee, so it is the effective floor for relay, the mempool min-fee, dust,
// and the wallet. Dropping it (V2 only charged the rate) is why small V2 txs underpaid the legacy.
static constexpr Amount MIN_FEE = COIN / 5; // 0.2 DVT

template <bool ceil>
static Amount GetFee(size_t nBytes_, Amount nSatoshisPerK) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    // Ensure fee is rounded up when truncated if ceil is true.
    Amount nFee = Amount::zero();
    if (ceil) {
        nFee = Amount(nSize * nSatoshisPerK % 1000 > Amount::zero()
                          ? nSize * nSatoshisPerK / 1000 + SATOSHI
                          : nSize * nSatoshisPerK / 1000);
    } else {
        nFee = nSize * nSatoshisPerK / 1000;
    }

    // DeVault [fee quantization]: fees are denominated in whole "spocks" (0.001 DVT = 100000 sat).
    // Legacy DeVault rounds every Amount up to a spock in the Amount constructor, so all fees, dust,
    // and the min-relay / min-block fee end up spock-multiples. V2 keeps a satoshi-granular Amount
    // (see amount.h), so we round the computed fee up to a whole spock here -- this is the single
    // point every fee rate flows through (min-relay, block-min, dust, wallet). Matches legacy.
    nFee = SpockQuantize(nFee);

    // DeVault [flat fee floor]: every non-empty tx pays at least MIN_FEE (legacy feerate.cpp does
    // `nFee = std::max(nFee, MIN_FEE)`). MIN_FEE = COIN/5 = 0.2 DVT = 200 spocks, so this also subsumes
    // the "at least one spock" floor. This is the per-tx minimum the legacy network enforces, so a V2
    // tx below it (e.g. a 225-byte tx at the rate alone = 0.113 DVT) is rejected by legacy nodes.
    if (nSize != 0 && nFee < MIN_FEE) {
        nFee = MIN_FEE;
    }

    return nFee;
}

Amount CFeeRate::GetFee(size_t nBytes) const {
    return ::GetFee<false>(nBytes, nSatoshisPerK);
}

Amount CFeeRate::GetFeeCeiling(size_t nBytes) const {
    return ::GetFee<true>(nBytes, nSatoshisPerK);
}

std::string CFeeRate::ToString() const {
    // DeVault: 3-decimal (spock) precision, not 8 satoshi digits.
    return strprintf("%d.%03d %s/kB", nSatoshisPerK / COIN,
                     (nSatoshisPerK % COIN) / (SPOCK_SATS * SATOSHI), CURRENCY_UNIT);
}
