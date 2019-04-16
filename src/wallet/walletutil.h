// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <fs_util.h>

//! Get the path of the wallet directory.
fs::path GetWalletDir();
fs::path GetWalletDirNoCreate(fs::path& added_dir);

