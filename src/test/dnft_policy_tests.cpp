// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// 4F — DNFT relay policy (spec Q15 + A4): post-DU1 (SCRIPT_ENABLE_TOKENS) a DNFT envelope output
// is its own standard type (TX_DNFT_ENVELOPE), exempt from the plain-OP_RETURN datacarrier cap,
// and the standard tx size cap rises from 100 KB to the consensus cap (1 MB). Pre-activation
// behavior is unchanged: envelopes are plain TX_NULL_DATA (223-byte aggregate cap) and the 100 KB
// size cap applies.

#include <devault/dnft_envelope.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/script_flags.h>
#include <script/standard.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(dnft_policy_tests, BasicTestingSetup)

namespace {

//! Flags emulating pre-DU1 standardness (no tokens).
constexpr uint32_t PRE_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS & ~SCRIPT_ENABLE_TOKENS;
//! Flags emulating post-DU1 standardness (GetMemPoolScriptFlags ORs SCRIPT_ENABLE_TOKENS in).
constexpr uint32_t POST_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS | SCRIPT_ENABLE_TOKENS;

//! A valid envelope scriptPubKey with a body of `bodySize` filler bytes.
CScript MakeEnvelope(size_t bodySize) {
    dnft::EnvelopeFields fields;
    fields.content_type = std::vector<uint8_t>{'t', 'e', 'x', 't', '/', 'p', 'l', 'a', 'i', 'n'};
    const std::vector<uint8_t> body(bodySize, 0xAB);
    return dnft::BuildDnftEnvelope(fields, Span<const uint8_t>{body.data(), body.size()});
}

//! A P2PKH scriptPubKey (any 20-byte hash; Solver does not validate the key).
CScript MakeP2PKH(uint8_t fill = 0x11) {
    return CScript() << OP_DUP << OP_HASH160 << std::vector<uint8_t>(20, fill) << OP_EQUALVERIFY
                     << OP_CHECKSIG;
}

//! A minimal 1-in/1-out standard tx skeleton the cases below mutate.
CMutableTransaction MakeTxSkeleton() {
    CMutableTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout = COutPoint(TxId(uint256S("11")), 0);
    t.vin[0].scriptSig = CScript() << std::vector<uint8_t>(65, 0); // push-only
    t.vout.resize(1);
    t.vout[0].nValue = COIN; // above the ~0.6 DVT dust floor
    t.vout[0].scriptPubKey = MakeP2PKH();
    return t;
}

} // namespace

BOOST_AUTO_TEST_CASE(solver_typing) {
    std::vector<std::vector<uint8_t>> sols;

    // A valid envelope: TX_NULL_DATA pre-activation, TX_DNFT_ENVELOPE post-activation.
    const CScript env = MakeEnvelope(100);
    BOOST_CHECK(dnft::IsDnftEnvelope(env));
    BOOST_CHECK_EQUAL(Solver(env, sols, PRE_FLAGS), TX_NULL_DATA);
    BOOST_CHECK_EQUAL(Solver(env, sols, POST_FLAGS), TX_DNFT_ENVELOPE);
    BOOST_CHECK_EQUAL(GetTxnOutputType(TX_DNFT_ENVELOPE), "dnftenvelope");

    // A plain OP_RETURN (no DNFT magic) stays TX_NULL_DATA either way.
    const CScript plain = CScript() << OP_RETURN << std::vector<uint8_t>{0xDE, 0xAD};
    BOOST_CHECK(!dnft::IsDnftEnvelope(plain));
    BOOST_CHECK_EQUAL(Solver(plain, sols, PRE_FLAGS), TX_NULL_DATA);
    BOOST_CHECK_EQUAL(Solver(plain, sols, POST_FLAGS), TX_NULL_DATA);

    // Wrong magic stays TX_NULL_DATA too.
    const CScript wrongMagic = CScript() << OP_RETURN << std::vector<uint8_t>{'D', 'N', 'F', 'X'};
    BOOST_CHECK_EQUAL(Solver(wrongMagic, sols, POST_FLAGS), TX_NULL_DATA);

    // Magic-bearing but non-push-only tail: the cheap detector (first push only) still types it
    // as an envelope post-activation — deliberately mirroring consensus, where CheckDnftRules
    // classifies it as an envelope with the same predicate and then rejects the malformed
    // envelope / unclaimed pairing, so such a tx can never enter the mempool or a block anyway.
    CScript badTail = CScript() << OP_RETURN << std::vector<uint8_t>{'D', 'N', 'F', 'T'};
    badTail << OP_CHECKSIG;
    BOOST_CHECK(dnft::IsDnftEnvelope(badTail));
    BOOST_CHECK_EQUAL(Solver(badTail, sols, POST_FLAGS), TX_DNFT_ENVELOPE);
    BOOST_CHECK_EQUAL(Solver(badTail, sols, PRE_FLAGS), TX_NONSTANDARD); // not push-only

    // Envelopes carry data, not addresses.
    txnouttype type;
    std::vector<CTxDestination> addrs;
    int required;
    BOOST_CHECK(!ExtractDestinations(env, type, addrs, required, POST_FLAGS));
    BOOST_CHECK_EQUAL(type, TX_DNFT_ENVELOPE);
}

