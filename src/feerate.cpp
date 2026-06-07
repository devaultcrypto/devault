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

    // A non-zero rate must still charge for a non-empty tx. SpockQuantize already rounds any positive
    // sub-spock fee up to one spock; this only covers a rate so small the satoshi fee truncated to 0.
    if (nFee == Amount::zero() && nSize != 0) {
        if (nSatoshisPerK > Amount::zero()) {
            nFee = SPOCK_SATS * SATOSHI;
        }
        if (nSatoshisPerK < Amount::zero()) {
            nFee = -(SPOCK_SATS * SATOSHI);
        }
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
    return strprintf("%d.%08d %s/kB", nSatoshisPerK / COIN,
                     (nSatoshisPerK % COIN) / SATOSHI, CURRENCY_UNIT);
}
