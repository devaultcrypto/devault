// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Bitcoin developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETDB_H
#define BITCOIN_WALLET_WALLETDB_H

#include <amount.h>
#include <key.h>
#include <primitives/transaction.h>
#include <script/standard.h> // for CTxDestination
#include <wallet/db.h>
#include <wallet/hdchain.h>
#include <cstdint>
#include <list>
#include <string>
#include <utility>
#include <vector>

/**
 * Overview of wallet database classes:
 *
 * - BerkeleyEnvironment is an environment in which the database exists (has no analog in
 * dbwrapper.h)
 * - WalletDatabase represents a wallet database (similar to CDBWrapper in
 * dbwrapper.h)
 * - BerkeleyBatch is a low-level database transaction (similar to CDBBatch in
 * dbwrapper.h)
 * - WalletBatch is a modifier object for the wallet, and encapsulates a database
 *   transaction as well as methods to act on the database (no analog in
 *   dbwrapper.h)
 *
 * The latter two are named confusingly, in contrast to what the names BerkeleyBatch
 * and WalletBatch suggest they are transient transaction objects and don't
 * represent the database itself.
 */

static const bool DEFAULT_FLUSHWALLET = true;

class CAccountingEntry;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CScript;
class CWallet;
class CWalletTx;
class uint160;
class uint256;

/** Backend-agnostic database type. */
using WalletDatabase = BerkeleyDatabase;

/** Error statuses for the wallet database */
enum class DBErrors {
    LOAD_OK,
    CORRUPT,
    NONCRITICAL_ERROR,
    TOO_NEW,
    LOAD_FAIL,
    NEED_REWRITE
};

/**
 * Access to the wallet database.
 * This represents a single transaction at the database. It will be committed
 * when the object goes out of scope. Optionally (on by default) it will flush
 * to disk as well.
 */
class WalletBatch {
private:
    template <typename K, typename T>
    bool WriteIC(const K &key, const T &value, bool fOverwrite = true) {
        if (!m_batch.Write(key, value, fOverwrite)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        return true;
    }

    template <typename K> bool EraseIC(const K &key) {
        if (!m_batch.Erase(key)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        return true;
    }

public:
    explicit WalletBatch(WalletDatabase &database, const char *pszMode = "r+",
                         bool _fFlushOnClose = true)
        : m_batch(database, pszMode, _fFlushOnClose), m_database(database) {}
    WalletBatch(const WalletBatch &) = delete;
    WalletBatch &operator=(const WalletBatch &) = delete;

    bool ReadName(const std::string& address, std::string &strName);
    bool ReadLabel(std::string& address, const std::string &strName);
    bool WriteNameAndLabel(const CTxDestination &address, const std::string &strName);
    bool EraseName(const CTxDestination &address);

    bool WritePurpose(const CTxDestination &address,
                      const std::string &purpose);
    bool ErasePurpose(const CTxDestination &address);

    bool WriteTx(const CWalletTx &wtx);
    bool EraseTx(uint256 hash);

    bool WriteMasterKey(unsigned int nID, const CMasterKey &kMasterKey);

    bool WriteCScript(const uint160 &hash, const CScript &redeemScript);

    bool WriteWatchOnly(const CScript &script, const CKeyMetadata &keymeta);
    bool EraseWatchOnly(const CScript &script);

    bool WriteBestBlock(const CBlockLocator &locator);
    bool ReadBestBlock(CBlockLocator &locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    bool ReadPool(int64_t nPool, CKeyPool &keypool);
    bool WritePool(int64_t nPool, const CKeyPool &keypool);
    bool ErasePool(int64_t nPool);

    bool ReadHDPubKeyTest(CPubKey& pubkey, CHDPubKey& hdpubkey);
    bool WriteHDPubKeyTest(CHDPubKey& hdpubkey);
    
    bool WriteHDPubKeys(const std::vector<CHDPubKey>& hdPubKey,
                        std::map<CKeyID, CKeyMetadata>& mapKeyMeta,
                        std::map<BKeyID, CKeyMetadata>& mapBLSKeyMeta);
    bool WritePool(const std::vector<CKeyPool> & keys, int64_t index);

    bool WriteMinVersion(int nVersion);

    /// This writes directly to the database, and will not update the CWallet's
    /// cached accounting entries!
    /// Use wallet.AddAccountingEntry instead, to write *and* update its caches.
    bool WriteAccountingEntry(const uint64_t nAccEntryNum,
                              const CAccountingEntry &acentry);

    /// Write destination data key,value tuple to database.
    bool WriteDestData(const CTxDestination &address, const std::string &key,
                       const std::string &value);
    /// Erase destination data tuple from wallet database.
    bool EraseDestData(const CTxDestination &address, const std::string &key);

    Amount GetAccountCreditDebit(const std::string &strAccount);
    void ListAccountCreditDebit(const std::string &strAccount,
                                std::list<CAccountingEntry> &acentries);

    DBErrors LoadWallet(CWallet *pwallet);
    DBErrors FindWalletTx(std::vector<TxId> &txIds,
                          std::vector<CWalletTx> &vWtx);
    DBErrors ZapWalletTx(std::vector<CWalletTx> &vWtx);
    DBErrors ZapSelectTx(std::vector<TxId> &txIdsIn,
                         std::vector<TxId> &txIdsOut);
    DBErrors FindLabelledAddresses(std::map<std::string, std::string>& mapLabels);
    
    /* Try to (very carefully!) recover wallet database (with a possible key
     * type filter) */
    static bool Recover(const fs::path &wallet_path, void *callbackDataIn,
                        bool (*recoverKVcallback)(void *callbackData,
                                                  CDataStream ssKey,
                                                  CDataStream ssValue),
                        std::string &out_backup_filename);
    /* Recover convenience-function to bypass the key filter callback, called
     * when verify fails, recovers everything */
    static bool Recover(const fs::path &wallet_path,
                        std::string &out_backup_filename);
    /* Recover filter (used as callback), will only let keys (cryptographical
     * keys) as KV/key-type pass through */
    static bool RecoverKeysOnlyFilter(void *callbackData, CDataStream ssKey,
                                      CDataStream ssValue);
    /* Function to determine if a certain KV/key-type is a key (cryptographical
     * key) type */
    static bool IsKeyType(const std::string &strType);
    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path &wallet_path,
                                  std::string &errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path &wallet_path,
                                   std::string &warningStr,
                                   std::string &errorStr);

    //! write the hdchain model (external chain child index counter)
    bool WriteCryptedHDChain(const CHDChain& chain);
    bool WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta);

    //! Begin a new transaction
    bool TxnBegin();
    //! Commit current transaction
    bool TxnCommit();
    //! Abort current transaction
    bool TxnAbort();
    //! Read wallet version
    bool ReadVersion(int &nVersion);
    //! Write wallet version
    bool WriteVersion(int nVersion);

private:
    BerkeleyBatch m_batch;
    WalletDatabase &m_database;
};

//! Compacts BDB state so that wallet.dat is self-contained (if there are
//! changes)
void MaybeCompactWalletDB();

#endif // BITCOIN_WALLET_WALLETDB_H
