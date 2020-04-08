// Copyright (c) 2020 The DeVault developers
// Copyright (c) 2020 Jon Spock
// Distributed under the MIT software license, see the accompanying

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

class CMutableTransaction;
class CKey;
class uint256;
class CTxIn;
class CTxOut;

uint256 VinHash(const CTxIn &vin);
uint256 VoutHash(const CTxOut &vin, const std::vector<uint8_t>& pubkey);

auto CreatePrivateTxWithSig(const std::vector<CKey>& keys,
                            const std::vector<bool>& was_paytopublic,
                            CMutableTransaction &txNew)
    -> std::optional<std::string>;

/*
void SetupPrivateTxWithSig(const std::vector<CKey>& keys,
                                          const std::vector<bool>& was_paytopublic,
                                          CMutableTransaction& txNew,
                                          std::map<uint256, CKey>& input_keys_and_hashes);
*/
