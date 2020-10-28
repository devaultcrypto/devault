// Copyright (c) 2020 The DeVault developers
// Copyright (c) 2020 Jon Spock
// Distributed under the MIT software license, see the accompanying

#include <bls/bls_functions.h>
#include <primitives/create_bls_transaction.h>
#include <wallet/bls_tx.h>
#include <wallet/wallet.h>

// TBD
auto CreatePrivateTxWithSig(const CWallet *pwallet, CMutableTransaction &txNew) -> std::optional<std::string> {
  std::vector<CKey> keys;
  CTransaction txNewConst(txNew);
  
  return std::nullopt;
}

auto hashFromPrivateKeyAndOutPoint(const CKey& key, const COutPoint& out, uint32_t count) {
  CHashWriter ss(SER_GETHASH, 0);
  uint256 keyhash = Hash(key.begin(),key.end());
  ss << keyhash;
  ss << out;
  ss << count;
  auto hash = ss.GetHash();
  std::vector<uint8_t> message(hash.begin(),hash.end());
  return message;
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
    COutPoint reference_outpoint;
    CKey reference_private;

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
        // just use the 1st coin found for this
        if (reference_outpoint.GetN() == uint32_t(-1)) {
          reference_outpoint = coin.outpoint;
          reference_private = key;
        }
    }

    uint32_t output_number = 0;
    for (const auto &out : txNew.vout) {
        CKey key;
        auto random_input = hashFromPrivateKeyAndOutPoint(reference_private, reference_outpoint, output_number++);
        key.MakeNewDeterministicBLSKey(random_input); // A deterministically random private/pub key pair that is used just once
        // Save key to wallet DB for future?
        pwallet->WriteBLSRandomKey(key); // TBD
        pubkey = key.GetPubKeyForBLS();
        pubkeys.push_back(ToByteVector(pubkey));
        rand_pubkeys.push_back(ToByteVector(pubkey));

        uint256 hash = VoutHash(out, ToByteVector(pubkey));

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

    // Put Agg Sig + Random Public Keys into the vin of the last input of the
    // tx.
    UpdateTransaction(txNew, nIn - 1, c); // Append to input to keep output simple

    aggSig.pop_back();
    bool check = bls::VerifySigForMessages(input_hashes, aggSig, pubkeys);
    if (!check) {
      return "BLS Verify check failed in CreateTransaction";
    }

    txNew.nVersion = CTransaction::BLS_ONLY_VERSION;
    return std::nullopt;
}
