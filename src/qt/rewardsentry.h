// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <qt/walletmodel.h>

#include <QStackedWidget>

class WalletModel;
class PlatformStyle;

namespace Ui {
class RewardsEntry;
}

/**
 * A single entry in the dialog for sending DVT.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class RewardsEntry : public QStackedWidget {
    Q_OBJECT

public:
    explicit RewardsEntry(const PlatformStyle *platformStyle,
                            QWidget *parent = nullptr);
    ~RewardsEntry();

    void setModel(WalletModel *model);
    bool validate(interfaces::Node &node);
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);
    void setAmount(const Amount amount);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default
     * in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void removeEntry(RewardsEntry *entry);

private Q_SLOTS:
    void deleteClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Amount m_amount;
    SendCoinsRecipient recipient;
    Ui::RewardsEntry *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;
    
    bool updateLabel(const QString &address);
};

