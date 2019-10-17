// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rewardcontroldialog.h>
#include <ui_rewardcontroldialog.h>

#include <addresstablemodel.h>
#include <bitcoinunits.h>
#include <guiutil.h>
#include <optionsmodel.h>
#include <platformstyle.h>
#include <txmempool.h>
#include <walletmodel.h>
#include <dvtui.h>
#include <devault/rewards.h>
#include <checkcoins.h>
#include <config.h>

#include <dstencode.h>
#include <init.h>
#include <policy/policy.h>
#include <validation.h> // For mempool
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

#include <dstencode.h>
#include <interfaces/node.h>
#include <policy/policy.h>

#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QSettings>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>

QList<Amount> RewardControlDialog::payAmounts;
bool RewardControlDialog::fSubtractFeeFromAmount = true;

bool CRewardControlWidgetItem::operator<(const QTreeWidgetItem &other) const {
    int column = treeWidget()->sortColumn();
    if (column == RewardControlDialog::COLUMN_AMOUNT)
        return data(column, Qt::UserRole).toLongLong() <
               other.data(column, Qt::UserRole).toLongLong();
    return QTreeWidgetItem::operator<(other);
}

RewardControlDialog::RewardControlDialog(const PlatformStyle *_platformStyle,
                                     QWidget *parent)
    : QDialog(parent), ui(new Ui::RewardControlDialog), model(nullptr),
      platformStyle(_platformStyle) {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    // click on checkbox
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this,
            &RewardControlDialog::viewItemChanged);

    // click on header
    ui->treeWidget->header()->setSectionsClickable(true);
    connect(ui->treeWidget->header(), &QHeaderView::sectionClicked, this,
            &RewardControlDialog::headerSectionClicked);

    // ok button
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this,
            &RewardControlDialog::buttonBoxClicked);

    // (un)select all
    connect(ui->pushButtonSelectAll, &QPushButton::clicked, this,
            &RewardControlDialog::buttonSelectAllClicked);

     // unvested only
    connect(ui->pushButtonUnvesting, &QPushButton::clicked, this,
            &RewardControlDialog::buttonUnvestingClicked);

    // change coin control first column label due Qt4 bug.
    // see https://github.com/bitcoin/bitcoin/issues/5716
    ui->treeWidget->headerItem()->setText(COLUMN_CHECKBOX, QString());

    ui->treeWidget->setColumnWidth(COLUMN_CHECKBOX, 75);
    ui->treeWidget->setColumnWidth(COLUMN_AMOUNT, 110);
    ui->treeWidget->setColumnWidth(COLUMN_ADDRESS, 400);
    ui->treeWidget->setColumnWidth(COLUMN_REWARDAGE, 120);
    ui->treeWidget->setColumnWidth(COLUMN_NUMREWARDS, 100);
    // store transaction hash in this column, but don't show it
    ui->treeWidget->setColumnHidden(COLUMN_TXID, true);
    // store vout index in this column, but don't show it
    ui->treeWidget->setColumnHidden(COLUMN_VOUT_INDEX, true);
    ui->treeWidget->setColumnWidth(COLUMN_REWARDBLOCK, 100);
    ui->treeWidget->setColumnWidth(COLUMN_REWARDDATE, 100);

    // default view is sorted by amount desc
    sortView(COLUMN_AMOUNT, Qt::DescendingOrder);

    ui->percent->setMinimum(0);
    ui->percent->setMaximum(100);
    ui->percent->setValue(10); // 10% by default
    percent = ui->percent->value(); // get from spinbox....
    unvestingonly = false;
    
    connect(ui->percent, SIGNAL(valueChanged(int)), this,  SLOT(changePercent()));
    
    // restore list mode and sortorder as a convenience feature
    QSettings settings;
    if (settings.contains("nCoinControlSortColumn") &&
        settings.contains("nCoinControlSortOrder"))
        sortView(settings.value("nCoinControlSortColumn").toInt(),
                 (static_cast<Qt::SortOrder>(
                     settings.value("nRewardControlSortOrder").toInt())));
}

RewardControlDialog::~RewardControlDialog() {
    QSettings settings;
    settings.setValue("nCoinControlSortColumn", sortColumn);
    settings.setValue("nCoinControlSortOrder", (int)sortOrder);

    delete ui;
}

void RewardControlDialog::setModel(WalletModel *_model) {
    this->model = _model;

    if (_model && _model->getOptionsModel() && _model->getAddressTableModel()) {
        updateView();
        updateLabelLocked();
        RewardControlDialog::updateLabels(_model, this);
    }
}

