// Copyright (c) 2020 Jon Spock
// Copyright (c) 2020 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <combine_transactions.h>

#include <coins.h>
#include <primitives/transaction.h>
#include <primitives/create_bls_transaction.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sighashtype.h>

#include <core_io.h>

#include <txmempool.h>
#include <validation.h>

//#include <util/strencodings.h>

#include <bls/bls_functions.h>
#include <checksigs.h>
#include <cstdint>

CMutableTransaction combine_transactions(const std::vector<CMutableTransaction> &txVariants,
                                         bool check_coins) {

    CMutableTransaction txNew;

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    if (check_coins) {
      LOCK(cs_main);
      LOCK(g_mempool.cs);
      CCoinsViewCache &viewChain = *pcoinsTip;
      CCoinsViewMemPool viewMempool(&viewChain, g_mempool);
      // temporarily switch cache backend to db+mempool view
      view.SetBackend(viewMempool);
      
      for (const CMutableTransaction &tx : txVariants) {
        for (const CTxIn &txin : tx.vin) {
          // Load entries from viewChain into view; can fail.
          view.AccessCoin(txin.prevout);
          }
      }
      
      // switch back to avoid locking mempool for too long
      view.SetBackend(viewDummy);
    }
    
    std::vector<std::vector<uint8_t>> rand_pubkeys;
    std::vector<std::vector<uint8_t>> aggSigs;

    std::vector<CTxIn> vins;

    std::map<std::vector<uint8_t>, CTxOut> output_keys_and_vouts;

    CTxIn vin;

    for (const auto &txv : txVariants) {
      
        for (size_t i = 0; i < txv.vin.size(); i++) {
            vin = txv.vin[i];
            
            auto scr = vin.scriptSig;
            
            if (check_coins) {
              const Coin &coin = view.AccessCoin(vin.prevout);
              if (coin.IsSpent()) {
                throw std::runtime_error("Input not found or already spent");
              }
            }

            if (i == txv.vin.size() - 1) {
                std::vector<std::vector<uint8_t>> pkeys;
                std::vector<uint8_t> aggSig;
                // Handle Grabbing Agg Sig + Input Pubkey + All of the Random
                // Pubkeys used
                std::tie(pkeys, aggSig) =   ExtractBLSPubKeysAndSig(scr, txv.vout.size());

                auto pubk = pkeys[0];
                pkeys.erase(pkeys.begin());

                aggSig.pop_back();
                aggSigs.push_back(aggSig);
                for (const auto &p : pkeys) rand_pubkeys.push_back(p);

                CScript c;
                // replace with just public key of input
                c << pubk;
                vin.scriptSig = c;
                vins.push_back(vin);

            } else {
                // simply add
                vins.push_back(vin);
           }
        }
    }

    // Now get combined Sig 
    std::vector<uint8_t> combined_sigs = bls::Aggregate(aggSigs);

    // Gathered outputs are sorted by the random pubkey addresses due to natural std::map sorting
    int jj=0;
    for (const auto &txv : txVariants) {
      for (const auto &vout : txv.vout) {
        output_keys_and_vouts.emplace(rand_pubkeys[jj++],vout);
      }
    }

    // Sort Vins and then add to txNew vins
    std::sort(vins.begin(),vins.end());
    for (const auto& v : vins) txNew.vin.push_back(v);

    // Need to modify the script for the last entry,
    // so pop off, modify and then add back to txNew.vin
    vin = txNew.vin.back();
    txNew.vin.pop_back();
    // 
    // Put the last pubkey of the input first
    CScript cfinal = vin.scriptSig;
    for (const auto &k : output_keys_and_vouts) {
      cfinal << ToByteVector(k.first); // Add random pubkeys to the last input
      txNew.vout.push_back(k.second);
    }

    // for Script serialization
    SigHashType sigHashType;
    combined_sigs.push_back(uint8_t(sigHashType.getRawSigHashType()));

    cfinal << combined_sigs;

    // Push back vin now with new script
    vin.scriptSig = cfinal;
    txNew.vin.push_back(vin);

    txNew.nVersion = CTransaction::BLS_BLOCK_VERSION;

    
    {
      CTransaction ctx(txNew);
      auto NewEnc = EncodeHexTx(ctx);
      //std::cout << "Combined : " << NewEnc << "\n";
      bool check = CheckPrivateSigs(ctx);
      if (!check) {
        std::cout << "************* Failed to verify combined transaction\n";
      }
    }

    return txNew;
}
