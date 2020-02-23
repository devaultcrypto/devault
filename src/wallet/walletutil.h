// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <util/fs_util.h>

//! Get the path of the wallet directory.
fs::path GetWalletDir();
fs::path GetWalletDirNoCreate(fs::path& added_dir);

//! The WalletLocation class provides wallet information.
class WalletLocation final {
    std::string m_name;
    fs::path m_path;

public:
    explicit WalletLocation() {}
    explicit WalletLocation(const std::string &name);

    //! Get wallet name.
    const std::string &GetName() const { return m_name; }

    //! Get wallet absolute path.
    const fs::path &GetPath() const { return m_path; }

    //! Return whether the wallet exists.
    bool Exists() const;
};

