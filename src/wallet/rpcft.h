// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_WALLET_RPCFT_H
#define DEVAULT_WALLET_RPCFT_H

class Config;
class JSONRPCRequest;
class UniValue;

// DeVault fungible-token (DFT) wallet RPCs (DEVAULT_FT_SPEC.md §11, phase 5D). Defined in
// rpcft.cpp; registered from rpcwallet.cpp's command table.
UniValue deployft(const Config &config, const JSONRPCRequest &request);
UniValue mintft(const Config &config, const JSONRPCRequest &request);
UniValue sendft(const Config &config, const JSONRPCRequest &request);
UniValue getftinfo(const Config &config, const JSONRPCRequest &request);
UniValue listfttokens(const Config &config, const JSONRPCRequest &request);
UniValue getftbalance(const Config &config, const JSONRPCRequest &request);

#endif // DEVAULT_WALLET_RPCFT_H
