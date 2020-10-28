// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <keystore.h>

#include <key.h>
#include <pubkey.h>
#include <util/system.h>

bool CKeyStore::AddKey(const CKey &key) {
    return AddKeyPubKey(key, key.GetPubKey());
}

// Remove key temporarily added for Sweep function
bool CBasicKeyStore::RemoveKey(const CKey& key) {
    if (mapKeys.count(key.GetPubKey().GetKeyID())) {
        return mapKeys.erase(key.GetPubKey().GetKeyID());
    } else if (mapBLSKeysTemp.count(key.GetPubKeyForBLS().GetBLSKeyID())) {
        return mapBLSKeysTemp.erase(key.GetPubKeyForBLS().GetBLSKeyID());
    }
    return false;
}

void CBasicKeyStore::ImplicitlyLearnRelatedKeyScripts(const CPubKey &pubkey) {
    AssertLockHeld(cs_KeyStore);
    auto key_id = pubkey.GetBLSKeyID();
    // We must actually know about this key already.
    assert(HaveKey(key_id) || mapBLSWatchKeys.count(key_id));
    // This adds the redeemscripts necessary to detect alternative outputs using
    // the same keys. Also note that having superfluous scripts in the keystore
    // never hurts. They're only used to guide recursion in signing and IsMine
    // logic - if a script is present but we can't do anything with it, it has
    // no effect. "Implicitly" refers to fact that scripts are derived
    // automatically from existing keys, and are present in memory, even without
    // being explicitly loaded (e.g. from a file).

    // Right now there are none so do nothing.
}

bool CBasicKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const {
    CKey key;
    if (!GetKey(address, key)) {
        LOCK(cs_KeyStore);
        auto it = mapWatchKeys.find(address);
        if (it != mapWatchKeys.end()) {
            vchPubKeyOut = it->second;
            return true;
        }
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}
bool CBasicKeyStore::GetPubKey(const BKeyID &address, CPubKey &vchPubKeyOut) const {
    CKey key;
    if (!GetKey(address, key)) {
        LOCK(cs_KeyStore);
        auto it = mapBLSWatchKeys.find(address);
        if (it != mapBLSWatchKeys.end()) {
            vchPubKeyOut = it->second;
            return true;
        }
        return false;
    }
    vchPubKeyOut = key.GetPubKeyForBLS();
    return true;
}

bool CBasicKeyStore::AddKeyPubKey(const CKey &key, const CPubKey &pubkey) {
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetKeyID()] = key;
    mapBLSKeysTemp[pubkey.GetBLSKeyID()] = key;
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}


bool CBasicKeyStore::HaveKey(const CKeyID &address) const {
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}

bool CBasicKeyStore::HaveKey(const BKeyID &address) const {
    LOCK(cs_KeyStore);
    return mapBLSKeysTemp.count(address);
}

bool CBasicKeyStore::GetKey(const CKeyID &address, CKey &keyOut) const {
    LOCK(cs_KeyStore);
    auto mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::GetKey(const BKeyID &address, CKey &keyOut) const {
    LOCK(cs_KeyStore);
    auto bi = mapBLSKeysTemp.find(address);
    if (bi != mapBLSKeysTemp.end()) {
        keyOut = bi->second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::AddCScript(const CScript &redeemScript) {
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        return error("CBasicKeyStore::AddCScript(): redeemScripts > %i bytes "
                     "are invalid",
                     MAX_SCRIPT_ELEMENT_SIZE);
    }

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID &hash) const {
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

std::set<CScriptID> CBasicKeyStore::GetCScripts() const {
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto &mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
}

bool CBasicKeyStore::GetCScript(const CScriptID &hash,
                                CScript &redeemScriptOut) const {
    LOCK(cs_KeyStore);
    auto mi = mapScripts.find(hash);
    if (mi != mapScripts.end()) {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

static bool ExtractPubKey(const CScript &dest, CPubKey &pubKeyOut) {
    // TODO: Use Solver to extract this?
    CScript::const_iterator pc = dest.begin();
    opcodetype opcode;
    std::vector<uint8_t> vch;
    if (!dest.GetOp(pc, opcode, vch) || !CPubKey::ValidSize(vch)) {
        return false;
    }
    pubKeyOut = CPubKey(vch);
    if (!pubKeyOut.IsFullyValid()) {
        return false;
    }
    if (!dest.GetOp(pc, opcode, vch) || opcode != OP_CHECKSIG ||
        dest.GetOp(pc, opcode, vch)) {
        return false;
    }
    return true;
}

bool CBasicKeyStore::AddWatchOnly(const CScript &dest) {
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        if (pubKey.IsEC()) mapWatchKeys[pubKey.GetKeyID()] = pubKey;
        else mapBLSWatchKeys[pubKey.GetBLSKeyID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest) {
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        if (pubKey.IsEC()) mapWatchKeys.erase(pubKey.GetKeyID();
        else mapBLSWatchKeys.erase(pubKey.GetBLSKeyID());

        mapWatchKeys.erase(pubKey.GetKeyID());
    }
    // Related CScripts are not removed; having superfluous scripts around is
    // harmless (see comment in ImplicitlyLearnRelatedKeyScripts).
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript &dest) const {
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const {
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}