// ok button
void RewardControlDialog::buttonBoxClicked(QAbstractButton *button) {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
        // closes the dialog
        done(QDialog::Accepted);
    }
}

// (un)select all
void RewardControlDialog::buttonSelectAllClicked() {
    Qt::CheckState state = Qt::Checked;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
        if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) !=
            Qt::Unchecked) {
            state = Qt::Unchecked;
            break;
        }
    }
    ui->treeWidget->setEnabled(false);
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
        if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) !=
            state) {
            ui->treeWidget->topLevelItem(i)->setCheckState(COLUMN_CHECKBOX,
                                                           state);
        }
    ui->treeWidget->setEnabled(true);
    if (state == Qt::Unchecked) {
        // just to be sure
        coinControl()->UnSelectAll();
    }
    unvestingonly = false;
    RewardControlDialog::updateLabels(model, this);
}
// (un)vesting all
void RewardControlDialog::buttonUnvestingClicked() {
    unvestingonly = !unvestingonly;
    updateView();
    RewardControlDialog::updateLabels(model, this);
}

// Percent changed
void RewardControlDialog::changePercent() {
    percent = ui->percent->value(); // get from spinbox....
    unvestingonly = false;
    updateView();
    RewardControlDialog::updateLabels(model, this);
}

// treeview: sort
void RewardControlDialog::sortView(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;
    ui->treeWidget->sortItems(column, order);
    ui->treeWidget->header()->setSortIndicator(sortColumn, sortOrder);
}

