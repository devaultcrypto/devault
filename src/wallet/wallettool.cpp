// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <dstencode.h>
#include <fs.h>
#include <util/time.h>
#include <util/splitstring.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace WalletTool {

static std::shared_ptr<CWallet> LoadWallet(const fs::path &wallet_path) {
  if (!fs::exists(wallet_path)) {
    tfm::format(std::cerr, "Error: Wallet files does not exist\n");
    return nullptr;
  }

  std::unique_ptr<WalletDatabase> database(new WalletDatabase(wallet_path));
  auto &config = const_cast<Config &>(GetConfig());
  auto &chainParams = config.GetChainParams();
  // dummy chain interface
  std::shared_ptr<CWallet> wallet_instance = std::make_unique<CWallet>(chainParams, WalletLocation(wallet_path),
                                                                       std::move(database));
  DBErrors load_wallet_ret;
  try {
    bool first_run;
    load_wallet_ret = wallet_instance->LoadWallet(first_run);
  } catch (const std::runtime_error &) {
    tfm::format(std::cerr, "Error loading %s. Is wallet being used by another process?\n", wallet_path.string());
    return nullptr;
  }

  if (load_wallet_ret != DBErrors::LOAD_OK) {
    wallet_instance = nullptr;
    if (load_wallet_ret == DBErrors::CORRUPT) {
      tfm::format(std::cerr, "Error loading %s: Wallet corrupted", wallet_path.string());
      return nullptr;
    } else if (load_wallet_ret == DBErrors::NONCRITICAL_ERROR) {
      tfm::format(std::cerr,
                  "Error reading %s! All keys read correctly, but transaction data"
                  " or address book entries might be missing or incorrect.",
                  wallet_path.string());
    } else if (load_wallet_ret == DBErrors::TOO_NEW) {
      tfm::format(std::cerr, "Error loading %s: Wallet requires newer version of DeVault Core", wallet_path.string());
      return nullptr;
    } else if (load_wallet_ret == DBErrors::NEED_REWRITE) {
      tfm::format(std::cerr, "Wallet needed to be rewritten: restart DeVault Core to complete");
      return nullptr;
    } else {
      tfm::format(std::cerr, "Error loading %s", wallet_path.string());
      return nullptr;
    }
  }

  return wallet_instance;
}

static bool unlockWallet(CWallet *wallet_instance) {
  std::cout << "\n\nEnter the Wallet Encryption password to decrypt. Enter return if you've used blank password or you don't have blank but want to skip decrypt. Failed password will not decrypt seed phrase/seed\n";
  SecureString pass1 = "";
  int char_count = 0; // This is used to handle Ctrl-C which would create
  // an infinite loop and crash below otherwise
  char c = '0';
  const int min_char_count = 2 * 4 + 1;
  do {
    do {
      std::cin.get(c);
      char_count++;
      if (char_count++ > 81) {
        // Don't print message, just exit because it can be due to Ctrl-C
        std::cout << "problem with password\n";
        return false;
      }
      if (c != '\n') pass1.push_back(c);
    } while (c != '\n');
    if (char_count < min_char_count) {
        if (wallet_instance->Unlock(pass1)) {
            std::cout << "Blank password works, will decrypt\n";
            return true;
        } else {
            std::cout << "Password too short, don't decrypt\n";
            return false;
        }
    }
  } while (char_count < min_char_count);
  wallet_instance->Unlock(pass1);
  return true;
}

