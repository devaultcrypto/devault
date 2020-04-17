// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/crypter.h>

#include <crypto/aes.h>
#include <crypto/sha512.h>
#include <util/system.h>

#include <vector>

int CCrypter::BytesToKeySHA512AES(const std::vector<uint8_t> &chSalt,
                                  const SecureString &strKeyData, int count,
                                  uint8_t *key, uint8_t *iv) const {
    // This mimics the behavior of openssl's EVP_BytesToKey with an aes256cbc
    // cipher and sha512 message digest. Because sha512's output size (64b) is
    // greater than the aes256 block size (16b) + aes256 key size (32b), there's
    // no need to process more than once (D_0).
    if (!count || !key || !iv) {
        return 0;
    }

    uint8_t buf[CSHA512::OUTPUT_SIZE];
    CSHA512 di;

    di.Write((const uint8_t *)strKeyData.c_str(), strKeyData.size());
    if (chSalt.size()) {
        di.Write(&chSalt[0], chSalt.size());
    }
    di.Finalize(buf);

    for (int i = 0; i != count - 1; i++) {
        di.Reset().Write(buf, sizeof(buf)).Finalize(buf);
    }

    memcpy(key, buf, WALLET_CRYPTO_KEY_SIZE);
    memcpy(iv, buf + WALLET_CRYPTO_KEY_SIZE, WALLET_CRYPTO_IV_SIZE);
    memory_cleanse(buf, sizeof(buf));
    return WALLET_CRYPTO_KEY_SIZE;
}

bool CCrypter::SetKeyFromPassphrase(const SecureString &strKeyData,
                                    const std::vector<uint8_t> &chSalt,
                                    const unsigned int nRounds,
                                    const unsigned int nDerivationMethod) {
    if (nRounds < 1 || chSalt.size() != WALLET_CRYPTO_SALT_SIZE) {
        return false;
    }

    int i = 0;
    if (nDerivationMethod == 0) {
        i = BytesToKeySHA512AES(chSalt, strKeyData, nRounds, vchKey.data(),
                                vchIV.data());
    }

    if (i != (int)WALLET_CRYPTO_KEY_SIZE) {
        memory_cleanse(vchKey.data(), vchKey.size());
        memory_cleanse(vchIV.data(), vchIV.size());
        return false;
    }

    fKeySet = true;
    return true;
}

bool CCrypter::SetKey(const CKeyingMaterial &chNewKey,
                      const std::vector<uint8_t> &chNewIV) {
    if (chNewKey.size() != WALLET_CRYPTO_KEY_SIZE ||
        chNewIV.size() != WALLET_CRYPTO_IV_SIZE) {
        return false;
    }

    memcpy(vchKey.data(), chNewKey.data(), chNewKey.size());
    memcpy(vchIV.data(), chNewIV.data(), chNewIV.size());

    fKeySet = true;
    return true;
}

bool CCrypter::Encrypt(const CKeyingMaterial &vchPlaintext,
                       std::vector<uint8_t> &vchCiphertext) const {
    if (!fKeySet) {
        return false;
    }

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCKSIZE bytes
    vchCiphertext.resize(vchPlaintext.size() + AES_BLOCKSIZE);

    AES256CBCEncrypt enc(vchKey.data(), vchIV.data(), true);
    size_t nLen =
        enc.Encrypt(&vchPlaintext[0], vchPlaintext.size(), &vchCiphertext[0]);
    if (nLen < vchPlaintext.size()) {
        return false;
    }
    vchCiphertext.resize(nLen);

    return true;
}

bool CCrypter::Decrypt(const std::vector<uint8_t> &vchCiphertext,
                       CKeyingMaterial &vchPlaintext) const {
    if (!fKeySet) {
        return false;
    }

    // plaintext will always be equal to or lesser than length of ciphertext
    int nLen = vchCiphertext.size();

    vchPlaintext.resize(nLen);

    AES256CBCDecrypt dec(vchKey.data(), vchIV.data(), true);
    nLen =
        dec.Decrypt(&vchCiphertext[0], vchCiphertext.size(), &vchPlaintext[0]);
    if (nLen == 0) {
        return false;
    }
    vchPlaintext.resize(nLen);
    return true;
}

static bool EncryptSecret(const CKeyingMaterial &vMasterKey,
                          const CKeyingMaterial &vchPlaintext,
                          const uint256 &nIV,
                          std::vector<uint8_t> &vchCiphertext) {
    CCrypter cKeyCrypter;
    std::vector<uint8_t> chIV(WALLET_CRYPTO_IV_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_IV_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV)) {
        return false;
    }
    return cKeyCrypter.Encrypt(*((const CKeyingMaterial *)&vchPlaintext),
                               vchCiphertext);
}

static bool DecryptSecret(const CKeyingMaterial &vMasterKey,
                          const std::vector<uint8_t> &vchCiphertext,
                          const uint256 &nIV, CKeyingMaterial &vchPlaintext) {
    CCrypter cKeyCrypter;
    std::vector<uint8_t> chIV(WALLET_CRYPTO_IV_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_IV_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV)) {
        return false;
    }
    return cKeyCrypter.Decrypt(vchCiphertext,
                               *((CKeyingMaterial *)&vchPlaintext));
}

