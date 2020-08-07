// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RAWTRANSACTION_H
#define BITCOIN_RPC_RAWTRANSACTION_H

class CBasicKeyStore;
class CChainParams;
class CMutableTransaction;
class UniValue;

namespace interfaces {
class Chain;
} // namespace interfaces

/** Sign a transaction with the given keystore and previous transactions */
UniValue SignTransaction(interfaces::Chain &chain, CMutableTransaction &mtx,
                         const UniValue &prevTxs, CBasicKeyStore *keystore,
                         bool tempKeystore, const UniValue &hashType);

/** Create a transaction from univalue parameters */
CMutableTransaction ConstructTransaction(const CChainParams &params,
                                         const UniValue &inputs_in,
                                         const UniValue &outputs_in,
                                         const UniValue &locktime);

#endif // BITCOIN_RPC_RAWTRANSACTION_H
