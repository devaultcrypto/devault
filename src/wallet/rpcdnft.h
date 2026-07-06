// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DEVAULT_WALLET_RPCDNFT_H
#define DEVAULT_WALLET_RPCDNFT_H

class Config;
class JSONRPCRequest;
class UniValue;

// DeVault onchain NFT (DNFT) wallet RPCs (DEVAULT_NFT_SPEC.md §11, phase 4D). Defined in
// rpcdnft.cpp; registered from rpcwallet.cpp's command table.
UniValue mintnft(const Config &config, const JSONRPCRequest &request);
UniValue sendnft(const Config &config, const JSONRPCRequest &request);
UniValue burnnft(const Config &config, const JSONRPCRequest &request);
UniValue listnfts(const Config &config, const JSONRPCRequest &request);
UniValue getnftinfo(const Config &config, const JSONRPCRequest &request);

#endif // DEVAULT_WALLET_RPCDNFT_H
