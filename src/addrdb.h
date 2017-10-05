// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <ban.h>
#include <fs.h>

/** Access to the (IP) address database (peers.dat) */
class CAddrDB {
private:
    fs::path pathAddr;
    const CChainParams &chainParams;

public:
    CAddrDB(const CChainParams &chainParams);
    bool Write(const CAddrMan &addr);
    bool Read(CAddrMan &addr);
    bool Read(CAddrMan &addr, CDataStream &ssPeers);
};

/** Access to the banlist database (banlist.dat) */
class CBanDB {
private:
    const fs::path m_ban_list_path;
    const CChainParams &chainParams;

public:
    CBanDB(fs::path ban_list_path, const CChainParams &_chainParams);
    bool Write(const banmap_t &banSet);
    bool Read(banmap_t &banSet);
};
