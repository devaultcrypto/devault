// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <dstencode.h>
#include <chain.h>
#include <config.h>
#include <chainparams.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace WalletTool {

static std::shared_ptr<CWallet> LoadWallet(const fs::path& wallet_path)
{
    if (!fs::exists(wallet_path)) {
        tfm::format(std::cerr, "Error: Wallet files does not exist\n");
        return nullptr;
    }

    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, "wallet.dat"));
    auto &config = const_cast<Config &>(GetConfig());
    auto &chainParams = config.GetChainParams();
    // dummy chain interface
    std::shared_ptr<CWallet> wallet_instance = std::make_unique<CWallet>(chainParams, std::move(dbw));
    DBErrors load_wallet_ret;
    try {
        bool first_run;
        load_wallet_ret = wallet_instance->LoadWallet(first_run);
    } catch (const std::runtime_error&) {
        tfm::format(std::cerr, "Error loading %s. Is wallet being used by another process?\n", wallet_path.c_str());
        return nullptr;
    }

    if (load_wallet_ret != DBErrors::LOAD_OK) {
        wallet_instance = nullptr;
        if (load_wallet_ret == DBErrors::CORRUPT) {
            tfm::format(std::cerr, "Error loading %s: Wallet corrupted", wallet_path.c_str());
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NONCRITICAL_ERROR) {
            tfm::format(std::cerr, "Error reading %s! All keys read correctly, but transaction data"
                            " or address book entries might be missing or incorrect.",
                        wallet_path.c_str());
        } else if (load_wallet_ret == DBErrors::TOO_NEW) {
            tfm::format(std::cerr, "Error loading %s: Wallet requires newer version of %s",
                        wallet_path.c_str(), PACKAGE_NAME);
            return nullptr;
        } else if (load_wallet_ret == DBErrors::NEED_REWRITE) {
            tfm::format(std::cerr, "Wallet needed to be rewritten: restart %s to complete", PACKAGE_NAME);
            return nullptr;
        } else {
            tfm::format(std::cerr, "Error loading %s", wallet_path.c_str());
            return nullptr;
        }
    }

    return wallet_instance;
}

static void WalletShowInfo(CWallet* wallet_instance)
{
    LOCK(wallet_instance->cs_wallet);

    tfm::format(std::cout, "Wallet info\n===========\n");
    tfm::format(std::cout, "Encrypted: %s\n", wallet_instance->IsCrypted() ? "yes" : "no");
    CHDChain ec;
    wallet_instance->GetCryptedHDChain(ec);
    SecureVector words;
    SecureVector seed = ec.GetSeed();
    ec.GetMnemonic(words);


    int i=0;
    for (const auto& m : wallet_instance->mapAddressBook) {
        tfm::format(std::cout, "[%d] Address: %s, purpose: %s, name: %s\n", i++, EncodeDestination(m.first), m.second.purpose, m.second.name);
    }
    i=0;
    for (const auto& m : wallet_instance->mapHdPubKeys) {
        tfm::format(std::cout, "[%d] % s, account index = %d, change index = %d, path = %s, id = %s\n", i++,
                    EncodeDestination(m.first),
                    m.second.nAccountIndex,
                    m.second.nChangeIndex,
                    m.second.GetKeyPath(), m.second.hdchainID.ToString());
    }

    tfm::format(std::cout, "encrypted words as Hex String: %s\n", HexStr(words));
    tfm::format(std::cout, "encrypted seed as Hex String: %s\n", HexStr(seed));
    tfm::format(std::cout, "Keypool Size: %u\n", wallet_instance->GetKeyPoolSize());
    tfm::format(std::cout, "Transactions: %zu\n", wallet_instance->mapWallet.size());
    tfm::format(std::cout, "Address Book: %zu\n", wallet_instance->mapAddressBook.size());

}

bool GetWalletInfo(const std::string& wallet_path)
{
    if (!fs::exists(wallet_path)) {
        tfm::format(std::cerr, "Error: no wallet file at %s\n", wallet_path.c_str());
        return false;
    }
    std::string error;
    /*
      if (!WalletBatch::VerifyEnvironment(path, error)) {
      tfm::format(std::cerr, "Error loading %s. Is wallet being used by other process?\n", name.c_str());
      return false;
      }
    */
    std::shared_ptr<CWallet> wallet_instance = LoadWallet(wallet_path);
    if (!wallet_instance) return false;
    WalletShowInfo(wallet_instance.get());
    wallet_instance->Flush(true);
    return true;
}
} // namespace WalletTool
