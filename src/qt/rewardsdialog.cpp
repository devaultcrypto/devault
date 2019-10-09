// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rewardsdialog.h>
#include <ui_rewardsdialog.h>

#include <addresstablemodel.h>
#include <bitcoinunits.h>
#include <clientmodel.h>
#include <rewardcontroldialog.h>
#include <guiutil.h>
#include <optionsmodel.h>
#include <platformstyle.h>
#include <rewardsentry.h>
#include <walletmodel.h>
#include <dvtui.h>

#include <chainparams.h>
#include <dstencode.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <validation.h> // mempool and minRelayTxFee
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>

#include <interfaces/node.h>

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>

RewardsDialog::RewardsDialog(const PlatformStyle *_platformStyle,
                                 QWidget *parent)
    : QDialog(parent), ui(new Ui::RewardsDialog), clientModel(nullptr), model(nullptr),
      fNewRecipientAllowed(true), fFeeMinimized(false),
      platformStyle(_platformStyle) {
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    if (!_platformStyle->getImagesOnButtons()) {
        ui->clearButton->setIcon(QIcon());
        ui->sendButton->setIcon(QIcon());
    } else {
        ui->clearButton->setIcon(
            _platformStyle->SingleColorIcon(":/icons/remove"));
        ui->sendButton->setIcon(
            _platformStyle->SingleColorIcon(":/icons/send"));
    }

    addEntry();

    connect(ui->clearButton, &QPushButton::clicked, this,
            &RewardsDialog::clear);

    // Coin Control
    connect(ui->pushButtonRewardControl, &QPushButton::clicked, this,
            &RewardsDialog::rewardControlButtonClicked);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized")) {
        settings.setValue("fFeeSectionMinimized", true);
    }
    // compatibility
    if (!settings.contains("nFeeRadio") &&
        settings.contains("nTransactionFee") &&
        settings.value("nTransactionFee").toLongLong() > 0) {
        // custom
        settings.setValue("nFeeRadio", 1);
    }
    if (!settings.contains("nFeeRadio")) {
        // recommended
        settings.setValue("nFeeRadio", 0);
    }
    // compatibility
    if (!settings.contains("nTransactionFee")) {
        settings.setValue("nTransactionFee",
                          qint64(DEFAULT_TRANSACTION_FEE.toInt()));
    }
    if (!settings.contains("fPayOnlyMinFee")) {
        settings.setValue("fPayOnlyMinFee", false);
    }
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());
}

void RewardsDialog::setClientModel(ClientModel *_clientModel) {
    this->clientModel = _clientModel;
}

void RewardsDialog::setModel(WalletModel *_model) {
    this->model = _model;

    if (_model && _model->getOptionsModel()) {
        for (int i = 0; i < ui->entries->count(); ++i) {
            RewardsEntry *entry = qobject_cast<RewardsEntry *>(
                ui->entries->itemAt(i)->widget());
            if (entry) {
                entry->setModel(_model);
            }
        }

        interfaces::WalletBalances balances = _model->wallet().getBalances();
        setBalance(balances);
        setUnvestedBalance(balances);
        connect(_model, &WalletModel::balanceChanged, this, &RewardsDialog::setBalance);
        connect(_model, &WalletModel::balanceChanged, this, &RewardsDialog::setUnvestedBalance);
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &RewardsDialog::updateDisplayUnit);
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged,
                this, &RewardsDialog::rewardControlUpdateLabels);
        connect(_model->getOptionsModel(),
                &OptionsModel::coinControlFeaturesChanged, this,
                &RewardsDialog::rewardControlFeatureChanged);
        ui->frameCoinControl->setVisible(_model->getOptionsModel()->getCoinControlFeatures());
        rewardControlUpdateLabels();
    }
}

RewardsDialog::~RewardsDialog() {
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    delete ui;
}