static void WalletShowInfo(CWallet *wallet_instance) {
  LOCK(wallet_instance->cs_wallet);

  tfm::format(std::cout, "Wallet info\n===========\n");
  tfm::format(std::cout, "Encrypted: yes\n");

  assert(wallet_instance->mapMasterKeys.size() > 0);
  
  int i = 0;
  for (const auto &m : wallet_instance->mapMasterKeys) {
    tfm::format(std::cout, "[%d] CMasterKey: Crypted = %s, Salt = %s, Method = %d, Iterations = %d\n", i++,
                HexStr(m.second.vchCryptedKey), HexStr(m.second.vchSalt), m.second.nDerivationMethod,
                m.second.nDeriveIterations);
  }
  

  bool decrypt = unlockWallet(wallet_instance);

  if (!decrypt || wallet_instance->Unlock("")) {
    i = 0;
    for (const auto &m : wallet_instance->mapAddressBook) {
      tfm::format(std::cout, "[%d] Address: %s, purpose: %s, name: %s\n", i++, EncodeDestination(m.first),
                  m.second.purpose, m.second.name);
    }

    // Put in a sorted KeyPath order for easier comparison
    std::map<uint64_t,std::string> sortedKeys;
    for (const auto &m : wallet_instance->mapHdPubKeys) {
      std::string fullstr = EncodeDestination(m.first);

      // std::string stime = FormatISO8601DateTime(wallet_instance->mapKeyMetadata[m.first].nCreateTime);
      
      // This is to make the output keys sorted by path
      // Put into a std::map based on numeric path that will be stored in sorted order
      std::vector<std::string> vParts;
      Split(vParts, m.second.GetKeyPath(), "/");
      uint64_t keynum = std::stoi(vParts.back());
      vParts.pop_back();
      uint64_t ext = std::stoi(vParts.back());
      uint64_t order = ext*100000+keynum;
      fullstr += strprintf(" # hdkeypath=%s", m.second.GetKeyPath());
      sortedKeys.insert(make_pair(order,fullstr));
    }

    for (const auto &m : sortedKeys) tfm::format(std::cout, "HDPubKey % s\n",m.second);

    // Clear so we can print a divider
    sortedKeys.clear();
    std::cout << "************************\n";
    
    for (const auto &m : wallet_instance->mapBLSPubKeys) {
         std::string fullstr = EncodeDestination(m.first);
         // This is to make the output keys sorted by path
         // Put into a std::map based on numeric path that will be stored in sorted order
         std::vector<std::string> vParts;
         Split(vParts, m.second.GetKeyPath(), "/");
         uint64_t keynum = std::stoi(vParts.back());
         vParts.pop_back();
         uint64_t ext = std::stoi(vParts.back());
         uint64_t order = ext*100000+keynum;
         fullstr += strprintf(" # hdkeypath=%s", m.second.GetKeyPath());
         sortedKeys.insert(make_pair(order,fullstr));
    }

    for (const auto &m : sortedKeys) tfm::format(std::cout, "HDPubKey % s\n",m.second);
    std::cout << "************************\n";

    i = 0;
    for (const auto &m : wallet_instance->mapMasterKeys) {
      tfm::format(std::cout, "[%d] CMasterKey: Crypted = %s, Salt = %s, Method = %d, Iterations = %d\n", i++,
                  HexStr(m.second.vchCryptedKey), HexStr(m.second.vchSalt), m.second.nDerivationMethod,
                  m.second.nDeriveIterations);
    }
  }

  tfm::format(std::cout, "Keypool Size: %u\n", wallet_instance->GetKeyPoolSize());
  tfm::format(std::cout, "Transactions: %zu\n", wallet_instance->mapWallet.size());
  tfm::format(std::cout, "Address Book: %zu\n", wallet_instance->mapAddressBook.size());
  tfm::format(std::cout, "Name: %s\n", wallet_instance->GetName());
  tfm::format(std::cout, "Wallet Version: %s\n", wallet_instance->GetVersion());

  if (decrypt) {
    auto [hdChainDec, hdChainEnc] = wallet_instance->GetHDChains();
    (void)hdChainEnc;
    SecureVector seed = hdChainDec.GetSeed();
    SecureString words;
    if (!wallet_instance->GetMnemonic(hdChainDec, words)) {
      throw std::runtime_error(std::string(__func__) + ": Get Mnemonic failed");
    }
    tfm::format(std::cout, "Decrypted word phrase: \"%s\"\n", words);
    tfm::format(std::cout, "Decrypted seed: %s\n", HexStr(seed));
  } else {
    CHDChain ec;
    wallet_instance->GetCryptedHDChain(ec);
    SecureVector words;
    SecureVector seed = ec.GetSeed();
    ec.GetMnemonic(words);
    tfm::format(std::cout, "encrypted words as Hex String: %s\n", HexStr(words));
    tfm::format(std::cout, "encrypted seed as Hex String: %s\n", HexStr(seed));
  }
}

bool GetWalletInfo() {

  fs::path wallet_dir = GetDataDir() / "wallets";
  std::string walletFile = "wallet.dat";
  fs::path wallet_path = wallet_dir / walletFile;
  
  if (!fs::exists(wallet_path)) {
    tfm::format(std::cerr, "Error: no wallet file at %s\n", wallet_path.string());
    return false;
  }
  std::string error;
  if (!BerkeleyBatch::VerifyEnvironment(wallet_dir, error)) {
      tfm::format(std::cerr, "Error loading %s. Is wallet being used by other process?\n", wallet_path.string());
      return false;
  }
  std::shared_ptr<CWallet> wallet_instance = LoadWallet(wallet_path);
  if (!wallet_instance) return false;
  WalletShowInfo(wallet_instance.get());
  wallet_instance->Flush(true);
  return true;
}
} // namespace WalletTool
