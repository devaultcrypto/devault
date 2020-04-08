// Copyright (c) 2020 The DeVault Developers
// Copyright (c) 2020 Jon Spock
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>

class CTransaction;
class CCoinsViewCache;
class CScript;
class uint256;

bool CheckSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
bool CheckPrivateSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
// assumes coins already checked for not spent
bool CheckPrivateSigs(const CTransaction &tx);

bool SetupCheckPrivateSigs(const CTransaction &tx, const std::vector<CScript>& scripts,
                           std::vector<uint256>& input_hashes,
                           std::vector<std::vector<uint8_t>>& input_pubkeys,
                           std::vector<uint8_t>& aggSig
                           );

