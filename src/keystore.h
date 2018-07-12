// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>

/** A virtual base class for key stores */
class CKeyStore {
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() = default;

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) = 0;
    virtual bool AddKey(const CKey &key);
    virtual bool RemoveKey(const CKey &key) = 0; // For temporary addition

    //! Check whether a key corresponding to a given address is present in the
    //! store.
    virtual bool HaveKey(const CKeyID &address) const = 0;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const = 0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const = 0;
    
    virtual bool HaveKey(const BKeyID &address) const = 0;
    virtual bool GetKey(const BKeyID &address, CKey &keyOut) const = 0;
    virtual bool GetPubKey(const BKeyID &address, CPubKey &vchPubKeyOut) const = 0;

    //! Support for BIP 0013 : see
    //! https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript &redeemScript) = 0;
    virtual bool HaveCScript(const CScriptID &hash) const = 0;
    virtual std::set<CScriptID> GetCScripts() const = 0;
    virtual bool GetCScript(const CScriptID &hash,
                            CScript &redeemScriptOut) const = 0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript &dest) = 0;
    virtual bool RemoveWatchOnly(const CScript &dest) = 0;
    virtual bool HaveWatchOnly(const CScript &dest) const = 0;
    virtual bool HaveWatchOnly() const = 0;
};

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore {
protected:
    mutable CCriticalSection cs_KeyStore;

    using KeyMap = std::map<CKeyID, CKey>;
    using BLSKeyMap = std::map<BKeyID, CKey>;
    using WatchKeyMap = std::map<CKeyID, CPubKey>;
    using WatchBLSKeyMap = std::map<BKeyID, CPubKey>;
    using ScriptMap = std::map<CScriptID, CScript>;
    using WatchOnlySet = std::set<CScript>;

    KeyMap mapKeys GUARDED_BY(cs_KeyStore);
    BLSKeyMap mapBLSKeysTemp GUARDED_BY(cs_KeyStore);
    WatchKeyMap mapWatchKeys GUARDED_BY(cs_KeyStore);
    WatchBLSKeyMap mapBLSWatchKeys GUARDED_BY(cs_KeyStore);
    ScriptMap mapScripts GUARDED_BY(cs_KeyStore);
    WatchOnlySet setWatchOnly GUARDED_BY(cs_KeyStore);

    void ImplicitlyLearnRelatedKeyScripts(const CPubKey &pubkey)
        EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

public:
    bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) override;
    bool RemoveKey(const CKey &key) override; // For temporary addition
    bool GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const override;
    bool GetPubKey(const BKeyID &address, CPubKey &vchPubKeyOut) const override;
    bool HaveKey(const CKeyID &address) const override;
    bool HaveKey(const BKeyID &address) const override;
    bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    bool GetKey(const BKeyID &address, CKey &keyOut) const override;
    
    bool AddCScript(const CScript &redeemScript) override;
    bool HaveCScript(const CScriptID &hash) const override;
    std::set<CScriptID> GetCScripts() const override;
    bool GetCScript(const CScriptID &hash, CScript &redeemScriptOut) const override;

    bool AddWatchOnly(const CScript &dest) override;
    bool RemoveWatchOnly(const CScript &dest) override;
    bool HaveWatchOnly(const CScript &dest) const override;
    bool HaveWatchOnly() const override;

};

typedef std::vector<uint8_t, secure_allocator<uint8_t>> CKeyingMaterial;
