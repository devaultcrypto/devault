// Copyright (c) 2019 DeVault developers
// Parts copied from startoptionsrevealed, created by Kolby on 6/19/2019.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <revealphrase.h>
#include <ui_revealphrase.h>
#include <utilsplitstring.h>

#include <dvtui.h>

#include <QMessageBox>
#include <QLabel>

RevealPhrase::~RevealPhrase() {
    delete ui;
}

RevealPhrase::RevealPhrase(const SecureVector &secure_words, QWidget *parent)
    : QDialog(parent), ui(new Ui::RevealPhrase) {
    ui->setupUi(this);
    if (DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    }
    ui->seedLabel->setText(
        tr("The words below are your recovery phrase (mnemonic seed). Please "
           "write them down and/or securely save them. "));

    std::list<QLabel *> labelsList;
    std::vector<std::string> Words;

    // Convert from SecureVector to std::vector<std::string>
    {
        std::vector<unsigned char> insecure_words(secure_words.begin(),
                                                  secure_words.end());
        std::string str_words(insecure_words.begin(), insecure_words.end());
        Split(Words, str_words, " ");
    }

    int rows = Words.size() / 6;
    int j=0;
    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < 6; k++) {

            QLabel *label = new QLabel(this);
            label->setStyleSheet(
                "QLabel{background-color:transparent;padding-left:8px;padding-"
                "right:8px;border-radius:0px;color:#fff;border-bottom:2px "
                "solid rgb(35,136,237); text-align:center; font-size:18px;}");
            label->setMinimumSize(110, 50);
            label->setMaximumSize(110, 50);
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            label->setContentsMargins(8, 12, 8, 12);
            label->setAlignment(Qt::AlignCenter);
            labelsList.push_back(label);
            ui->gridLayoutRevealed->addWidget(label, i, k, Qt::AlignCenter);
            label->setText(QString::fromStdString(Words[j++]));
        }
    }
}
