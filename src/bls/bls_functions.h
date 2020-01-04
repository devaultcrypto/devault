// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock

#include "uint256.h"
#include <vector>
#include <key.h>

namespace bls {
    class Signature; // Forward Declaration

    bool SignBLS(const CKey &key, const uint256 &hash, Signature& sig);
    //    bool VerifyBLS(const uint256 &hash, Signature& sig, const uint8_t *vch);
    std::vector<uint8_t> AggregateSigs(std::vector<Signature*> &vpSigs);

    std::vector<uint8_t> AggregateSig(std::vector<CKey>& keys, const uint256& hash);
    bool VerifyAggregate(const uint256& hash,
                         const std::vector<uint8_t> &aggSig,
                         const std::vector<uint8_t> &aggPubKeys);

    // More Generic interfaces
    
    bool SignBLS(const CKey& key, const uint256 &hash, std::vector<uint8_t> &vchSig);
    bool VerifyBLS(const uint256 &hash, const std::vector<uint8_t> &vchSig, const uint8_t* vch);
    CPubKey GetBLSPublicKey(const CKey &key);

    //    bool SignBLSMessage(const CKey &key, const std::vector<uint8_t> &message, Signature& sig);
    std::vector<uint8_t> AggregatePubKeys(std::vector<std::vector<uint8_t>> &vPubKeys);
    
} // namespace bls
