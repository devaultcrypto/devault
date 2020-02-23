// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/system.h>
#include <wallet/walletutil.h>

fs::path GetWalletDir() {
    fs::path path;

    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetArg("-walletdir", "");
        if (!fs::is_directory(path)) {
            // If the path specified doesn't exist, we return the deliberately
            // invalid empty string.
            path = "";
        }
    } else {
        path = GetDataDir();
        // Always use a wallets directory
        path /= "wallets";
    }

    return path;
}

fs::path GetWalletDirNoCreate(fs::path& added_dir) {
  fs::path path;
  
  if (gArgs.IsArgSet("-walletdir")) {
    path = gArgs.GetArg("-walletdir", "");
    if (!fs::is_directory(path)) {
      // If the path specified doesn't exist, we return the deliberately
      // invalid empty string.
      path = "";
    }
  } else {
    path = GetDataDirNoCreate();
    
    // This will be Net specific addition
    if (added_dir != "") {
      path /= added_dir;
    }
  
    // Always assume a wallets directory
    path /= "wallets";
  }
  
  return path;
}

fs::path GetWalletPathNoCreate(fs::path& added, const std::string& file) {
#ifdef NO_BOOST_FILESYSTEM
  return GetWalletDirNoCreate(added) / file;
#else
  return fs::absolute(file, GetWalletDirNoCreate(added));
#endif
}


WalletLocation::WalletLocation(const std::string &name)
  : m_name(name) {
#ifdef NO_BOOST_FILESYSTEM
  m_path = fs::absolute( GetWalletDir() / name );
#else
  m_path = fs::absolute( name, GetWalletDir() );
#endif
}

bool WalletLocation::Exists() const {
#ifdef NO_BOOST_FILESYSTEM
  return fs::symlink_status(m_path).type() != fs::file_type::not_found;
#else
  return fs::symlink_status(m_path).type() != fs::file_not_found;
#endif
}
