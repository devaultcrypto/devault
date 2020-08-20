// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <config.h>
#include <net.h>
#include <scheduler.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/rpcwallet.h>
#include <wallet/rpcdump.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>
#include <walletinitinterface.h>

class WalletInit : public WalletInitInterface {
public:
    //! Was the wallet component compiled in.
    bool HasWalletSupport() const override {return true;}

    //! Return the wallets help message.
    void AddWalletOptions() const override;
    //std::string GetHelpString(bool showDebug) const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    bool CheckIfWalletExists(const CChainParams &chainParams) const override;

    //! Load wallet databases.
  /*
    bool Open(const CChainParams &chainParams, interfaces::Chain &chain, const SecureString& walletPassphrase,
              const std::vector<std::string>& words, bool use_bls) const override;
  */
    //! Add wallets that should be opened to list of init interfaces.
    void Construct(InitInterfaces &interfaces) const override;


};

static WalletInit g_wallet_init;
const WalletInitInterface &g_wallet_init_interface = g_wallet_init;

void WalletInit::AddWalletOptions() const {
    gArgs.AddArg("-disablewallet",
                 "Do not load the wallet and disable wallet RPC calls",
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-bypasspassword",
               "Set a default password of \"\" to bypass interactively setting it)",
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-keypool=<n>",
                 strprintf("Set key pool size to <n> (default: %u)",
                           DEFAULT_KEYPOOL_SIZE),
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-fallbackfee=<amt>",
                 strprintf("A fee rate (in %s/kB) that will be used when fee "
                             "estimation has insufficient data (default: %s)",
                           CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)),
                 false, OptionsCategory::WALLET);
    gArgs.AddArg(
        "-paytxfee=<amt>",
        strprintf(
            "Fee (in %s/kB) to add to transactions you send (default: %s)",
            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())),
        false, OptionsCategory::WALLET);
    gArgs.AddArg(
        "-rescan",
        "Rescan the block chain for missing wallet transactions on startup",
        false, OptionsCategory::WALLET);
    gArgs.AddArg(
        "-salvagewallet",
        "Attempt to recover private keys from a corrupt wallet on startup",
        false, OptionsCategory::WALLET);

    gArgs.AddArg("-spendzeroconfchange",
                 strprintf("Spend unconfirmed change when sending "
                             "transactions (default: %d)",
                           DEFAULT_SPEND_ZEROCONF_CHANGE),
                 false, OptionsCategory::WALLET);
    
    gArgs.AddArg("-upgradewallet",
                 "Upgrade wallet to latest format on startup", false,
                 OptionsCategory::WALLET);
    gArgs.AddArg("-wallet=<file>",
                 "Specify wallet file (within data directory) " +
                     strprintf("(default: %s)", DEFAULT_WALLET_DAT),
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletbroadcast",
                 "Make the wallet broadcast transactions " +
                     strprintf("(default: %d)", DEFAULT_WALLETBROADCAST),
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletdir=<dir>",
                 "Specify directory to hold wallets (default: "
                   "<datadir>/wallets if it exists, otherwise <datadir>)",
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletnotify=<cmd>",
                 "Execute command when a wallet transaction changes (%s in "
                   "cmd is replaced by TxID)",
                 false, OptionsCategory::WALLET);
    gArgs.AddArg("-zapwallettxes=<mode>",
                 "Delete all wallet transactions and only recover those "
                   "parts of the blockchain through -rescan on startup"
                     "(1 = keep tx meta data e.g. account owner and payment "
                       "request information, 2 = drop tx meta data)",
                 false, OptionsCategory::WALLET);

    gArgs.AddArg("-dblogsize=<n>",
                 strprintf("Flush wallet database activity from memory to disk "
                           "log every <n> megabytes (default: %u)",
                           DEFAULT_WALLET_DBLOGSIZE),
                 true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg(
        "-flushwallet",
        strprintf("Run a thread to flush wallet periodically (default: %d)",
                  DEFAULT_FLUSHWALLET),
        true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-privdb",
                 strprintf("Sets the DB_PRIVATE flag in the wallet db "
                           "environment (default: %d)",
                           DEFAULT_WALLET_PRIVDB),
                 true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-walletrejectlongchains",
                 strprintf("Wallet will not create transactions that violate "
                             "mempool chain limits (default: %d)",
                           DEFAULT_WALLET_REJECT_LONG_CHAINS),
                 true, OptionsCategory::WALLET_DEBUG_TEST);
}

