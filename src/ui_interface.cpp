// Copyright (c) 2010-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ui_interface.h>
#include <util/system.h>

CClientUIInterface uiInterface;


struct UISignals {
  /*
    boost::signals2::signal<CClientUIInterface::ThreadSafeMessageBoxSig,
                            boost::signals2::last_value<bool>>
        ThreadSafeMessageBox;
    boost::signals2::signal<CClientUIInterface::ThreadSafeQuestionSig,
                            boost::signals2::last_value<bool>>
        ThreadSafeQuestion;
    boost::signals2::signal<CClientUIInterface::InitMessageSig> InitMessage;
    boost::signals2::signal<CClientUIInterface::NotifyNumConnectionsChangedSig>
        NotifyNumConnectionsChanged;
    boost::signals2::signal<CClientUIInterface::NotifyNetworkActiveChangedSig>
        NotifyNetworkActiveChanged;
    boost::signals2::signal<CClientUIInterface::NotifyAlertChangedSig>
        NotifyAlertChanged;
  */
    boost::signals2::signal<CClientUIInterface::LoadWalletSig> LoadWallet;
  /*
    boost::signals2::signal<CClientUIInterface::ShowProgressSig> ShowProgress;
    boost::signals2::signal<CClientUIInterface::NotifyBlockTipSig>
        NotifyBlockTip;
    boost::signals2::signal<CClientUIInterface::NotifyHeaderTipSig>
        NotifyHeaderTip;
    boost::signals2::signal<CClientUIInterface::BannedListChangedSig>
        BannedListChanged;
  */
} g_ui_signals;

#define ADD_SIGNALS_IMPL_WRAPPER(signal_name)                                  \
    boost::signals2::connection CClientUIInterface::signal_name##_connect(     \
        std::function<signal_name##Sig> fn) {                                  \
        return g_ui_signals.signal_name.connect(fn);                           \
    }                                                                          \
    void CClientUIInterface::signal_name##_disconnect(                         \
        std::function<signal_name##Sig> fn) {                                  \
        return g_ui_signals.signal_name.disconnect(&fn);                       \
    }

ADD_SIGNALS_IMPL_WRAPPER(LoadWallet);

void CClientUIInterface::LoadWallet(
    std::unique_ptr<interfaces::Wallet> &wallet) {
    return g_ui_signals.LoadWallet(wallet);
}

bool InitError(const std::string &str) {
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

bool ShowSeedPhrase(const std::string &str) {
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_SEED);
    return false;
}

void InitWarning(const std::string &str) {
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
}

std::string AmountHighWarn(const std::string &optname) {
    return strprintf(_("%s is set very high!"), optname);
}

std::string AmountErrMsg(const char *const optname,
                         const std::string &strValue) {
    return strprintf(_("Invalid amount for -%s=<amount>: '%s'"), optname,
                     strValue);
}
