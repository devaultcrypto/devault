// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <net.h>

// Already in init.cpp which is linked
//std::unique_ptr<CConnman> g_connman;

[[noreturn]] void Shutdown(void *parg) { std::exit(EXIT_SUCCESS); }

[[noreturn]] void StartShutdown() { std::exit(EXIT_SUCCESS); }

bool ShutdownRequested() { return false; }
