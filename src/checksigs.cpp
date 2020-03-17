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


std::vector<uint8_t> ExtractPubKeyFromBLSScript(const CScript &scriptPubKey) {
    std::vector<uint8_t> pubkey;
    txnouttype whichTypeRet;
    std::vector<std::vector<uint8_t>> vSolutions;
    bool ok = Solver(scriptPubKey, whichTypeRet, vSolutions);
    if (!ok || whichTypeRet != TX_BLSPUBKEY) {
        return pubkey; // emtpy value => problem
    }
    return vSolutions[0];
}

bool CheckPrivateSigs(const CTransaction &tx, const CCoinsViewCache &inputs) {

    std::map<std::vector<uint8_t>, CPubKey> mapPubKeys;
    
    std::vector<std::vector<uint8_t>> input_pubkeys;
    std::vector<uint256> input_hashes;
    std::vector<uint256> rand_hashes;
    std::vector<uint8_t> aggSig;

    //LogPrintf("For %d inputs, Tx size for Temporary debug = %d\n",   tx.vin.size(), tx.GetTotalSize());

    // Collect Raw Public Keys 1st and create map with PKH
    for (size_t i = 0; i < tx.vin.size(); i++) {
        const CScript &scr = tx.vin[i].scriptSig; // will have BLS Public Key

        // Go from script to PubKeys here if it was P2PK
        bool was_pubkey = IsValidBLSPubKeySize(scr);
      
        CPubKey pubkey;
        if (i < tx.vin.size() - 1) {
            // Pubkey not in script, therefore look up from coin script
            if (was_pubkey) {
                std::vector<uint8_t> pubk;
                pubkey = ExtractBLSPubKey(scr);
                mapPubKeys.emplace(ToByteVector(pubkey.GetBLSKeyID()),pubkey);
            }
        } else {
            // For last Tx - For now assume always PUBKEY is present
            // Check if script is 1) Public Key + Aggregate Signature + Random
            // Pubkey 2) Just Aggregate Signature + Random Pubkeys
            std::vector<std::vector<uint8_t>> pubkeys;
            // Handle Grabbing Agg Sig + Input Pubkey + All of the Random
            // Pubkeys used
            std::tie(pubkeys, aggSig) = ExtractBLSPubKeysAndSig(scr, tx.vout.size());
            // Just do for the 1st pubkey - i.e ignore random keys
            CPubKey p(pubkeys[0]);
            mapPubKeys.emplace(ToByteVector(p.GetBLSKeyID()),pubkeys[0]);
        }
    }

    // Collect Public Keys 1st
    for (size_t i = 0; i < tx.vin.size(); i++) {
        const CScript &scr = tx.vin[i].scriptSig; // will have BLS Public Key
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin &coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // Go from script to PubKeys here if it was P2PK
        bool was_pubkey = IsValidBLSPubKeySize(scr);
      
        //std::cout << "i = " << i << " script size = " << scr.size() << "\n";
        
        CPubKey pubkey;
        if (i < tx.vin.size() - 1) {
            // Pubkey not in script, therefore look up from coin script
            std::vector<uint8_t> pubk;
            if (!was_pubkey) {
              // std::cout << " i = " << i << " other size = " << coin.GetTxOut().scriptPubKey.size() << "\n";
                if (MatchPayToBLSPubkey(coin.GetTxOut().scriptPubKey, pubk)) {
                    input_pubkeys.push_back(pubk);
                } else {
                    std::vector<uint8_t> pubkh;
                    if (MatchPayToBLSkeyHash(coin.GetTxOut().scriptPubKey, pubkh)) {
                        auto p = mapPubKeys[pubkh];
                      pubk = ToByteVector(p);
                      // Get Pub Key from PKH
                    } else {
                      // Get Pub Key from Script
                      pubk = ExtractPubKeyFromBLSScript(scr);
                    }
                    // need to get Public Key for this PKH
                    if (pubk.size() != CPubKey::BLS_PUBLIC_KEY_SIZE) return false;
                    input_pubkeys.push_back(pubk);
                }
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