void RewardsDialog::on_sendButton_clicked() {
    if (!model || !model->getOptionsModel()) {
        return;
    }

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for (int i = 0; i < ui->entries->count(); ++i) {
        RewardsEntry *entry =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(i)->widget());
        if (entry) {
            if (entry->validate(model->node())) {
                recipients.append(entry->getValue());
            } else {
                valid = false;
            }
        }
    }

    if (!valid || recipients.isEmpty()) {
        return;
    }

    fNewRecipientAllowed = false;
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid()) {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    // Always use a CRewardControl instance, use the RewardControlDialog instance if
    // RewardControl has been enabled
    CCoinControl ctrl;
    if (model->getOptionsModel()->getCoinControlFeatures()) {
        ctrl = *RewardControlDialog::coinControl();
    }

    updateRewardControlState(ctrl);

    prepareStatus = model->prepareTransaction(currentTransaction, ctrl);

    // process prepareStatus and on error generate message shown to user
    processRewardsReturn(
        prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                     currentTransaction.getTransactionFee()));

    if (prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    Amount txFee = currentTransaction.getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    for (const SendCoinsRecipient &rcp : currentTransaction.getRecipients()) {
        // generate bold amount string with wallet name in case of multiwallet
        QString amount =
            "<b>" + BitcoinUnits::formatHtmlWithUnit(
                        model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        if (model->isMultiwallet()) {
            amount.append(
                " <u>" +
                tr("from wallet %1")
                    .arg(GUIUtil::HtmlEscape(model->getWalletName())) +
                "</u> ");
        }
        amount.append("</b>");
        // generate monospace address string
        QString address =
            "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        // normal payment
        {
            if (rcp.label.length() > 0) {
                // label with address
                recipientElement =
                    tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            } else {
                // just address
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if (txFee > Amount::zero()) {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(
            model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(
            " (" +
            QString::number((double)currentTransaction.getTransactionSize() /
                            1000) +
            " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    Amount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    for (BitcoinUnits::Unit u : BitcoinUnits::availableUnits()) {
        if (u != model->getOptionsModel()->getDisplayUnit()) {
            alternativeUnits.append(
                BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
        }
    }
    questionString.append(
        tr("Total Amount %1")
            .arg(BitcoinUnits::formatHtmlWithUnit(
                model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(
        QString("<span style='font-size:10pt;font-weight:normal;'><br "
                "/>(=%2)</span>")
            .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    RewardConfirmationDialog confirmationDialog(
        tr("Confirm send coins"), questionString.arg(formatted.join("<br />")),
        SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval =
        static_cast<QMessageBox::StandardButton>(confirmationDialog.result());

    if (retval != QMessageBox::Yes) {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus =  model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processRewardsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK) {
        accept();
        RewardControlDialog::coinControl()->UnSelectAll();
        rewardControlUpdateLabels();
        Q_EMIT coinsSent(currentTransaction.getWtx()->get().GetId());
    }
    fNewRecipientAllowed = true;
}

void RewardsDialog::clear() {
    // Remove entries until only one left
    while (ui->entries->count()) {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void RewardsDialog::reject() {
    clear();
}

void RewardsDialog::accept() {
    clear();
}

RewardsEntry *RewardsDialog::addEntry() {
    RewardsEntry *entry = new RewardsEntry(platformStyle, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, &RewardsEntry::removeEntry, this,
            &RewardsDialog::removeEntry);
    connect(entry, &RewardsEntry::useAvailableBalance, this,
            &RewardsDialog::useAvailableBalance);
    connect(entry, &RewardsEntry::subtractFeeFromAmountChanged, this,
            &RewardsDialog::rewardControlUpdateLabels);

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(
        ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar *bar = ui->scrollArea->verticalScrollBar();
    if (bar) {
        bar->setSliderPosition(bar->maximum());
    }

    updateTabsAndLabels();
    return entry;
}

void RewardsDialog::updateTabsAndLabels() {
    setupTabChain(nullptr);
    rewardControlUpdateLabels();
}

void RewardsDialog::removeEntry(RewardsEntry *entry) {
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1) {
        addEntry();
    }

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *RewardsDialog::setupTabChain(QWidget *prev) {
    for (int i = 0; i < ui->entries->count(); ++i) {
        RewardsEntry *entry =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(i)->widget());
        if (entry) {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    return ui->clearButton;
}

void RewardsDialog::setAddress(const QString &address) {
    RewardsEntry *entry = nullptr;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        RewardsEntry *first =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void RewardsDialog::pasteEntry(const SendCoinsRecipient &rv) {
    if (!fNewRecipientAllowed) {
        return;
    }

    RewardsEntry *entry = nullptr;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        RewardsEntry *first =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool RewardsDialog::handlePaymentRequest(const SendCoinsRecipient &rv) {
    // Just paste the entry, all pre-checks are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void RewardsDialog::setBalance(const interfaces::WalletBalances &balances) {
    if (model && model->getOptionsModel()) {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(
            model->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}

void RewardsDialog::setUnvestedBalance(const interfaces::WalletBalances &balances) {
    if (model && model->getOptionsModel()) {
        ui->ulabelBalance->setText(BitcoinUnits::formatWithUnit(
            model->getOptionsModel()->getDisplayUnit(), balances.unvesting_balance));
    }
}

void RewardsDialog::updateDisplayUnit() {
    setBalance(model->wallet().getBalances());
    setUnvestedBalance(model->wallet().getBalances());
}

void RewardsDialog::processRewardsReturn(
    const WalletModel::SendCoinsReturn &rewardsReturn,
    const QString &msgArg) {
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to RewardsDialog usage of
    // WalletModel::RewardsReturn.
    // WalletModel::TransactionCommitFailed is used only in
    // WalletModel::rewards() all others are used only in
    // WalletModel::prepareTransaction()
    switch (rewardsReturn.status) {
        case WalletModel::InvalidAddress:
            msgParams.first =
                tr("The recipient address is not valid. Please recheck.");
            break;
        case WalletModel::InvalidAmount:
            msgParams.first = tr("The amount to pay must be larger than 0.");
            break;
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("The amount exceeds your balance.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 "
                                 "transaction fee is included.")
                                  .arg(msgArg);
            break;
        case WalletModel::DuplicateAddress:
            msgParams.first = tr("Duplicate address found: addresses should "
                                 "only be used once each.");
            break;
        case WalletModel::TransactionCreationFailed:
            msgParams.first = tr("Transaction creation failed!");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::TransactionCommitFailed:
            msgParams.first =
                tr("The transaction was rejected with the following reason: %1")
                    .arg(rewardsReturn.reasonCommitFailed);
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::AbsurdFee:
            msgParams.first =
                tr("A fee higher than %1 is considered an absurdly high fee.")
                    .arg(BitcoinUnits::formatWithUnit(
                        model->getOptionsModel()->getDisplayUnit(),
                        model->node().getMaxTxFee()));
            break;
        // included to prevent a compiler warning.
        case WalletModel::OK:
        default:
            return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void RewardsDialog::minimizeFeeSection(bool fMinimize) {
    ui->frameFeeSelection->setVisible(!fMinimize);
    fFeeMinimized = fMinimize;
}

void RewardsDialog::on_buttonChooseFee_clicked() {
    minimizeFeeSection(false);
}

void RewardsDialog::on_buttonMinimizeFee_clicked() {
    minimizeFeeSection(true);
}

void RewardsDialog::useAvailableBalance(RewardsEntry *entry) {
    // Get CRewardControl instance if RewardControl is enabled or create a new one.
    CCoinControl coin_control;
    if (model->getOptionsModel()->getCoinControlFeatures()) {
        coin_control = *RewardControlDialog::coinControl();
    }

    // Calculate available amount to send.
    Amount amount = model->wallet().getAvailableBalance(coin_control);
    for (int i = 0; i < ui->entries->count(); ++i) {
        RewardsEntry *e =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(i)->widget());
        if (e && !e->isHidden() && e != entry) {
            amount -= e->getValue().amount;
        }
    }

    if (amount > Amount::zero()) {
        entry->setAmount(amount);
    } else {
        entry->setAmount(Amount::zero());
    }
}

void RewardsDialog::setMinimumFee() {
}

void RewardsDialog::updateRewardControlState(CCoinControl &ctrl) {
    ctrl.m_feerate.reset();
}

// Coin Control: settings menu - coin control enabled/disabled by user
void RewardsDialog::rewardControlFeatureChanged(bool checked) {
    ui->frameCoinControl->setVisible(checked);

    // coin control features disabled
    if (!checked && model) {
        RewardControlDialog::coinControl()->SetNull();
    }

    rewardControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void RewardsDialog::rewardControlButtonClicked() {
    RewardControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    rewardControlUpdateLabels();
}

// Coin Control: update labels
void RewardsDialog::rewardControlUpdateLabels() {
    if (!model || !model->getOptionsModel()) {
        return;
    }

    updateRewardControlState(*RewardControlDialog::coinControl());

    // set pay amounts
    RewardControlDialog::payAmounts.clear();
    RewardControlDialog::fSubtractFeeFromAmount = false;
    for (int i = 0; i < ui->entries->count(); ++i) {
        RewardsEntry *entry =
            qobject_cast<RewardsEntry *>(ui->entries->itemAt(i)->widget());
        if (entry && !entry->isHidden()) {
            SendCoinsRecipient rcp = entry->getValue();
            RewardControlDialog::payAmounts.append(rcp.amount);
            if (rcp.fSubtractFeeFromAmount) {
                RewardControlDialog::fSubtractFeeFromAmount = true;
            }
        }
    }

    // actual coin control calculation
    RewardControlDialog::updateLabels(model, this);
    // always show
    ui->widgetRewardControl->show();
 
}

RewardConfirmationDialog::RewardConfirmationDialog(const QString &title,
                                               const QString &text,
                                               int _secDelay, QWidget *parent)
    : QMessageBox(QMessageBox::Question, title, text,
                  QMessageBox::Yes | QMessageBox::Cancel, parent),
      secDelay(_secDelay) {
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, &QTimer::timeout, this,
            &RewardConfirmationDialog::countDown);
}

int RewardConfirmationDialog::exec() {
    updateYesButton();
    countDownTimer.start(1000);
    return QMessageBox::exec();
}

void RewardConfirmationDialog::countDown() {
    secDelay--;
    updateYesButton();

    if (secDelay <= 0) {
        countDownTimer.stop();
    }
}

void RewardConfirmationDialog::updateYesButton() {
    if (secDelay > 0) {
        yesButton->setEnabled(false);
        yesButton->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    } else {
        yesButton->setEnabled(true);
        yesButton->setText(tr("Yes"));
    }
}
