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
    
    std::vector<std::vector<uint8_t>> pubkeys;
    std::vector<std::vector<uint8_t>> rand_pubkeys;
    std::vector<std::vector<uint8_t>> aggSigs;
    std::vector<uint256> msgs;
    std::vector<uint256> out_msgs;

    CScript cfinal;
    CTxIn vin;

    auto j = txVariants.size() - 1;
    for (const auto &txv : txVariants) {
      //auto enc = EncodeHexTx(CTransaction(txv));
        //std::cout << "Tx : " << enc << "\n";
        for (const auto &vout : txv.vout) txNew.vout.push_back(vout);
      
        for (size_t i = 0; i < txv.vin.size(); i++) {
            vin = txv.vin[i];
            auto scr = vin.scriptSig;
            uint256 in_hash = VinHash(vin);
            msgs.push_back(in_hash);
            
            if (check_coins) {
              const Coin &coin = view.AccessCoin(vin.prevout);
              if (coin.IsSpent()) {
                throw std::runtime_error("Input not found or already spent");
              }
            }

            // const CScript &prevPubKey = coin.GetTxOut().scriptPubKey;
            // const Amount &amount = coin.GetTxOut().nValue;
            if (i == txv.vin.size() - 1) {
                std::vector<std::vector<uint8_t>> pkeys;
                std::vector<uint8_t> aggSig;
                // Handle Grabbing Agg Sig + Input Pubkey + All of the Random
                // Pubkeys used
                std::tie(pkeys, aggSig) =   ExtractBLSPubKeysAndSig(scr, txv.vout.size());

                auto pubk = pkeys[0];
                pubkeys.push_back(pubk);
                pkeys.erase(pkeys.begin());

                aggSig.pop_back();
                aggSigs.push_back(aggSig);
                for (const auto &p : pkeys) rand_pubkeys.push_back(p);

                if (j-- == 0) {
                    cfinal << pubk;
                    // CScript c; c already has the last pubkey of the input
                    for (const auto &pub : rand_pubkeys)
                        cfinal << ToByteVector(pub); // Add random pubkeys to the last input

                } else {
                    CScript c;
                    c << pubk;
                    vin.scriptSig = c;
                    txNew.vin.push_back(vin);
                }

            } else {
                // simply add
                txNew.vin.push_back(vin);
                // gather pubkeys
                auto pubkey = ExtractBLSPubKey(scr);
                pubkeys.push_back(ToByteVector(pubkey));
            }
        }
    }

    int jj=0;
    for (const auto &txv : txVariants) {
      for (const auto &vout : txv.vout) {
        auto& pk = rand_pubkeys[jj++];
        out_msgs.push_back(VoutHash(vout,pk));
      }
    }


    msgs.insert(msgs.end(), out_msgs.begin(),out_msgs.end());
    pubkeys.insert(pubkeys.end(), rand_pubkeys.begin(),rand_pubkeys.end());

    // Now get combined Sig 
    std::vector<uint8_t> combined_sigs = bls::MakeAggregateSigsForMessages(
                                                                           msgs,
                                                                           aggSigs,
                                                                           pubkeys);

    //std::cout << "combined sig is " << HexStr(combined_sigs) << "\n";
    
    // for Script serialization
    SigHashType sigHashType;
    combined_sigs.push_back(uint8_t(sigHashType.getRawSigHashType()));

    cfinal << combined_sigs;
    cfinal << OP_CHECKSIG; // do we need this?, check later

    vin.scriptSig = cfinal;
    txNew.vin.push_back(vin);

    txNew.nVersion = CTransaction::BLS_ONLY_VERSION;

    
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
