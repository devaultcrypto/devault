// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "uint256.h"
#include <vector>
#include <map>
#include <key.h>

namespace bls {
  
    bool SignBLS(const CKey& key, const uint256 &hash, std::vector<uint8_t> &vchSig);
    bool SignBLS(const CKey& key, const std::vector<uint8_t> &message, std::vector<uint8_t> &vchSig);
    auto SignBLS(const CKey &key, const uint256 &hash) -> std::optional<std::vector<uint8_t>>;


    // for test routine
    std::vector<uint8_t> AggregatePubKeys(std::vector<std::vector<uint8_t>> &vPubKeys);

    std::vector<uint8_t> Aggregate(std::vector<std::vector<uint8_t>> &vSigs);

    bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t* vch);
    CPubKey GetBLSPublicKey(const CKey &key);
    CKey GetBLSPrivateKey(const std::vector<uint8_t>& seed, uint32_t childIndex);
    CKey GetBLSMasterKey(const SecureVector& seed);
    CKey GetBLSChild(const CKey& master, uint32_t childIndex);
    bool CheckValidBLSPrivateKey(const uint8_t* bytes);


    // For Distinct Messages
    //  - need
    std::vector<uint8_t> AggregateSigForMessages(std::map<uint256,CKey>& keys_plus_hash);
    bool VerifySigForMessages(const std::vector<uint256>& msgs,
                              const std::vector<uint8_t> &aggSigs, 
                              const std::vector<std::vector<uint8_t>> &aggPubKeys);


    bool VerifySigForMessages(const std::vector<std::vector<uint8_t>> &msgs, const std::vector<uint8_t> &aggSigs,
                              const std::vector<std::vector<uint8_t>> &aggPubKeys);


    
} // namespace bls
