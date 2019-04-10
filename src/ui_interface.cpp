// Copyright (c) 2010-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ui_interface.h"
#include "util.h"

CClientUIInterface uiInterface;

bool InitError(const std::string &str) {
    bool fRet;
    uiInterface.ThreadSafeMessageBox.fire(str, "", CClientUIInterface::MSG_ERROR, &fRet);
    return false;
}

bool ShowSeedPhrase(const std::string &str) {
    bool fRet;
    uiInterface.ThreadSafeMessageBox.fire(str, "", CClientUIInterface::MSG_SEED, &fRet);
    return false;
}

void InitWarning(const std::string &str) {
    bool fRet;
    uiInterface.ThreadSafeMessageBox.fire(str, "", CClientUIInterface::MSG_WARNING, &fRet);
}

std::string AmountHighWarn(const std::string &optname) {
    return strprintf(_("%s is set very high!"), optname);
}

std::string AmountErrMsg(const char *const optname,
                         const std::string &strValue) {
    return strprintf(_("Invalid amount for -%s=<amount>: '%s'"), optname,
                     strValue);
}
