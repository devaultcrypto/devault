// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

// This header is to allow unit tests but should not be
// generally included except in dstencode.h
// this should avoid confusion with the naming of the below functions

#include <script/standard.h>
#include <key.h>
#include <string>
#include <vector>
#include <dstencode.h>

struct CashAddrContent {
    CashAddrType type;
    std::vector<uint8_t> hash;
};

std::string EncodeCashAddr(const CTxDestination &, const CChainParams &);
std::string EncodeCashAddr(const std::string &prefix, const CashAddrContent &content);
CTxDestination DecodeCashAddr(const std::string &addr, const CChainParams &params);
CashAddrContent DecodeCashAddrContent(const std::string &addr, const std::string &prefix);
std::vector<uint8_t> PackCashAddrContent(const CashAddrContent &content);

