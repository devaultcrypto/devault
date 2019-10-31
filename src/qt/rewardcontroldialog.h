// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <amount.h>

#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QTreeWidgetItem>

class PlatformStyle;
class WalletModel;

class CCoinControl;
class COutPoint;

namespace Ui {
class RewardControlDialog;
}

#define ASYMP_UTF8 "\xE2\x89\x88"

class CRewardControlWidgetItem : public QTreeWidgetItem {
public:
    explicit CRewardControlWidgetItem(QTreeWidget *parent, int type = Type)
        : QTreeWidgetItem(parent, type) {}
    explicit CRewardControlWidgetItem(int type = Type) : QTreeWidgetItem(type) {}
    explicit CRewardControlWidgetItem(QTreeWidgetItem *parent, int type = Type)
        : QTreeWidgetItem(parent, type) {}

    bool operator<(const QTreeWidgetItem &other) const override;
};

class RewardControlDialog : public QDialog {
    Q_OBJECT

public:
    explicit RewardControlDialog(const PlatformStyle *platformStyle,
                               QWidget *parent = nullptr);
    ~RewardControlDialog();

    void setModel(WalletModel *model);

    // static because also called from sendcoinsdialog
    static void updateLabels(WalletModel *, QDialog *);

    static QList<Amount> payAmounts;
    static CCoinControl *coinControl();
    static bool fSubtractFeeFromAmount;

private:
    Ui::RewardControlDialog *ui;
    WalletModel *model;
    int sortColumn;
    Qt::SortOrder sortOrder;
    QMenu *contextMenu;
    QTreeWidgetItem *contextMenuItem;
    int percent;
    bool unvestingonly;
    
    const PlatformStyle *platformStyle;

    void sortView(int, Qt::SortOrder);
    void updateView();

    enum {
        COLUMN_CHECKBOX = 0,
        COLUMN_AMOUNT,
        COLUMN_ADDRESS,
        COLUMN_TXID,
        COLUMN_VOUT_INDEX,
        COLUMN_REWARDAGE,
        COLUMN_NUMREWARDS,
        COLUMN_REWARDBLOCK,
        COLUMN_REWARDDATE,
    };
    friend class CRewardControlWidgetItem;

    static COutPoint buildOutPoint(const QTreeWidgetItem *item);

private Q_SLOTS:
    void showMenu(const QPoint &);
    void viewItemChanged(QTreeWidgetItem *, int);
    void headerSectionClicked(int);
    void buttonBoxClicked(QAbstractButton *);
    void buttonSelectAllClicked();
    void buttonUnvestingClicked();
    void updateLabelLocked();
    void changePercent();
    void copyAddress();
};

