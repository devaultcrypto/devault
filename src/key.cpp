// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>

#include <arith_uint256.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>
#include <pubkey.h>
#include <random.h>

#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <secp256k1_schnorr.h>

static secp256k1_context *secp256k1_context_sign = nullptr;

/**
 * These functions are taken from the libsecp256k1 distribution and are very
 * ugly.
 */

/**
 * This parses a format loosely based on a DER encoding of the ECPrivateKey type
 * from section C.4 of SEC 1 <http://www.secg.org/sec1-v2.pdf>, with the
 * following caveats:
 *
 * * The octet-length of the SEQUENCE must be encoded as 1 or 2 octets. It is
 * not required to be encoded as one octet if it is less than 256, as DER would
 * require.
 * * The octet-length of the SEQUENCE must not be greater than the remaining
 * length of the key encoding, but need not match it (i.e. the encoding may
 * contain junk after the encoded SEQUENCE).
 * * The privateKey OCTET STRING is zero-filled on the left to 32 octets.
 * * Anything after the encoding of the privateKey OCTET STRING is ignored,
 * whether or not it is validly encoded DER.
 *
 * out32 must point to an output buffer of length at least 32 bytes.
 */
static int ec_privkey_import_der(const secp256k1_context *ctx, uint8_t *out32,
                                 const uint8_t *privkey, size_t privkeylen) {
    const uint8_t *end = privkey + privkeylen;
    memset(out32, 0, 32);
    /* sequence header */
    if (end - privkey < 1 || *privkey != 0x30u) {
        return 0;
    }
    privkey++;
    /* sequence length constructor */
    if (end - privkey < 1 || !(*privkey & 0x80u)) {
        return 0;
    }
    ptrdiff_t lenb = *privkey & ~0x80u;
    privkey++;
    if (lenb < 1 || lenb > 2) {
        return 0;
    }
    if (end - privkey < lenb) {
        return 0;
    }
    /* sequence length */
    ptrdiff_t len =
        privkey[lenb - 1] | (lenb > 1 ? privkey[lenb - 2] << 8 : 0u);
    privkey += lenb;
    if (end - privkey < len) {
        return 0;
    }
    /* sequence element 0: version number (=1) */
    if (end - privkey < 3 || privkey[0] != 0x02u || privkey[1] != 0x01u ||
        privkey[2] != 0x01u) {
        return 0;
    }
    privkey += 3;
    /* sequence element 1: octet string, up to 32 bytes */
    if (end - privkey < 2 || privkey[0] != 0x04u) {
        return 0;
    }
    ptrdiff_t oslen = privkey[1];
    privkey += 2;
    if (oslen > 32 || end - privkey < oslen) {
        return 0;
    }
    memcpy(out32 + (32 - oslen), privkey, oslen);
    if (!secp256k1_ec_seckey_verify(ctx, out32)) {
        memset(out32, 0, 32);
        return 0;
    }
    return 1;
}

bool CKey::Check(const uint8_t *vch) {
    return secp256k1_ec_seckey_verify(secp256k1_context_sign, vch);
}

void CKey::MakeNewKey() {
    do {
        GetStrongRandBytes(keydata.data(), keydata.size());
    } while (!Check(keydata.data()));
    fValid = true;
}

CPubKey CKey::GetPubKey() const {
    assert(fValid);
    secp256k1_pubkey pubkey;
    size_t clen = CPubKey::COMPRESSED_PUBLIC_KEY_SIZE;
    CPubKey result;
    int ret =
        secp256k1_ec_pubkey_create(secp256k1_context_sign, &pubkey, begin());
    assert(ret);
    secp256k1_ec_pubkey_serialize(
        secp256k1_context_sign, (uint8_t *)result.begin(), &clen, &pubkey,
        SECP256K1_EC_COMPRESSED);
    result.SetSize(clen);
    assert(result.HasCompressedByte());
    assert(result.IsValid());
    return result;
}

bool CKey::SignECDSA(const uint256 &hash, std::vector<uint8_t> &vchSig,
                     uint32_t test_case) const {
    if (!fValid) {
        return false;
    }
    vchSig.resize(CPubKey::SIGNATURE_SIZE);
    size_t nSigLen = CPubKey::SIGNATURE_SIZE;
    uint8_t extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);
    secp256k1_ecdsa_signature sig;
    int ret = secp256k1_ecdsa_sign(secp256k1_context_sign, &sig, hash.begin(),
                                   begin(), secp256k1_nonce_function_rfc6979,
                                   test_case ? extra_entropy : nullptr);
    assert(ret);
    secp256k1_ecdsa_signature_serialize_der(secp256k1_context_sign,
                                            vchSig.data(), &nSigLen, &sig);
    vchSig.resize(nSigLen);
    return true;
}

bool CKey::SignSchnorr(const uint256 &hash, std::vector<uint8_t> &vchSig,
                       uint32_t test_case) const {
    if (!fValid) {
        return false;
    }
    vchSig.resize(64);
    uint8_t extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);

    int ret = secp256k1_schnorr_sign(
        secp256k1_context_sign, &vchSig[0], hash.begin(), begin(),
        secp256k1_nonce_function_rfc6979, test_case ? extra_entropy : nullptr);
    assert(ret);
    return true;
}