bool CCryptoKeyStore::IsLocked() const {
    LOCK(cs_KeyStore);
    return vMasterKey.empty();
}

bool CCryptoKeyStore::Lock() {
    {
        LOCK(cs_KeyStore);
        if (!gArgs.GetBoolArg("-bypasspassword",false))  vMasterKey.clear();
    }

    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial &vMasterKeyIn) {
    {
        LOCK(cs_KeyStore);
        vMasterKey = vMasterKeyIn;
        if(!cryptedHDChain.IsNull()) {
          bool chainPass = false;
          // try to decrypt seed and make sure it matches
          CHDChain hdChainTmp;
          if (DecryptHDChain(hdChainTmp)) {
            // make sure seed matches this chain
            chainPass = cryptedHDChain.GetID() == hdChainTmp.GetSeedHash();
          }
          if (!chainPass) {
            vMasterKey.clear();
            return false;
          }
        }
        fDecryptionThoroughlyChecked = true;
    }
    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const {
    {
        LOCK(cs_KeyStore);
        // Check for watch-only pubkeys
        return CBasicKeyStore::GetPubKey(address, vchPubKeyOut);
    }
}
bool CCryptoKeyStore::GetPubKey(const BKeyID &address, CPubKey &vchPubKeyOut) const {
    {
        LOCK(cs_KeyStore);
        // Check for watch-only pubkeys
        return CBasicKeyStore::GetPubKey(address, vchPubKeyOut);
    }
}


bool CCryptoKeyStore::EncryptHDChain(const CKeyingMaterial& vMasterKeyIn, const CHDChain& hdc)
{  
  CHDChain hdChain = hdc;
  std::vector<unsigned char> vchCryptedSeed;
  if (!EncryptSecret(vMasterKeyIn, hdChain.GetSeed(), hdChain.GetID(), vchCryptedSeed))
    return false;
  
  cryptedHDChain = hdChain; // Will preserve the ID
  cryptedHDChain.SetCrypted(true);
  
  SecureString strMnemonic;
  std::vector<unsigned char> vchCryptedMnemonic;
  hdChain.GetMnemonic(strMnemonic);
  SecureVector vchMnemonic(strMnemonic.begin(), strMnemonic.end());
  
  if (!vchMnemonic.empty() && !EncryptSecret(vMasterKeyIn, vchMnemonic, hdChain.GetID(), vchCryptedMnemonic))
    return false;
  
  // Convert to SecureVectors for SetupCrypted
  SecureVector vchSecureCryptedMnemonic(vchCryptedMnemonic.begin(), vchCryptedMnemonic.end());
  SecureVector vchSecureCryptedSeed(vchCryptedSeed.begin(), vchCryptedSeed.end());
  
  cryptedHDChain.SetupCrypted(vchSecureCryptedMnemonic, vchSecureCryptedSeed);
  
  return true;
}

bool CCryptoKeyStore::DecryptHDChain(CHDChain& hdChainRet) const
{
    if (cryptedHDChain.IsNull())
        return false;

    if (!cryptedHDChain.IsCrypted())
        return false;

    SecureVector vchSecureSeed;
    SecureVector vchSecureCryptedSeed = cryptedHDChain.GetSeed();
    std::vector<unsigned char> vchCryptedSeed(vchSecureCryptedSeed.begin(), vchSecureCryptedSeed.end());
    if (!DecryptSecret(vMasterKey, vchCryptedSeed, cryptedHDChain.GetID(), vchSecureSeed))
        return false;

    hdChainRet = cryptedHDChain;
    if (!hdChainRet.SetSeed(vchSecureSeed, false))
        return false;

    // hash of decrypted seed must match chain id
    if (hdChainRet.GetSeedHash() != cryptedHDChain.GetID())
        return false;

    SecureVector vchSecureCryptedMnemonic;
    if (cryptedHDChain.GetMnemonic(vchSecureCryptedMnemonic)) {
      SecureVector vchSecureMnemonic;
      std::vector<unsigned char> vchCryptedMnemonic(vchSecureCryptedMnemonic.begin(), vchSecureCryptedMnemonic.end());
      if (!vchCryptedMnemonic.empty() && !DecryptSecret(vMasterKey, vchCryptedMnemonic, cryptedHDChain.GetID(), vchSecureMnemonic))
        return false;
      hdChainRet.SetMnemonic(vchSecureMnemonic);
    }
  
    hdChainRet.SetCrypted(false);

    return true;
}


bool CCryptoKeyStore::SetCryptedHDChain(const CHDChain& chain)
{
    if (!chain.IsCrypted())
        return false;

    cryptedHDChain = chain;
    return true;
}
bool CCryptoKeyStore::GetCryptedHDChain(CHDChain& hdChainRet) const
{
  hdChainRet = cryptedHDChain;
  return !cryptedHDChain.IsNull();
}

bool CCryptoKeyStore::GetDecryptedHDChain(CHDChain& hdChainRet) const
{
  hdChainRet = cryptedHDChain;
  if (!DecryptHDChain(hdChainRet)) {
      return false;
  }
  return !cryptedHDChain.IsNull();
}