bool WalletInit::ParameterInteraction() const {
    ::minRelayTxFee = GetConfig().GetMinFeePerKB();
    gArgs.SoftSetArg("-wallet", DEFAULT_WALLET_DAT);
    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;
    if (is_multiwallet) {
      return InitError(
                       strprintf("%s is only allowed with a single wallet file",
                                 "-wallet"));
    }

    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return true;
    }

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) &&
        gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting "
                  "-walletbroadcast=0\n",
                  __func__);
    }

    if (gArgs.GetBoolArg("-salvagewallet", false) &&
        gArgs.SoftSetBoolArg("-rescan", true)) {
        if (is_multiwallet) {
            return InitError(
                strprintf("%s is only allowed with a single wallet file",
                          "-salvagewallet"));
        }
        // Rewrite just private keys: rescan to find transactions
        LogPrintf("%s: parameter interaction: -salvagewallet=1 -> setting "
                  "-rescan=1\n",
                  __func__);
    }

    int zapwallettxes = gArgs.GetArg("-zapwallettxes", 0);
    // -zapwallettxes implies dropping the mempool on startup
    if (zapwallettxes != 0 && gArgs.SoftSetBoolArg("-persistmempool", false)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting "
                  "-persistmempool=0\n",
                  __func__, zapwallettxes);
    }

    // -zapwallettxes implies a rescan
    if (zapwallettxes != 0) {
        if (is_multiwallet) {
            return InitError(
                strprintf("%s is only allowed with a single wallet file",
                          "-zapwallettxes"));
        }
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting "
                      "-rescan=1\n",
                      __func__, zapwallettxes);
        }
        LogPrintf("%s: parameter interaction: -zapwallettxes=<mode> -> setting "
                  "-rescan=1\n",
                  __func__);
    }

    if (is_multiwallet) {
        if (gArgs.GetBoolArg("-upgradewallet", false)) {
            return InitError(
                strprintf("%s is only allowed with a single wallet file",
                          "-upgradewallet"));
        }
    }

    if (gArgs.GetBoolArg("-sysperms", false)) {
        return InitError("-sysperms is not allowed in combination with enabled "
                         "wallet functionality");
    }

    if (gArgs.GetArg("-prune", 0) && gArgs.GetBoolArg("-rescan", false)) {
        return InitError(
            _("Rescans are not possible in pruned mode. You will need to use "
              "-reindex which will download the whole blockchain again."));
    }

    if (minRelayTxFee.GetFeePerK() > HIGH_TX_FEE_PER_KB) {
        InitWarning(
            AmountHighWarn("-minrelaytxfee") + " " +
            _("The wallet will avoid paying less than the minimum relay fee."));
    }

    if (gArgs.IsArgSet("-fallbackfee")) {
        Amount nFeePerK = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK)) {
            return InitError(
                strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"),
                          gArgs.GetArg("-fallbackfee", "")));
        }

        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            InitWarning(AmountHighWarn("-fallbackfee") + " " +
                        _("This is the transaction fee you may pay when fee "
                          "estimates are not available."));
        }

        CWallet::fallbackFee = CFeeRate(nFeePerK);
    }

    if (gArgs.IsArgSet("-paytxfee")) {
        Amount nFeePerK = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK)) {
            return InitError(
                AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", "")));
        }

        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            InitWarning(AmountHighWarn("-paytxfee") + " " +
                        _("This is the transaction fee you will pay if you "
                          "send a transaction."));
        }

        payTxFee = CFeeRate(nFeePerK, 1000);
        if (payTxFee < minRelayTxFee) {
            return InitError(strprintf(
                _("Invalid amount for -paytxfee=<amount>: '%s' (must "
                  "be at least %s)"),
                gArgs.GetArg("-paytxfee", ""), minRelayTxFee.ToString()));
        }
    }

    if (gArgs.IsArgSet("-maxtxfee")) {
        Amount nMaxFee = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee)) {
            return InitError(
                AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", "")));
        }

        if (nMaxFee > HIGH_MAX_TX_FEE) {
            InitWarning(_("-maxtxfee is set very high! Fees this large could "
                          "be paid on a single transaction."));
        }

        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < minRelayTxFee) {
            return InitError(strprintf(
                _("Invalid amount for -maxtxfee=<amount>: '%s' (must "
                  "be at least the minrelay fee of %s to prevent "
                  "stuck transactions)"),
                gArgs.GetArg("-maxtxfee", ""), minRelayTxFee.ToString()));
        }
    }

    bSpendZeroConfChange =
        gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);

    g_address_type = OutputType::LEGACY;
    
    return true;
}