bool CKey::VerifyPubKey(const CPubKey &pubkey) const {
    uint8_t rnd[8];
    std::string str = "DeVault key verification\n";
    GetRandBytes(rnd, sizeof(rnd));
    uint256 hash;
    CHash256()
        .Write((uint8_t *)str.data(), str.size())
        .Write(rnd, sizeof(rnd))
        .Finalize(hash.begin());
    std::vector<uint8_t> vchSig;
    SignECDSA(hash, vchSig);
    return pubkey.VerifyECDSA(hash, vchSig);
}

bool CKey::SignCompact(const uint256 &hash,
                       std::vector<uint8_t> &vchSig) const {
    if (!fValid) {
        return false;
    }
    vchSig.resize(CPubKey::COMPACT_SIGNATURE_SIZE);
    int rec = -1;
    secp256k1_ecdsa_recoverable_signature sig;
    int ret = secp256k1_ecdsa_sign_recoverable(
        secp256k1_context_sign, &sig, hash.begin(), begin(),
        secp256k1_nonce_function_rfc6979, nullptr);
    assert(ret);
    secp256k1_ecdsa_recoverable_signature_serialize_compact(
        secp256k1_context_sign, &vchSig[1], &rec, &sig);
    assert(ret);
    assert(rec != -1);
    vchSig[0] = 27 + rec + 4;
    return true;
}

bool CKey::Load(const CPrivKey &privkey, const CPubKey &vchPubKey,
                bool fSkipCheck = false) {
    if (!ec_privkey_import_der(secp256k1_context_sign, (uint8_t *)begin(),
                               privkey.data(), privkey.size()))
        return false;
    fValid = true;

    if (fSkipCheck) {
        return true;
    }

    return VerifyPubKey(vchPubKey);
}

bool CKey::Derive(CKey &keyChild, ChainCode &ccChild, unsigned int nChild,
                  const ChainCode &cc) const {
    assert(IsValid());
    std::vector<uint8_t, secure_allocator<uint8_t>> vout(64);
    if ((nChild >> 31) == 0) {
        CPubKey pubkey = GetPubKey();
        assert(pubkey.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE);
        BIP32Hash(cc, nChild, *pubkey.begin(), pubkey.begin() + 1, vout.data());
    } else {
        assert(size() == 32);
        BIP32Hash(cc, nChild, 0, begin(), vout.data());
    }
    memcpy(ccChild.begin(), vout.data() + 32, 32);
    memcpy((uint8_t *)keyChild.begin(), begin(), 32);
    bool ret = secp256k1_ec_privkey_tweak_add(
        secp256k1_context_sign, (uint8_t *)keyChild.begin(), vout.data());
    keyChild.fValid = ret;
    return ret;
}

bool CExtKey::Derive(CExtKey &out, unsigned int _nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id = key.GetPubKey().GetKeyID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = _nChild;
    return key.Derive(out.key, out.chaincode, _nChild, chaincode);
}

void CExtKey::SetMaster(const uint8_t *seed, unsigned int nSeedLen) {
    static const uint8_t hashkey[] = {'B', 'i', 't', 'c', 'o', 'i',
                                      'n', ' ', 's', 'e', 'e', 'd'};
    std::vector<uint8_t, secure_allocator<uint8_t>> vout(64);
    CHMAC_SHA512(hashkey, sizeof(hashkey))
        .Write(seed, nSeedLen)
        .Finalize(vout.data());
    key.Set(vout.data(), vout.data() + 32);
    memcpy(chaincode.begin(), vout.data() + 32, 32);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
}

CExtPubKey CExtKey::Neuter() const {
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    ret.chaincode = chaincode;
    return ret;
}

void CExtKey::Encode(uint8_t code[BIP32_EXTKEY_SIZE]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, chaincode.begin(), 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code + 42, key.begin(), 32);
}

void CExtKey::Decode(const uint8_t code[BIP32_EXTKEY_SIZE]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(chaincode.begin(), code + 9, 32);
    key.Set(code + 42, code + BIP32_EXTKEY_SIZE);
}

bool ECC_InitSanityCheck() {
    CKey key;
    key.MakeNewKey();
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}

void ECC_Start() {
    assert(secp256k1_context_sign == nullptr);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    assert(ctx != nullptr);

    {
        // Pass in a random blinding seed to the secp256k1 context.
        std::vector<uint8_t, secure_allocator<uint8_t>> vseed(32);
        GetRandBytes(vseed.data(), 32);
        bool ret = secp256k1_context_randomize(ctx, vseed.data());
        assert(ret);
    }

    secp256k1_context_sign = ctx;
}

void ECC_Stop() {
    secp256k1_context *ctx = secp256k1_context_sign;
    secp256k1_context_sign = nullptr;

    if (ctx) {
        secp256k1_context_destroy(ctx);
    }
}