// treeview: clicked on header
void RewardControlDialog::headerSectionClicked(int logicalIndex) {
    // click on most left column -> do nothing
    if (logicalIndex == COLUMN_CHECKBOX) {
        ui->treeWidget->header()->setSortIndicator(sortColumn, sortOrder);
    } else {
        if (sortColumn == logicalIndex) {
            sortOrder =
                ((sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder
                                                   : Qt::AscendingOrder);
        } else {
            sortColumn = logicalIndex;
            // if label or address then default => asc, else default => desc
            sortOrder =
                ((sortColumn == COLUMN_ADDRESS)
                     ? Qt::AscendingOrder
                     : Qt::DescendingOrder);
        }

        sortView(sortColumn, sortOrder);
    }
}

// checkbox clicked by user
void RewardControlDialog::viewItemChanged(QTreeWidgetItem *item, int column) {
    // transaction hash is 64 characters (this means its a child node, so its
    // not a parent node in tree mode)
    if (column == COLUMN_CHECKBOX && item->text(COLUMN_TXID).length() == 64) {
        COutPoint outpoint = buildOutPoint(item);

        if (item->checkState(COLUMN_CHECKBOX) == Qt::Unchecked) {
            coinControl()->UnSelect(outpoint);
        } else if (item->isDisabled()) {
            // locked (this happens if "check all" through parent node)
            item->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        } else {
            coinControl()->Select(outpoint);
        }

        // selection changed -> update labels
        if (ui->treeWidget->isEnabled()) {
            // do not update on every click for (un)select all
            RewardControlDialog::updateLabels(model, this);
        }
    }
}

// shows count of locked unspent outputs
void RewardControlDialog::updateLabelLocked() {
    std::vector<COutPoint> vOutpts;
    model->wallet().listLockedCoins(vOutpts);
    if (vOutpts.size() > 0) {
        ui->labelLocked->setText(tr("(%1 locked)").arg(vOutpts.size()));
        ui->labelLocked->setVisible(true);
    } else {
        ui->labelLocked->setVisible(false);
    }
}

void RewardControlDialog::updateLabels(WalletModel *model, QDialog *dialog) {
    if (!model) {
        return;
    }

    // nPayAmount
    Amount nPayAmount = Amount::zero();
    bool fDust = false;
    CMutableTransaction txDummy;
    for (const Amount& amount : RewardControlDialog::payAmounts) {
        nPayAmount += amount;

        if (amount > Amount::zero()) {
            CTxOut txout(amount,
                         static_cast<CScript>(std::vector<uint8_t>(24, 0)));
            txDummy.vout.push_back(txout);
            fDust |= IsDust(txout, model->node().getDustRelayFee());
        }
    }

    Amount nAmount = Amount::zero();
    Amount nPayFee = Amount::zero();
    Amount nAfterFee = Amount::zero();
    Amount nChange = Amount::zero();
    unsigned int nBytes = 0;
    unsigned int nBytesInputs = 0;
    unsigned int nQuantity = 0;
    int nQuantityUncompressed = 0;

    std::vector<COutPoint> vRewardControl;
    coinControl()->ListSelected(vRewardControl);

    size_t i = 0;
    for (const auto &out : model->wallet().getCoins(vRewardControl)) {
        if (out.depth_in_main_chain < 0) {
            continue;
        }

        // unselect already spent, very unlikely scenario, this could happen
        // when selected are spent elsewhere, like rpc or another computer
        const COutPoint &outpt = vRewardControl[i++];
        if (out.is_spent) {
            coinControl()->UnSelect(outpt);
            continue;
        }

         
        // Quantity
        nQuantity++;

        // Amount
        nAmount += out.txout.nValue;

        // Bytes
        CTxDestination address;
        if (ExtractDestination(out.txout.scriptPubKey, address)) {
            CPubKey pubkey;
#ifdef HAVE_VARIANT
            CKeyID *keyid = &std::get<CKeyID>(address);
#else
            CKeyID *keyid = boost::get<CKeyID>(&address);
#endif
            if (keyid && model->wallet().getPubKey(*keyid, pubkey)) {
                nBytesInputs += (pubkey.IsCompressed() ? 148 : 180);
                if (!pubkey.IsCompressed()) {
                    nQuantityUncompressed++;
                }
            } else {
                // in all error cases, simply assume 148 here
                nBytesInputs += 148;
            }
        } else {
            nBytesInputs += 148;
        }
    }

    // calculation
    if (nQuantity > 0) {
        // Bytes
        // always assume +1 output for change here
        nBytes = nBytesInputs +
                 ((RewardControlDialog::payAmounts.size() > 0
                       ? RewardControlDialog::payAmounts.size() + 1
                       : 2) *
                  34) +
                 10;

        // in the subtract fee from amount case, we can tell if zero change
        // already and subtract the bytes, so that fee calculation afterwards is
        // accurate
        if (RewardControlDialog::fSubtractFeeFromAmount) {
            if (nAmount - nPayAmount == Amount::zero()) {
                nBytes -= 34;
            }
        }

        // Fee
        nPayFee = model->node().getMinimumFee(nBytes, *coinControl());

        if (nPayAmount > Amount::zero()) {
            nChange = nAmount - nPayAmount;
            if (!RewardControlDialog::fSubtractFeeFromAmount) {
                nChange -= nPayFee;
            }

            // Never create dust outputs; if we would, just add the dust to the
            // fee.
            if (nChange > Amount::zero() && nChange < MIN_CHANGE) {
                CTxOut txout(nChange,
                             static_cast<CScript>(std::vector<uint8_t>(24, 0)));
                if (IsDust(txout, model->node().getDustRelayFee())) {
                    nPayFee += nChange;
                    nChange = Amount::zero();
                    if (RewardControlDialog::fSubtractFeeFromAmount) {
                        // we didn't detect lack of change above
                        nBytes -= 34;
                    }
                }
            }

            if (nChange == Amount::zero() &&
                !RewardControlDialog::fSubtractFeeFromAmount) {
                nBytes -= 34;
            }
        }

        // after fee
        nAfterFee = std::max(nAmount - nPayFee, Amount::zero());
    }

    // actually update labels
    int nDisplayUnit = BitcoinUnits::DVT;
    if (model && model->getOptionsModel()) {
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();
    }

    QLabel *l1 = dialog->findChild<QLabel *>("labelRewardControlQuantity");
    QLabel *l2 = dialog->findChild<QLabel *>("labelRewardControlAmount");
    QLabel *l3 = dialog->findChild<QLabel *>("labelRewardControlFee");
    QLabel *l4 = dialog->findChild<QLabel *>("labelRewardControlAfterFee");
    QLabel *l5 = dialog->findChild<QLabel *>("labelRewardControlBytes");
    QLabel *l7 = dialog->findChild<QLabel *>("labelRewardControlLowOutput");

    // stats
    // Quantity
    l1->setText(QString::number(nQuantity));
    // Amount
    l2->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));
    // Fee
    l3->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee));
    // After Fee
    l4->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee));
    // Bytes
    l5->setText(((nBytes > 0) ? ASYMP_UTF8 : "") + QString::number(nBytes));
    // Dust
    l7->setText(fDust ? tr("yes") : tr("no"));
    // Change
    if (nPayFee > Amount::zero()) {
        l3->setText(ASYMP_UTF8 + l3->text());
        l4->setText(ASYMP_UTF8 + l4->text());
    }

    // turn label red when dust
    l7->setStyleSheet((fDust) ? "color:red;" : "");

    // tool tips
    QString toolTipDust =
        tr("This label turns red if any recipient receives an amount smaller "
           "than the current dust threshold.");

    // how many satoshis the estimated fee can vary per byte we guess wrong
    double dFeeVary = (nBytes != 0) ? double(nPayFee.toInt()) / nBytes : 0;

    QString toolTip4 =
        tr("Can vary +/- %1 satoshi(s) per input.").arg(dFeeVary);

    l3->setToolTip(toolTip4);
    l4->setToolTip(toolTip4);
    l7->setToolTip(toolTipDust);
    dialog->findChild<QLabel *>("labelRewardControlFeeText")
        ->setToolTip(l3->toolTip());
    dialog->findChild<QLabel *>("labelRewardControlAfterFeeText")
        ->setToolTip(l4->toolTip());
    dialog->findChild<QLabel *>("labelRewardControlBytesText")
        ->setToolTip(l5->toolTip());
    dialog->findChild<QLabel *>("labelRewardControlLowOutputText")
        ->setToolTip(l7->toolTip());

    // Insufficient funds
    QLabel *label = dialog->findChild<QLabel *>("labelRewardControlInsuffFunds");
    if (label) {
        label->setVisible(nChange < Amount::zero());
    }
}

