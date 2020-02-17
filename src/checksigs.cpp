// Copyright (c) 2020 The DeVault Developers
// Copyright (c) 2020 Jon Spock
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checksigs.h>
#include <bls/bls_functions.h>
#include <coins.h>
#include <hash.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <script/sigcache.h>
#include <script/standard.h>

static uint256 VinHash(const CTxIn &vin) {
    CHashWriter ss(SER_GETHASH, 0);
    CTxIn v(vin);

    // Null out the scriptsig from the Vins
    CScript c;
    v.scriptSig = c;

    ss << v;
    return ss.GetHash();
}
static uint256 VoutHash(const CTxOut &vin) {
    CHashWriter ss(SER_GETHASH, 0);
    ss << vin;
    return ss.GetHash();
}

bool CheckPrivateSigs(const CTransaction &tx, const CCoinsViewCache &inputs) {

    std::vector<std::vector<uint8_t>> input_pubkeys;
    std::vector<uint256> input_hashes;
    std::vector<uint256> rand_hashes;
    std::vector<uint8_t> aggSig;

    LogPrintf("For %d inputs, Tx size for Temporary debug = %d\n",
              tx.vin.size(), tx.GetTotalSize());

    // Collect Public Keys 1st
    for (size_t i = 0; i < tx.vin.size(); i++) {
        const CScript &scr = tx.vin[i].scriptSig; // will have BLS Public Key
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin &coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // Go from script to PubKeys here if it was P2PK
        bool was_pubkey = IsValidBLSPubKeySize(scr);
        
        CPubKey pubkey;
        if (i < tx.vin.size() - 1) {
            // Pubkey not in script, therefore look up from coin script
            if (!was_pubkey) {
                std::vector<uint8_t> pubk;
                MatchPayToBLSPubkey(coin.GetTxOut().scriptPubKey, pubk);
                input_pubkeys.push_back(pubk);
            } else {
                pubkey = ExtractBLSPubKey(scr);
                input_pubkeys.push_back(ToByteVector(pubkey));
            }
        } else {
            // For last Tx - For now assume always PUBKEY is present
            // Check if script is 1) Public Key + Aggregate Signature + Random
            // Pubkey 2) Just Aggregate Signature + Random Pubkeys
            std::vector<std::vector<uint8_t>> pubkeys;
            // Handle Grabbing Agg Sig + Input Pubkey + All of the Random
            // Pubkeys used
            std::tie(pubkeys, aggSig) = ExtractBLSPubKeysAndSig(scr, tx.vout.size());
            for (const auto &p : pubkeys) input_pubkeys.push_back(p);
        }

        uint256 in_hash = VinHash(tx.vin[i]);
        input_hashes.push_back(in_hash);
    }

    for (const auto &vout : tx.vout) rand_hashes.push_back(VoutHash(vout));
    // Concantenate Random Pubkeys + Hashes to Input ones
    for (const auto &o : rand_hashes) input_hashes.push_back(o);
    return bls::VerifySigForMessages(input_hashes, aggSig, input_pubkeys);
}
