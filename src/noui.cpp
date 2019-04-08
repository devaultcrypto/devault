// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "noui.h"

#include "ui_interface.h"
#include "util.h"

#include <cstdint>
#include <cstdio>
#include <string>

static void noui_ThreadSafeMessageBox(const std::string &message,
                                      const std::string &caption,
                                      unsigned int style, bool* b) {
    bool fSecure = style & CClientUIInterface::SECURE;
    style &= ~CClientUIInterface::SECURE;

    std::string strCaption;
    // Check for usage of predefined caption
    switch (style) {
        case CClientUIInterface::MSG_ERROR:
            strCaption += _("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            strCaption += _("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            strCaption += _("Information");
            break;
        default:
            // Use supplied caption (can be empty)
            strCaption += caption;
    }

    if (!fSecure) LogPrintf("%s: %s\n", strCaption, message);
    fprintf(stderr, "%s: %s\n", strCaption.c_str(), message.c_str());
    *b = false;
}

static void noui_ThreadSafeQuestion(const std::string & /* ignored interactive message */,
                                    const std::string &message, const std::string &caption,
                                    unsigned int style, bool* b) {
    noui_ThreadSafeMessageBox(message, caption, style, b);
}

static void noui_InitMessage(const std::string &message) {
    LogPrintf("init message: %s\n", message);
}

void noui_connect() {
    // Connect bitcoind signal handlers
    uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    uiInterface.ThreadSafeQuestion.connect(noui_ThreadSafeQuestion);
    uiInterface.InitMessage.connect(noui_InitMessage);
}
