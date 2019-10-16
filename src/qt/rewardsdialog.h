// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <qt/walletmodel.h>

#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

class CCoinControl;
class ClientModel;
class PlatformStyle;
class RewardsEntry;
class SendCoinsRecipient;

namespace Ui {
class RewardsDialog;
}

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending DVT */
class RewardsDialog : public QDialog {
    Q_OBJECT

public:
    explicit RewardsDialog(const PlatformStyle *platformStyle,
                             QWidget *parent = nullptr);
    ~RewardsDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

    /**
     * Set up the tab chain manually, as Qt messes up the tab chain by default
     * in some cases (issue
     * https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);

public Q_SLOTS:
    void clear();
    void reject() override;
    void accept() override;
    RewardsEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(const interfaces::WalletBalances &balances);
    void setUnvestedBalance(const interfaces::WalletBalances &balances);

Q_SIGNALS:
    void coinsSent(const uint256 &txid);

private:
    Ui::RewardsDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    bool fNewRecipientAllowed;
    bool fFeeMinimized;
    const PlatformStyle *platformStyle;

    // Process WalletModel::RewardsReturn and generate a pair consisting of a
    // message and message flags for use in Q_EMIT message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void
    processRewardsReturn(const WalletModel::SendCoinsReturn &rewardsReturn,
                           const QString &msgArg = QString());
    void minimizeFeeSection(bool fMinimize);
    void updateFeeMinimizedLabel();
    // Update the passed in CRewardControl with state from the GUI
    void updateRewardControlState(CCoinControl &ctrl);

private Q_SLOTS:
    void on_sendButton_clicked();
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void removeEntry(RewardsEntry *entry);
    void useAvailableBalance(RewardsEntry *entry);
    void updateDisplayUnit();
    void rewardControlFeatureChanged(bool);
    void rewardControlButtonClicked();
    void rewardControlUpdateLabels();

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message,
                 unsigned int style);
};

#define SEND_CONFIRM_DELAY 3

class RewardConfirmationDialog : public QMessageBox {
    Q_OBJECT

public:
    RewardConfirmationDialog(const QString &title, const QString &text,
                           int secDelay = SEND_CONFIRM_DELAY,
                           QWidget *parent = nullptr);
    int exec();

private Q_SLOTS:
    void countDown();
    void updateYesButton();

private:
    QAbstractButton *yesButton;
    QTimer countDownTimer;
    int secDelay;
};

