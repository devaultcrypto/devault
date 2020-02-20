// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <keyid.h>

#include <boost/range/adaptor/sliced.hpp>

#include <stdexcept>
#include <vector>

typedef uint256 ChainCode;

static constexpr unsigned int BLS_ACCOUNT = 1; // 0 would mean secret keys same as default EC Account/Keys


/** An encapsulated public key. */
class CPubKey {
public:
    static constexpr unsigned int BLS_SIGNATURE_SIZE = 96;
    static constexpr unsigned int BLS_PUBLIC_KEY_SIZE = 48;
    
    // secp256k1:
    static constexpr unsigned int COMPRESSED_PUBLIC_KEY_SIZE = 33;
    static constexpr unsigned int SIGNATURE_SIZE = 72;
    static constexpr unsigned int COMPACT_SIGNATURE_SIZE = 65;
    /**
     * see www.keylength.com
     * script supports up to 75 for single byte push
     */

private:
    /**
     * Just store the serialized data.
     * Its length can very cheaply be computed from the first byte.
     */
    uint8_t vch[BLS_PUBLIC_KEY_SIZE]; // since > COMPRESSED_PUBLIC_KEY_SIZE
    uint8_t _size;

    //! Set this key data to be invalid
    void Invalidate() { _size = 0; }

public:
    bool static ValidSize(const std::vector<uint8_t> &vch) {
        return vch.size() == COMPRESSED_PUBLIC_KEY_SIZE || vch.size() == BLS_PUBLIC_KEY_SIZE;
    }

    //! Construct an invalid public key.
    CPubKey() { Invalidate(); }

    //! Initialize a public key using begin/end iterators to byte data.
    template <typename T> void Set(const T pbegin, const T pend) {
        _size = pend - pbegin;
        if (_size <= BLS_PUBLIC_KEY_SIZE) {
            memcpy(vch, (uint8_t *)&pbegin[0], _size);
        }
    }

    //! Construct a public key using begin/end iterators to byte data.
    template <typename T> CPubKey(const T pbegin, const T pend) {
        Set(pbegin, pend);
    }

    //! Construct a public key from a byte vector.
    explicit CPubKey(const std::vector<uint8_t> &_vch) {
        Set(_vch.begin(), _vch.end());
    }

    //! Simple read-only vector-like interface to the pubkey data.
    unsigned int size() const { return _size; }
    const uint8_t *begin() const { return vch; }
    const uint8_t *end() const { return vch + size(); }
    const uint8_t &operator[](unsigned int pos) const { return vch[pos]; }

    //! Comparator implementation.
    friend bool operator==(const CPubKey &a, const CPubKey &b) {
        return a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) == 0;
    }
    friend bool operator!=(const CPubKey &a, const CPubKey &b) {
        return !(a == b);
    }
    friend bool operator<(const CPubKey &a, const CPubKey &b) {
        return a.vch[0] < b.vch[0] ||
               (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) < 0);
    }

    //! Implement serialization, as if this was a byte vector.
    template <typename Stream> void Serialize(Stream &s) const {
        unsigned int len = size();
        ::WriteCompactSize(s, len);
        s.write((char *)vch, len);
    }
    template <typename Stream> void Unserialize(Stream &s) {
        unsigned int len = ::ReadCompactSize(s);
       _size = len;
        if (len == COMPRESSED_PUBLIC_KEY_SIZE || len == BLS_PUBLIC_KEY_SIZE) {
            s.read((char *)vch, len);
        }
    }

    //! Get the KeyID of this public key (hash of its serialization)
    CKeyID GetKeyID() const { return CKeyID(Hash160(vch, vch + size())); }
    BKeyID GetBLSKeyID() const { return BKeyID(Hash160(vch, vch + size())); }

    //! Get the 256-bit hash of this public key.
    uint256 GetHash() const { return Hash(vch, vch + size()); }

    /*
     * Check syntactic correctness.
     *
     * Note that this is consensus critical as CheckSig() calls it!
     */
    bool IsValid() const { return size() == COMPRESSED_PUBLIC_KEY_SIZE || size() == BLS_PUBLIC_KEY_SIZE; }

    //! fully validate whether this is a valid public key (more expensive than
    //! IsValid())
    bool IsFullyValid() const;

    //! Check whether this is a compressed public key.
    bool IsCompressed() const { return size() == COMPRESSED_PUBLIC_KEY_SIZE; }
    bool HasCompressedByte() const { return vch[0] == 2 || vch[0] == 3; }
    void SetSize(int s) { _size = s; }
    bool IsEC() const { return size() == COMPRESSED_PUBLIC_KEY_SIZE; }
    bool IsBLS() const { return size() == BLS_PUBLIC_KEY_SIZE; }

    /**
     * Verify a DER-serialized ECDSA signature (~72 bytes).
     * If this public key is not fully valid, the return value will be false.
     */
    bool VerifyECDSA(const uint256 &hash,
                     const std::vector<uint8_t> &vchSig) const;

    /* Not yet Ready since vch[] is too small currently */
    bool VerifyBLS(const uint256 &hash,
                   const std::vector<uint8_t> &vchSig) const;

    /**
     * Check whether a DER-serialized ECDSA signature is normalized (lower-S).
     */
    static bool
    CheckLowS(const boost::sliced_range<const std::vector<uint8_t>> &vchSig);
    static bool CheckLowS(const std::vector<uint8_t> &vchSig) {
        return CheckLowS(vchSig | boost::adaptors::sliced(0, vchSig.size()));
    }

    //! Recover a public key from a compact ECDSA signature.
    bool RecoverCompact(const uint256 &hash,
                        const std::vector<uint8_t> &vchSig);

    //! Derive BIP32 child pubkey.
    bool Derive(CPubKey &pubkeyChild, ChainCode &ccChild, unsigned int nChild,
                const ChainCode &cc) const;
};

/**
 * Users of this module must hold an ECCVerifyHandle. The constructor and
 * destructor of these are not allowed to run in parallel, though.
 */
class ECCVerifyHandle {
    static int refcount;

public:
    ECCVerifyHandle();
    ~ECCVerifyHandle();
};

