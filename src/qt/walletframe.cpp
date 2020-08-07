// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <walletframe.h>
#include <walletmodel.h>

#include <bitcoingui.h>
#include <walletview.h>

#include <cassert>
#include <cstdio>

#include <QHBoxLayout>
#include <QLabel>

WalletFrame::WalletFrame(const PlatformStyle *_platformStyle,
                         const Config *configIn, BitcoinGUI *_gui)
    : QFrame(_gui), gui(_gui), platformStyle(_platformStyle), config(configIn) {
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0, 0, 0, 0);
    walletStack = new QStackedWidget(this);
    walletFrameLayout->setContentsMargins(0, 0, 0, 0);
    walletFrameLayout->addWidget(walletStack);

    QLabel *noWallet = new QLabel(tr("No wallet has been loaded."));
    noWallet->setAlignment(Qt::AlignCenter);
    walletStack->addWidget(noWallet);
}

WalletFrame::~WalletFrame() = default;

void WalletFrame::setClientModel(ClientModel *_clientModel) {
    this->clientModel = _clientModel;
}

bool WalletFrame::addWallet(WalletModel *walletModel) {
    if (!gui || !clientModel || !walletModel) {
        return false;
    }

    if (mapWalletViews.count(walletModel) > 0) {
        return false;
    }

    WalletView *walletView = new WalletView(platformStyle, config, this);
    walletView->setBitcoinGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    walletView->showOutOfSyncWarning(bOutOfSync);

    /* TODO we should goto the currently selected page once dynamically adding
     * wallets is supported */
    walletView->gotoOverviewPage();
    walletStack->addWidget(walletView);
    mapWalletViews[walletModel] = walletView;

    // Ensure a walletView is able to show the main window
    connect(walletView, &WalletView::showNormalIfMinimized,
            [this] { gui->showNormalIfMinimized(); });

    connect(walletView, &WalletView::outOfSyncWarningClicked, this,
            &WalletFrame::outOfSyncWarningClicked);

    return true;
}

bool WalletFrame::setCurrentWallet(WalletModel *wallet_model) {
    if (mapWalletViews.count(wallet_model) == 0) return false;

    WalletView *walletView = mapWalletViews.value(wallet_model);
    walletStack->setCurrentWidget(walletView);
    assert(walletView);
    walletView->updateWalletStatus();
    return true;
}

bool WalletFrame::removeWallet(WalletModel *wallet_model) {
    if (mapWalletViews.count(wallet_model) == 0) return false;

    WalletView *walletView = mapWalletViews.take(wallet_model);
    walletStack->removeWidget(walletView);
    return true;
}

void WalletFrame::removeAllWallets() {
    for (const auto& m : mapWalletViews)  walletStack->removeWidget(m);
    mapWalletViews.clear();
}

bool WalletFrame::handlePaymentRequest(const SendCoinsRecipient &recipient) {
    WalletView *walletView = currentWalletView();
    if (!walletView) return false;

    return walletView->handlePaymentRequest(recipient);
}

void WalletFrame::showOutOfSyncWarning(bool fShow) {
    bOutOfSync = fShow;
    for (const auto& m : mapWalletViews)  m->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage() {
    for (const auto& m : mapWalletViews)  m->gotoOverviewPage();
}

void WalletFrame::gotoRewardsPage() {
    for (const auto& m : mapWalletViews)  m->gotoRewardsPage();
} 
/*
void WalletFrame::gotoHistoryPage() {
    QMap<QString, WalletView *>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
} */

void WalletFrame::gotoReceiveCoinsPage() {
    for (const auto& m : mapWalletViews)  m->gotoReceiveCoinsPage();
}

void WalletFrame::gotoSendCoinsPage(QString addr) {
    for (const auto& m : mapWalletViews)  m->gotoSendCoinsPage(addr);
}

void WalletFrame::gotoSignMessageTab(QString addr) {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr) {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->gotoVerifyMessageTab(addr);
}

void WalletFrame::backupWallet() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->backupWallet();
}

void WalletFrame::changePassphrase() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->changePassphrase();
}

void WalletFrame::revealPhrase() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->revealPhrase();
}

void WalletFrame::sweeplegacy() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->sweeplegacy();
}

void WalletFrame::sweep() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->sweep();
}

void WalletFrame::unlockWallet() {
    WalletView *walletView = currentWalletView();
    if (walletView) {
        walletView->unlockWallet();
        walletView->updateWalletStatus();
    }
}

void WalletFrame::lockWallet() {
    WalletView *walletView = currentWalletView();
    if (walletView) {
        walletView->lockWallet();
        walletView->updateWalletStatus();
    }
}

void WalletFrame::usedSendingAddresses() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->usedSendingAddresses();
}

void WalletFrame::usedReceivingAddresses() {
    WalletView *walletView = currentWalletView();
    if (walletView) walletView->usedReceivingAddresses();
}

WalletView *WalletFrame::currentWalletView() const {
    return qobject_cast<WalletView *>(walletStack->currentWidget());
}

WalletModel *WalletFrame::currentWalletModel() const {
    WalletView *wallet_view = currentWalletView();
    return wallet_view ? wallet_view->getWalletModel() : nullptr;
}

void WalletFrame::outOfSyncWarningClicked() {
    Q_EMIT requestedSyncWarningInfo();
}
