// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock

#include "uint256.h"
#include <vector>
#include <map>
#include <key.h>

namespace bls {
    class Signature; // Forward Declaration

    bool SignBLS(const CKey &key, const uint256 &hash, Signature& sig);
    bool SignBLS(const CKey& key, const uint256 &hash, std::vector<uint8_t> &vchSig);
    auto SignBLS(const CKey &key, const uint256 &hash) -> std::optional<std::vector<uint8_t>>;
    std::vector<uint8_t> AggregateSigs(std::vector<Signature*> &vpSigs);

  // for test routine
  std::vector<uint8_t> AggregatePubKeys(std::vector<std::vector<uint8_t>> &vPubKeys);

    // More Generic interfaces - need
    std::vector<uint8_t> MakeAggregateSigsForMessages(const std::vector<uint256> &msgs,
                                                      const std::vector<std::vector<uint8_t>> &aggSigs,
                                                      const std::vector<std::vector<uint8_t>> &pubkeys);

    bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t* vch);
    CPubKey GetBLSPublicKey(const CKey &key);


    // For Distinct Messages
    //  - need
    std::vector<uint8_t> AggregateSigForMessages(std::map<uint256,CKey>& keys_plus_hash);
    bool VerifySigForMessages(const std::vector<uint256>& msgs,
                              const std::vector<uint8_t> &aggSigs, 
                              const std::vector<std::vector<uint8_t>> &aggPubKeys);


    
} // namespace bls