BOOST_AUTO_TEST_CASE(datacarrier_exemption) {
    std::string reason;

    // An envelope well past the 223-byte plain-OP_RETURN cap (but a small tx): pre-activation it
    // is TX_NULL_DATA and busts the aggregate datacarrier cap; post-activation it is standard.
    CMutableTransaction t = MakeTxSkeleton();
    t.vout.resize(2);
    t.vout[1].nValue = Amount::zero();
    t.vout[1].scriptPubKey = MakeEnvelope(2000);
    BOOST_CHECK(t.vout[1].scriptPubKey.size() > nMaxDatacarrierBytes);
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason, PRE_FLAGS));
    BOOST_CHECK_EQUAL(reason, "oversize-op-return");
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason, POST_FLAGS));

    // The envelope does not count toward the plain-OP_RETURN aggregate: envelope + a plain
    // OP_RETURN exactly at the cap is standard post-activation...
    t.vout.resize(3);
    t.vout[2].nValue = Amount::zero();
    t.vout[2].scriptPubKey = CScript() << OP_RETURN << std::vector<uint8_t>(220, 0x42);
    BOOST_CHECK_EQUAL(t.vout[2].scriptPubKey.size(), nMaxDatacarrierBytes);
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason, POST_FLAGS));

    // ...but one byte more of plain OP_RETURN data still trips the unchanged 223-byte cap.
    t.vout[2].scriptPubKey = CScript() << OP_RETURN << std::vector<uint8_t>(221, 0x42);
    BOOST_CHECK(!IsStandardTx(CTransaction(t), reason, POST_FLAGS));
    BOOST_CHECK_EQUAL(reason, "oversize-op-return");

    // A zero-value envelope output is not dust (unspendable data carrier).
    t.vout.resize(2);
    BOOST_CHECK_EQUAL(t.vout[1].nValue, Amount::zero());
    BOOST_CHECK(IsStandardTx(CTransaction(t), reason, POST_FLAGS));
}

BOOST_AUTO_TEST_CASE(tx_size_cap_a4) {
    std::string reason;

    // A ~500 KB mint-shaped tx (inscribed-scale envelope): "tx-size" pre-activation (100 KB cap,
    // checked before output types), standard post-activation — which also exercises the
    // datacarrier exemption at real mint scale.
    CMutableTransaction t = MakeTxSkeleton();
    t.vout.resize(2);
    t.vout[1].nValue = Amount::zero();
    t.vout[1].scriptPubKey = MakeEnvelope(500'000);
    {
        const CTransaction ct(t);
        BOOST_CHECK(ct.GetTotalSize() > MAX_STANDARD_TX_SIZE);
        BOOST_CHECK(ct.GetTotalSize() <= MAX_STANDARD_TX_SIZE_DU1);
        BOOST_CHECK(!IsStandardTx(ct, reason, PRE_FLAGS));
        BOOST_CHECK_EQUAL(reason, "tx-size");
        BOOST_CHECK(IsStandardTx(ct, reason, POST_FLAGS));
    }

    // Past the consensus cap (1 MB) it stays non-standard even post-activation.
    t.vout[1].scriptPubKey = MakeEnvelope(1'000'100);
    {
        const CTransaction ct(t);
        BOOST_CHECK(ct.GetTotalSize() > MAX_STANDARD_TX_SIZE_DU1);
        BOOST_CHECK(!IsStandardTx(ct, reason, POST_FLAGS));
        BOOST_CHECK_EQUAL(reason, "tx-size");
    }

    // The raised cap is not envelope-specific (A4 raises the tx size cap, period): a >100 KB
    // all-P2PKH tx is likewise standard only post-activation.
    CMutableTransaction big = MakeTxSkeleton();
    big.vout.assign(3100, CTxOut(COIN, MakeP2PKH()));
    {
        const CTransaction ct(big);
        BOOST_CHECK(ct.GetTotalSize() > MAX_STANDARD_TX_SIZE);
        BOOST_CHECK(ct.GetTotalSize() <= MAX_STANDARD_TX_SIZE_DU1);
        BOOST_CHECK(!IsStandardTx(ct, reason, PRE_FLAGS));
        BOOST_CHECK_EQUAL(reason, "tx-size");
        BOOST_CHECK(IsStandardTx(ct, reason, POST_FLAGS));
    }
}

BOOST_AUTO_TEST_SUITE_END()
