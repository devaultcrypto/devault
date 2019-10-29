// Copyright (c) 2019 DeVault developers
// Parts copied from startoptionsrevealed, created by Kolby on 6/19/2019.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <cashaddrenc.h>
#include <sweep.h>
#include <ui_sweep.h>

#include <dvtui.h>

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

Sweep::~Sweep() {
    delete ui;
}

Sweep::Sweep(QWidget *parent) : QDialog(parent), ui(new Ui::Sweep) {

    secret = "";

    ui->setupUi(this);
    if (DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(QIcon());
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setIcon(QIcon());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void Sweep::accept() {
    secret = ui->privkey->text().toStdString().c_str();
    if (CheckSecretIsValid(secret)) {
        QDialog::accept();
    } else {
        QMessageBox::critical(this, tr("Invalid Secret key"),
                              tr("This is an invalid Secret key."));
    }
    close();
}
void Sweep::reject() {
    secret = "";
    QDialog::reject();
    QDialog::close();
}

void Sweep::on_okButton_accepted() {
    secret = ui->privkey->text().toStdString().c_str();
    if (CheckSecretIsValid(secret)) {
        QDialog::accept();
    } else {
        QMessageBox::critical(this, tr("Invalid Secret key"),
                              tr("This is an invalid Secret key."));
    }
    close();
}
