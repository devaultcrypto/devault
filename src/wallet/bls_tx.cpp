// Copyright (c) 2020 The DeVault developers
// Copyright (c) 2020 Jon Spock
// Distributed under the MIT software license, see the accompanying

#include <bls/bls_functions.h>
#include <wallet/bls_tx.h>
#include <wallet/wallet.h>

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

// TBD
auto CreatePrivateTxWithSig(const CWallet *pwallet, CMutableTransaction &txNew) -> std::optional<std::string> {
  std::vector<CKey> keys;
  CTransaction txNewConst(txNew);
  
  return std::nullopt;
}


auto CreatePrivateTxWithSig(const CWallet *pwallet,
                            const std::set<CInputCoin> &setCoins,
                            CMutableTransaction &txNew)
    -> std::optional<std::string> {

    SigHashType sigHashType;
    CTransaction txNewConst(txNew);
    int nIn = 0;

    // Collect the Public Keys, getting from Hashes if necessary
    CPubKey pubkey;
    bool has_pubkey_already = false;
    std::set<CPubKey> pubkeyset;
    std::map<uint256, CKey> input_keys_and_hashes;
    std::vector<uint256> input_hashes;
    std::vector<std::vector<uint8_t>> pubkeys;
    std::vector<std::vector<uint8_t>> rand_pubkeys;
    assert(setCoins.size() == txNew.vin.size());

    CScript c;
    bool was_public = false;
    for (const auto &coin : setCoins) {
        CKey key;
        bool got_key;
        std::tie(got_key, key, pubkey, was_public) = pwallet->ExtractFromBLSScript(coin.txout.scriptPubKey);
        if (!got_key) {
            return "Unable to get BLS Public Key or Sign with BLS Key";
        }
        has_pubkey_already = false;
        // collect private keys in a set
        if (pubkeyset.count(pubkey) == 0) {
            pubkeyset.emplace(pubkey);
        } else {
            has_pubkey_already = true;
        }
        pubkeys.push_back(ToByteVector(pubkey));

        // Add PubKey
        // also if pubkey was already added no need to repeat, or if former TX was P2PK
        // except (for now) for the last input (due to the current extraction
        // script)
        c.clear();
        if ((!has_pubkey_already && !was_public) || (nIn == (int)setCoins.size() - 1)) {
            c << ToByteVector(pubkey);
        }

        uint256 in_hash = VinHash(
            txNewConst.vin[nIn]); // vin & setCoins are aligned so this is OK

        UpdateTransaction(txNew, nIn++, c);

        input_hashes.push_back(in_hash);
        input_keys_and_hashes.emplace(in_hash, key);
    }

    for (const auto &out : txNew.vout) {
        CKey key;
        key.MakeNewKey(); // A random private/pub key pair that is disposable
                          // and used just once
        pubkey = key.GetPubKeyForBLS();
        pubkeys.push_back(ToByteVector(pubkey));
        rand_pubkeys.push_back(ToByteVector(pubkey));

        uint256 hash = VoutHash(out);

        input_keys_and_hashes.emplace(hash, key);
        input_hashes.push_back(hash);
    }

    // Need Aggregate Signatures of keys with distinct messages
    std::vector<uint8_t> aggSig = bls::AggregateSigForMessages(input_keys_and_hashes);

    // for Script serialization
    aggSig.push_back(uint8_t(sigHashType.getRawSigHashType()));

    // CScript c; c already has the last pubkey of the input
    for (const auto &pub : rand_pubkeys)
        c << ToByteVector(pub); // Add random pubkeys to the last input

    c << aggSig;
    c << OP_CHECKSIG; // since we have custom extraction not sure we actually
                      // need this, check later

    // Put Agg Sig + Random Public Keys into the vin of the last input of the
    // tx.
    UpdateTransaction(txNew, nIn - 1, c); // Append to input to keep output simple

    aggSig.pop_back();
    bool check = bls::VerifySigForMessages(input_hashes, aggSig, pubkeys);
    if (!check) {
      return "BLS Verify check failed in CreateTransaction";
    }

    return std::nullopt;
}
