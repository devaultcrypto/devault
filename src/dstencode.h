// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// key.h and pubkey.h are not used here, but gcc doesn't want to instantiate
// CTxDestination if types are unknown
#include <key.h>
#include <pubkey.h>
#include <script/standard.h>
#include <string>

class Config;
class CChainParams;
class CTxOut;

// 13 means most mainnet BLS addresses start with 'd'
enum CashAddrType : uint8_t { PUBKEY_TYPE = 0, SCRIPT_TYPE = 1, SECRET_TYPE = 2, BLSPUBKEY_TYPE = 13 };

std::string GetAddrFromTxOut(const CTxOut& out);
CTxDestination GetDestFromTxOut(const CTxOut& out);

std::string EncodeDestination(const CTxDestination &dest, const Config &config);
CTxDestination DecodeDestination(const std::string &addr, const CChainParams &);

bool IsValidDestinationString(const std::string &addr, const CChainParams &params);

//? Temporary workaround. Don't rely on global state, pass all parameters in new code.
std::string EncodeDestination(const CTxDestination &);


CKey DecodeSecret(const std::string &addr);
std::string EncodeSecret(const CKey& key);
bool CheckSecretIsValid(const std::string &addr);