CCoinControl *RewardControlDialog::coinControl() {
    static CCoinControl coin_control;
    return &coin_control;
}

COutPoint RewardControlDialog::buildOutPoint(const QTreeWidgetItem *item) {
    TxId txid;
    txid.SetHex(item->text(COLUMN_TXID).toStdString());
    return COutPoint(txid, item->text(COLUMN_VOUT_INDEX).toUInt());
}

void RewardControlDialog::updateView() {
    if (!model || !model->getOptionsModel() || !model->getAddressTableModel()) {
        return;
    }

    ui->treeWidget->clear();
    // performance, otherwise updateLabels would be called for every checked
    // checkbox
    ui->treeWidget->setEnabled(false);
    QFlags<Qt::ItemFlag> flgCheckbox =
        Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;

    int nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    // For RewardBlock/Date
    std::vector<CRewardValue> rewards = prewards->GetOrderedRewards();
    auto &config = const_cast<Config &>(GetConfig());

    std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  
    const int nMinRewardInCoins = config.GetChainParams().GetConsensus().nMinReward.toIntCoins();
    const int nMinBlocks = config.GetChainParams().GetConsensus().nMinRewardBlocks;
    const int nBlocksPerYear = config.GetChainParams().GetConsensus().nBlocksPerYear;
    const int nPowTargetSpacing = config.GetChainParams().GetConsensus().nPowTargetSpacing;
    const int nMaxYearIndex = config.GetChainParams().GetConsensus().nPerCentPerYear.size()-1;

    // Minimum balances for 1 month payout given current reward % of 15,12,9,7,5
    std::vector<double> nMinBalance;
    for (auto pc :  config.GetChainParams().GetConsensus().nPerCentPerYear) {
        double val = nMinRewardInCoins * 12 * 100.0 / pc;
        nMinBalance.push_back(val);
    }


    for (const auto &coins : model->wallet().listCoins()) {
        CRewardControlWidgetItem *itemWalletAddress =
            new CRewardControlWidgetItem();
        itemWalletAddress->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        QString sWalletAddress =
            QString::fromStdString(EncodeDestination(coins.first));
        QString sWalletLabel =
            model->getAddressTableModel()->labelForAddress(sWalletAddress);
        if (sWalletLabel.isEmpty()) {
            sWalletLabel = tr("(no label)");
        }

        Amount nSum = Amount::zero();
        int nChildren = 0;
        for (const auto &outpair : coins.second) {
            const COutPoint &output = std::get<0>(outpair);
            const interfaces::WalletTxOut &out = std::get<1>(outpair);
            nSum += out.txout.nValue;
            nChildren++;


            CRewardControlWidgetItem *itemOutput;
            itemOutput = new CRewardControlWidgetItem(ui->treeWidget);
            itemOutput->setFlags(flgCheckbox);
            itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

            // address
            CTxDestination outputAddress;
            QString sAddress = "";
            if (ExtractDestination(out.txout.scriptPubKey, outputAddress)) {
                sAddress =
                    QString::fromStdString(EncodeDestination(outputAddress));

                // if listMode or change => show bitcoin address. In tree mode,
                // address is not shown again for direct wallet address outputs
                itemOutput->setText(COLUMN_ADDRESS, sAddress);
            }

            // amount
            itemOutput->setText(
                COLUMN_AMOUNT,
                BitcoinUnits::format(nDisplayUnit, out.txout.nValue));
            // padding so that sorting works correctly
            itemOutput->setData(
                COLUMN_AMOUNT, Qt::UserRole,
                QVariant(qlonglong(out.txout.nValue.toInt())));

            // transaction id
            itemOutput->setText(
                COLUMN_TXID, QString::fromStdString(output.GetTxId().GetHex()));

            // vout index
            itemOutput->setText(COLUMN_VOUT_INDEX,
                                QString::number(output.GetN()));


            CRewardValue rewardval;
            bool setCheckbox = false;
            if (prewardsdb->GetReward(output, rewardval)) {
              ///
              auto Height = chainActive.Tip()->nHeight;
              auto nMinBlock = Params().GetConsensus().nMinRewardBlocks;
              // Quantize to integer % and limit to 99% so that sorting will work better
              auto payAge = int((100.0*(Height - rewardval.GetHeight()))/nMinBlock);
              if (payAge > 99) payAge = 99;
              auto payCount = rewardval.GetPayCount();
              itemOutput->setText(COLUMN_REWARDAGE, QString::number(payAge).rightJustified(2,'0') + QString("%"));
              itemOutput->setText(COLUMN_NUMREWARDS, QString::number(payCount));

              if (!unvestingonly) {
                  if (payAge > percent) {
                      setCheckbox = false;
                  } else
                      setCheckbox = true;
              } else {
                  setCheckbox = false;
              }
              
              int nNumOlder = 0;
              for (auto& inner_val : rewards) {
                  if ((inner_val.GetHeight() < rewardval.GetHeight()) && inner_val.IsActive()) nNumOlder++;
              }

              // Use nMinBlocks unless there are more older candidates that need to get paid out
              int nYear = rewardval.GetHeight()/nBlocksPerYear;
              if (nYear > nMaxYearIndex) nYear = nMaxYearIndex;
              // Check if balance is below minimum required for 1 month payout,
              // if it is, then extend the needed number of blocks for payout
              int neededBlocks = nMinBlocks;
              if (rewardval.GetValue().toIntCoins() < (int)nMinBalance[nYear]) {
                  neededBlocks *= std::ceil(nMinBalance[nYear]/rewardval.GetValue().toIntCoins());
              }
              // In event of very large number of older payouts that need to be made,
              // extend by even more blocks
              nNumOlder = std::max(nNumOlder,neededBlocks);
              int payoutHeight = rewardval.GetHeight()+nNumOlder;
              int blocksNeeded = payoutHeight - chainActive.Height();
              // Use blocksNeeded to estimate date
              std::time_t nexttime = cftime + blocksNeeded*nPowTargetSpacing;
                
              itemOutput->setText(COLUMN_REWARDBLOCK, QString::number(payoutHeight));
              itemOutput->setText(COLUMN_REWARDDATE, QString::fromStdString(FormatISO8601Date(nexttime)));

              
            } else {
              itemOutput->setText(COLUMN_REWARDAGE, tr("N.A."));
              itemOutput->setText(COLUMN_NUMREWARDS, tr("N.A."));
              itemOutput->setText(COLUMN_REWARDBLOCK, tr("N.A."));
              itemOutput->setText(COLUMN_REWARDDATE, tr("N.A."));
              setCheckbox = true;
            }
            
            // disable locked coins
            if (model->wallet().isLockedCoin(output)) {
                // just to be sure
                coinControl()->UnSelect(output);
                itemOutput->setDisabled(true);
                itemOutput->setIcon(
                    COLUMN_CHECKBOX,
                    platformStyle->SingleColorIcon(":/icons/lock_closed"));
                itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
            } else {
                if (setCheckbox) {
                    coinControl()->Select(output);
                    itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Checked);
                } else {
                    coinControl()->UnSelect(output);
                    itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
                }
            }
        }
    }

    // sort view
    sortView(sortColumn, sortOrder);
    ui->treeWidget->setEnabled(true);

    //std::string s = checkcoins(model->wallet().listCoins());
    //QMessageBox::warning(this, tr("OK"), QString::fromStdString(s));
}

