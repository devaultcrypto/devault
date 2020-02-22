// Copyright (c) 2020 The DeVault developers
// Copyright (c) 2020 Jon Spock
// Distributed under the MIT software license, see the accompanying

// Although these functions require interacting with wallet, they have been kept
// outside of the class due to minimal interaction. Wallet is required for the
// private keys used.

#pragma once

#include <optional>
#include <set>
#include <string>
#include <utility>

class CWallet;
class CInputCoin;
class CMutableTransaction;

auto CreatePrivateTxWithSig(const CWallet *pwallet, const std::set<CInputCoin> &setCoins, CMutableTransaction &txNew)
    -> std::optional<std::string>;
auto CreatePrivateTxWithSig(const CWallet *pwallet, CMutableTransaction &txNew) -> std::optional<std::string>;
