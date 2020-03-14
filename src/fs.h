// Copyright (c) 2017 The Bitcoin Core developers
// Copyright (c) 2019 The Devault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// clang-format off
#pragma once

#include <cstdio>
#include <string>

#ifdef NO_BOOST_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#endif

#ifdef NO_BOOST_FILESYSTEM
namespace fs = std::filesystem;
#else
/** Filesystem operations and types */
namespace fs = boost::filesystem;
#endif

/** Bridge operations to C stdio */
namespace fsbridge {
inline FILE *fopen(const fs::path &p, const char *mode) {   return ::fopen(p.string().c_str(), mode);}
}; // namespace fsbridge

// clang-format on