bool VerifyWallets(const CChainParams &chainParams,
                   interfaces::Chain &chain,
                   const std::vector<std::string>& wallet_files) {
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return true;
    }

    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        if (!fs::exists(wallet_dir)) {
            return InitError(
                strprintf(_("Specified -walletdir \"%s\" does not exist"),
                          wallet_dir.string()));
        } else if (!fs::is_directory(wallet_dir)) {
            return InitError(
                strprintf(_("Specified -walletdir \"%s\" is not a directory"),
                          wallet_dir.string()));
        } else if (!wallet_dir.is_absolute()) {
            return InitError(
                strprintf(_("Specified -walletdir \"%s\" is a relative path"),
                          wallet_dir.string()));
        }
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    uiInterface.InitMessage(_("Verifying wallet(s)..."));

    // Parameter interaction code should have thrown an error if -salvagewallet
    // was enabled with more than wallet file, so the wallet_files size check
    // here should have no effect.
    bool salvage_wallet =
        gArgs.GetBoolArg("-salvagewallet", false) && wallet_files.size() <= 1;

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        WalletLocation location(wallet_file);
        
        if (!wallet_paths.insert(location.GetPath()).second) {
            return InitError(strprintf(_("Error loading wallet %s. Duplicate "
                                         "-wallet filename specified."),
                                       wallet_file));
        }

        std::string error_string;
        std::string warning_string;
        bool verify_success = CWallet::Verify(chainParams, chain, location, salvage_wallet,
                                            error_string, warning_string);
        if (!error_string.empty()) {
          InitError(error_string);
        }
        if (!warning_string.empty()) {
          InitWarning(warning_string);
        }
        if (!verify_success) {
          return false;
        }
    }

    return true;
}

bool WalletInit::CheckIfWalletExists(const CChainParams &chainParams) const {
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return true; //???
    }

    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        if (!fs::exists(wallet_dir)) {
          return false;
        } else if (!fs::is_directory(wallet_dir)) {
          return false;
        } else if (!wallet_dir.is_absolute()) {
          return false;
        }
    }

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    std::vector<std::string> wallet_files = gArgs.GetArgs("-wallet");
    for (const auto& wallet_file : wallet_files) {

        fs::path added_dir = BaseParams().DataDir();
        fs::path wallet_path = GetWalletPathNoCreate(added_dir, wallet_file);
        
        if (fs::exists(wallet_path)) {
          return true;
        }
    }

  return false;
}

void WalletInit::Construct(InitInterfaces &interfaces) const {
  if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
    LogPrintf("Wallet disabled!\n");
            return;
  }
    gArgs.SoftSetArg("-wallet", "");
    interfaces.chain_clients.emplace_back(interfaces::MakeWalletClient(
        *interfaces.chain, gArgs.GetArgs("-wallet")));
}

bool LoadWallets(const CChainParams &chainParams, interfaces::Chain &chain,
                 const std::vector<std::string> &wallet_files,
                 const SecureString& walletPassphrase,
                 const std::vector<std::string>& words, bool use_bls
                 ) {
  
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;
    if (is_multiwallet) {
      return InitError(
                       strprintf("%s is only allowed with a single wallet file",
                                 "-wallet"));
    }

    for (const std::string &walletFile : wallet_files) {
        WalletFlag flags;
        if (use_bls) { flags.SetBLS();
        } else { flags.SetLEGACY();}
        std::shared_ptr<CWallet> pwallet = CWallet::CreateWalletFromFile(
                                                                         chainParams, chain, WalletLocation(walletFile),
                                                                         walletPassphrase, words, flags);
      if (!pwallet) {
        return false;
      }
      AddWallet(pwallet);
    }

    return true;
}

/*
bool LoadWallets(const CChainParams &chainParams, interfaces::Chain &chain,
                 const std::vector<std::string> &wallet_files) {
    for (const std::string &walletFile : wallet_files) {
        std::shared_ptr<CWallet> pwallet = CWallet::CreateWalletFromFile(
            chainParams, chain, WalletLocation(walletFile));
        if (!pwallet) {
            return false;
        }
        AddWallet(pwallet);
    }

    return true;
}
*/


void StartWallets(CScheduler &scheduler) {
    for (const std::shared_ptr<CWallet> &pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }
}

void FlushWallets() {
    for (const std::shared_ptr<CWallet> &pwallet : GetWallets()) {
        pwallet->Flush(false);
    }
}

void StopWallets() {
    for (const std::shared_ptr<CWallet> &pwallet : GetWallets()) {
        pwallet->Flush(true);
    }
}

void UnloadWallets() {
    auto wallets = GetWallets();
    while (!wallets.empty()) {
        auto wallet = wallets.back();
        wallets.pop_back();
        RemoveWallet(wallet);
        UnloadWallet(std::move(wallet));
    }
}
