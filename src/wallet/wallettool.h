// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <wallet/wallet.h>

namespace WalletTool {

std::shared_ptr<CWallet> LoadWallet(const std::string& name, const fs::path& path);
void WalletShowInfo(CWallet* wallet_instance);
bool GetWalletInfo(const std::string& file);

} // namespace WalletTool
