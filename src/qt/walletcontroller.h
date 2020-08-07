// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETCONTROLLER_H
#define BITCOIN_QT_WALLETCONTROLLER_H

#include <qt/walletmodel.h>
#include <sync.h>

#include <QMutex>

#include <list>
#include <memory>
#include <vector>

class OptionsModel;
class PlatformStyle;

namespace interfaces {
class Handler;
class Node;
} // namespace interfaces

/**
 * Controller between interfaces::Node, WalletModel instances and the GUI.
 */
class WalletController : public QObject {
    Q_OBJECT

    WalletModel *getOrCreateWallet(std::unique_ptr<interfaces::Wallet> wallet);
    void removeAndDeleteWallet(WalletModel *wallet_model);

public:
    WalletController(interfaces::Node &node,
                     const PlatformStyle *platform_style,
                     OptionsModel *options_model, QObject *parent);
    ~WalletController();

    std::vector<WalletModel *> getWallets() const;

private Q_SLOTS:
    void addWallet(WalletModel *wallet_model);

Q_SIGNALS:
    void walletAdded(WalletModel *wallet_model);
    void walletRemoved(WalletModel *wallet_model);

    void coinsSent(WalletModel *wallet_model, SendCoinsRecipient recipient,
                   QByteArray transaction);

private:
    interfaces::Node &m_node;
    const PlatformStyle *const m_platform_style;
    OptionsModel *const m_options_model;
    mutable QMutex m_mutex;
    std::vector<WalletModel *> m_wallets;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;
};

#endif // BITCOIN_QT_WALLETCONTROLLER_H
