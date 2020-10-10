// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ui_rewardsentry.h>
#include <qt/rewardsentry.h>

#include <qt/rewardsdialog.h>

#include <config.h>
#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QPushButton>
#include <QClipboard>

RewardsEntry::RewardsEntry(const PlatformStyle *_platformStyle,
                               QWidget *parent)
    : QStackedWidget(parent), ui(new Ui::RewardsEntry), model(nullptr),
      platformStyle(_platformStyle) {
    ui->setupUi(this);

    ui->addressBookButton->setIcon(
        platformStyle->SingleColorIcon(":/icons/address-book"));
    ui->pasteButton->setIcon(
        platformStyle->SingleColorIcon(":/icons/editpaste"));
    ui->deleteButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_is->setIcon(
        platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_s->setIcon(
        platformStyle->SingleColorIcon(":/icons/remove"));

    auto schemes = GUIUtil::bitcoinURIScheme(GetConfig());
    ui->messageTextLabel->setToolTip(
        tr("A message that was attached to the %1 or %2 URI which will be"
           " stored with the transaction for your reference. Note: "
           "This message will not be sent over the DeVault network.")
            .arg(std::get<0>(schemes)).arg(std::get<1>(schemes)));

    setCurrentWidget(ui->Rewards);

    if (platformStyle->getUseExtraSpacing()) ui->payToLayout->setSpacing(4);
    ui->addAsLabel->setPlaceholderText(
        tr("Enter a label for this address to add it to your address book"));

    // normal bitcoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying bitcoin address(es)
    ui->payTo_is->setFont(GUIUtil::fixedPitchFont());

    // Connect signals
    connect(ui->deleteButton, &QPushButton::clicked, this,            &RewardsEntry::deleteClicked);
    connect(ui->deleteButton_is, &QPushButton::clicked, this,            &RewardsEntry::deleteClicked);
    connect(ui->deleteButton_s, &QPushButton::clicked, this,            &RewardsEntry::deleteClicked);
}

RewardsEntry::~RewardsEntry() {
    delete ui;
}

void RewardsEntry::on_pasteButton_clicked() {
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void RewardsEntry::on_addressBookButton_clicked() {
    if (!model) return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection,
                        AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if (dlg.exec()) {
        ui->payTo->setText(dlg.getReturnValue());
        //        ui->payAmount->setFocus();
    }
}

void RewardsEntry::on_payTo_textChanged(const QString &address) {
    updateLabel(address);
}

void RewardsEntry::setModel(WalletModel *_model) {
    this->model = _model;

    if (_model && _model->getOptionsModel())
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged,
                this, &RewardsEntry::updateDisplayUnit);

    clear();
}

void RewardsEntry::clear() {
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    // clear UI elements for unauthenticated payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    // clear UI elements for authenticated payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();

    // update the display unit, to not use the default ("DVT")
    updateDisplayUnit();
}

void RewardsEntry::deleteClicked() {
    Q_EMIT removeEntry(this);
}


bool RewardsEntry::validate(interfaces::Node &node) {
    if (!model) {
        return false;
    }

    // Check input validity
    bool retval = true;

    if (!model->validateAddress(ui->payTo->text())) {
        ui->payTo->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient RewardsEntry::getValue() {
    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = m_amount;
    recipient.message = ui->messageTextLabel->text();

    return recipient;
}

QWidget *RewardsEntry::setupTabChain(QWidget *prev) {
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    return ui->deleteButton;
}

void RewardsEntry::setValue(const SendCoinsRecipient &value) {
    recipient = value;

    {
        // message
        ui->messageTextLabel->setText(recipient.message);
        ui->messageTextLabel->setVisible(!recipient.message.isEmpty());
        ui->messageLabel->setVisible(!recipient.message.isEmpty());

        ui->addAsLabel->clear();
        ui->payTo->setText(
            recipient.address);         // this may set a label from addressbook
        if (!recipient.label.isEmpty()) // if a label had been set from the
                                        // addressbook, don't overwrite with an
                                        // empty label
            ui->addAsLabel->setText(recipient.label);
        //        ui->payAmount->setValue(recipient.amount);
    }
}

void RewardsEntry::setAddress(const QString &address) {
    ui->payTo->setText(address);
}

void RewardsEntry::setAmount(const Amount amount) {
    m_amount = amount;
}

bool RewardsEntry::isClear() {
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() &&
           ui->payTo_s->text().isEmpty();
}

void RewardsEntry::setFocus() {
    ui->payTo->setFocus();
}

void RewardsEntry::updateDisplayUnit() {}

bool RewardsEntry::updateLabel(const QString &address) {
    if (!model) return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel =
        model->getAddressTableModel()->labelForAddress(address);
    if (!associatedLabel.isEmpty()) {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}
