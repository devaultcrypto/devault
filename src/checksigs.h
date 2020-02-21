// Copyright (c) 2020 The DeVault Developers
// Copyright (c) 2020 Jon Spock
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

class CTransaction;
class CCoinsViewCache;

bool CheckSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
bool CheckPrivateSigs(const CTransaction &tx, const CCoinsViewCache &inputs);
