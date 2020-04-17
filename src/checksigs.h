// Copyright (c) 2020 The DeVault Developers
// Copyright (c) 2020 Jon Spock
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>
#include <cstdint>

class CTransaction;
class CCoinsViewCache;
class CScript;
class uint256;

bool CheckSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
bool CheckPrivateSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
// assumes coins already checked for not spent
bool CheckPrivateSigs(const CTransaction &tx);

