// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <openuridialog.h>
#include <ui_openuridialog.h>

#include <guiutil.h>
#include <walletmodel.h>

#include <QUrl>

OpenURIDialog::OpenURIDialog(const Config *configIn, QWidget *parent)
    : QDialog(parent), ui(new Ui::OpenURIDialog), config(configIn) {
    ui->setupUi(this);
    ui->uriEdit->setPlaceholderText(std::get<0>(GUIUtil::bitcoinURIScheme(*config)) + ":");
}

OpenURIDialog::~OpenURIDialog() {
    delete ui;
}

QString OpenURIDialog::getURI() {
    return ui->uriEdit->text();
}

void OpenURIDialog::accept() {
    SendCoinsRecipient rcp;
    auto uriSchemes = GUIUtil::bitcoinURIScheme(*config);
    if (GUIUtil::parseBitcoinURI(uriSchemes, getURI(), &rcp)) {
        /* Only accept value URIs */
        QDialog::accept();
    } else {
        ui->uriEdit->setValid(false);
    }
}

void OpenURIDialog::on_selectFileButton_clicked() {
    QString filename = GUIUtil::getOpenFileName(
        this, tr("Select payment request file to open"), "", "", nullptr);
    if (filename.isEmpty()) return;
    QUrl fileUri = QUrl::fromLocalFile(filename);
    auto uriSchemes = GUIUtil::bitcoinURIScheme(*config);
    ui->uriEdit->setText(std::get<0>(uriSchemes) +
                         ":?r=" + QUrl::toPercentEncoding(fileUri.toString()));
}
