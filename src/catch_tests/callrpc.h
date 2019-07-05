// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <rpc/server.h>

#pragma once
#include <string>
#include <univalue.h>
UniValue CallRPC(std::string args);
