// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <pubkey.h>
#include <serialize.h>
#include <support/allocators/secure.h>
#include <uint256.h>

#include <stdexcept>
#include <vector>

/**
 * secure_allocator is defined in allocators.h
 * CPrivKey is a serialized private key, with all parameters included (COMPRESSED_PRIVATE_KEY_SIZE bytes)
 */
typedef std::vector<uint8_t, secure_allocator<uint8_t>> CPrivKey;

/** An encapsulated secp256k1 private key - always compressed. */
class CKey {
public:
    /**
     * secp256k1:
     */
    static const unsigned int COMPRESSED_PRIVATE_KEY_SIZE = 214;

private:
    //! Whether this private key is valid. We check for correctness when
    //! modifying the key data, so fValid should always correspond to the actual
    //! state.
    bool fValid;

    //! The actual byte data
    std::vector<uint8_t, secure_allocator<uint8_t>> keydata;

    //! Check whether the 32-byte array pointed to by vch is valid keydata.
    static bool Check(const uint8_t *vch);

public:
    //! Construct an invalid private key.
    CKey() : fValid(false) {
        // Important: vch must be 32 bytes in length to not break serialization
        keydata.resize(32);
    }

    friend bool operator==(const CKey &a, const CKey &b) {
        return a.size() == b.size() &&
               memcmp(a.keydata.data(), b.keydata.data(), a.size()) == 0;
    }

    //! Initialize using begin and end iterators to byte data.
    template <typename T>
    void Set(const T pbegin, const T pend) {
        if (size_t(pend - pbegin) != keydata.size()) {
            fValid = false;
        } else if (Check(&pbegin[0])) {
            memcpy(keydata.data(), (uint8_t *)&pbegin[0], keydata.size());
            fValid = true;
        } else {
            fValid = false;
        }
    }

    //! Simple read-only vector-like interface.
    unsigned int size() const { return (fValid ? keydata.size() : 0); }
    const uint8_t *begin() const { return keydata.data(); }
    const uint8_t *end() const { return keydata.data() + size(); }

    //! Check whether this private key is valid.
    bool IsValid() const { return fValid; }

    //! Generate a new private key using a cryptographic PRNG.
    void MakeNewKey();

    /**
     * Compute the public key from a private key.
     * This is expensive.
     */
    CPubKey GetPubKey() const;
    CPubKey GetPubKeyForBLS() const;
    
    /**
     * Create a DER-serialized ECDSA signature.
     * The test_case parameter tweaks the deterministic nonce.
     */
    bool SignECDSA(const uint256 &hash, std::vector<uint8_t> &vchSig,
                   uint32_t test_case = 0) const;

    /**
     * Create a BLS signature.
     */
    bool SignBLS(const uint256 &hash, std::vector<uint8_t> &vchSig) const;
    //bool SignBLS(const uint256 &hash, bls::Signature& sig) const;

    /**
     * Create a compact ECDSA signature (65 bytes), which allows reconstructing
     * the used public key.
     * The format is one header byte, followed by two times 32 bytes for the
     * serialized r and s values.
     * The header byte: 0x1B = first key with even y, 0x1C = first key with odd
     * y,
     *                  0x1D = second key with even y, 0x1E = second key with
     * odd y,
     *                  add 0x04 for compressed keys.
     */
    bool SignCompact(const uint256 &hash, std::vector<uint8_t> &vchSig) const;

    //! Derive BIP32 child key.
    bool Derive(CKey &keyChild, ChainCode &ccChild, unsigned int nChild,
                const ChainCode &cc) const;

    /**
     * Verify thoroughly whether a private key and a public key match.
     * This is done using a different mechanism than just regenerating it.
     * (An ECDSA signature is created then verified.)
     */
    bool VerifyPubKey(const CPubKey &vchPubKey) const;

    //! Load private key and check that public key matches.
    bool Load(const CPrivKey &privkey, const CPubKey &vchPubKey,
              bool fSkipCheck);
};


/**
 * Initialize the elliptic curve support. May not be called twice without
 * calling ECC_Stop first.
 */
void ECC_Start(void);

/**
 * Deinitialize the elliptic curve support. No-op if ECC_Start wasn't called
 * first.
 */
void ECC_Stop(void);

/** Check that required EC support is available at runtime. */
bool ECC_InitSanityCheck(void);

#endif // BITCOIN_KEY_H
