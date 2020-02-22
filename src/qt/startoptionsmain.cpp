//
// Copyright (c) 2019 DeVault developers
// Created by Kolby on 6/19/2019.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <startoptionsdialog.h>
#include <startoptionsmain.h>
#include <ui_startoptionsmain.h>

#include <QDebug>
#include <QFileDialog>
#include <dvtui.h>
#include <iostream>
#include <qt/guiutil.h>
#include <random.h>

#include <ui_interface.h>
#include <wallet/mnemonic.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

inline bool is_not_space(int c) { return !std::isspace(c); }

StartOptionsMain::StartOptionsMain(QWidget *parent)
    : QDialog(parent), ui(new Ui::StartOptionsMain) {
    ui->setupUi(this);
    if (DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    }

    this->setWindowTitle("DeVault Wallet Setup");

    wordsList = mnemonic::getListOfAllWordInLanguage(language::en);

    for (unsigned long i = 0; i < wordsList.size(); i++) {
        qWordList.append(QString::fromStdString(wordsList[i]));
    }

    this->setContentsMargins(0, 0, 0, 0);
    ui->QStackTutorialContainer->setContentsMargins(0, 0, 0, 10);
    ui->Back->setVisible(false);
    ui->Next->setVisible(false);
    startOptions = new StartOptions(this);
    ui->QStackTutorialContainer->addWidget(startOptions);
    ui->QStackTutorialContainer->setCurrentWidget(startOptions);
    bls = true;
}

StartOptionsMain::~StartOptionsMain() {
    delete ui;
}

void StartOptionsMain::on_NewWallet_clicked() {
    pageNum = CreateOrRestorePage;
    ui->NewWallet->setVisible(false);
    ui->RestoreWallet->setVisible(false);
    ui->RestoreLegacyWallet->setVisible(false);
    ui->Back->setVisible(true);
    ui->Next->setVisible(true);
    rows = startOptions->getRows();

    std::vector<uint8_t> hashWords;
    if (rows == 4) {
        std::tie(words, hashWords) = mnemonic::GenerateSeedPhrase(24);
    } else {
        std::tie(words, hashWords) = mnemonic::GenerateSeedPhrase();
    }

    startOptionsRevealed = new StartOptionsRevealed(words, rows, this);
    ui->QStackTutorialContainer->addWidget(startOptionsRevealed);
    ui->QStackTutorialContainer->setCurrentWidget(startOptionsRevealed);
    bls = true;
}

void StartOptionsMain::on_RestoreWallet_clicked() {
    pageNum = CheckWordsPage;
    ui->NewWallet->setVisible(false);
    ui->RestoreLegacyWallet->setVisible(false);
    ui->RestoreWallet->setVisible(false);
    ui->Back->setVisible(true);
    ui->Next->setVisible(true);
    rows = startOptions->getRows();

    startOptionsRestore = new StartOptionsRestore(qWordList, rows, true, this);
    ui->QStackTutorialContainer->addWidget(startOptionsRestore);
    ui->QStackTutorialContainer->setCurrentWidget(startOptionsRestore);
}

void StartOptionsMain::on_RestoreLegacyWallet_clicked() {
    pageNum = CheckWordsPage;
    ui->NewWallet->setVisible(false);
    ui->RestoreLegacyWallet->setVisible(false);
    ui->RestoreWallet->setVisible(false);
    ui->Back->setVisible(true);
    ui->Next->setVisible(true);
    rows = startOptions->getRows();

    startOptionsRestore = new StartOptionsRestore(qWordList, rows, false, this);
    ui->QStackTutorialContainer->addWidget(startOptionsRestore);
    ui->QStackTutorialContainer->setCurrentWidget(startOptionsRestore);
}

void StartOptionsMain::on_Back_clicked() {
    /*  Pages
     * Page 1 : Main page were you select the amount of words and the option
     *      Path 1 : Create wallet
     *          Page 2 : Shows you your words
     *          Page 3 : Order the words
     *      Path 2 : Restore wallet
     *          Page 4 Enter words to restore
     */
    switch (pageNum) {
        case StartPage: {
            // does nothing as you can not click back on page one
        }
        case CreateOrRestorePage: {
            pageNum = StartPage;
            ui->NewWallet->setVisible(true);
            ui->RestoreWallet->setVisible(true);
            ui->RestoreLegacyWallet->setVisible(true);
            ui->Back->setVisible(false);
            ui->Next->setVisible(false);
            ui->QStackTutorialContainer->setCurrentWidget(startOptions);
            break;
        }
        case OrderWordsPage: {
            pageNum = CreateOrRestorePage;
            ui->QStackTutorialContainer->addWidget(startOptionsRevealed);
            ui->QStackTutorialContainer->setCurrentWidget(startOptionsRevealed);
            break;
        }
        case CheckWordsPage: {
            pageNum = StartPage;
            ui->NewWallet->setVisible(true);
            ui->RestoreWallet->setVisible(true);
            ui->RestoreLegacyWallet->setVisible(true);
            ui->Back->setVisible(false);
            ui->Next->setVisible(false);
            ui->QStackTutorialContainer->setCurrentWidget(startOptions);
            break;
        }
    }
}

void StartOptionsMain::on_Next_clicked() {
    auto rtrim = [](std::string& s) { s.erase(std::find_if(s.rbegin(), s.rend(), is_not_space).base(), s.end()); };
    /*  Pages
     * Page 1 : Main page were you select the amount of words and the option
     *      Path 1 : Create wallet
     *          Page 2 : Shows you your words
     *          Page 3 : Order the words
     *      Path 2 : Restore wallet
     *          Page 4 Enter words to restore
     */
    switch (pageNum) {
        case StartPage: {
            // does nothing as you can not click back on page one
        }

        case CreateOrRestorePage: {
#ifndef DEV_DEBUG
            pageNum = OrderWordsPage;
            startOptionsSort = new StartOptionsSort(words, rows, this);
            ui->QStackTutorialContainer->addWidget(startOptionsSort);
            ui->QStackTutorialContainer->setCurrentWidget(startOptionsSort);
#else
#pragma warning "Quick Dev hack for new wallets"
            wordsDone = words;
            QApplication::quit();
#endif
            break;
        }
        case OrderWordsPage: {
            words_empty_str = "";
            std::list<QString> word_str = startOptionsSort->getOrderedStrings();
            // reverses the lists order
            word_str.reverse();
            for (QString &q_word : word_str) {
                if (words_empty_str.empty())
                    words_empty_str = q_word.toStdString();
                else
                    words_empty_str += "" + q_word.toStdString();
            }

            words_mnemonic = "";
            for (std::string &q_word : words) {
                if (words_mnemonic.empty())
                    words_mnemonic = q_word;
                else
                    words_mnemonic += "" + q_word;
            }
            rtrim(words_empty_str);
            rtrim(words_mnemonic);
            if (words_empty_str != words_mnemonic) {
                QString error = "Unfortunately, your words are in the wrong "
                                "order. Please try again.";
                StartOptionsDialog dlg(error, this);
                dlg.exec();
            } else {
                wordsDone = words;
                QApplication::quit();
            }
            break;
        }
        case CheckWordsPage: {
            std::vector<std::string> word_str = startOptionsRestore->getOrderedStrings();
            bls = startOptionsRestore->is_bls();

            std::string seedphrase = "";
            for (std::string &q_word : word_str) {
                if (seedphrase.empty())
                    seedphrase = q_word;
                else
                    seedphrase += " " + q_word;
            }
            // reverses the lists order
            if (mnemonic::isValidSeedPhrase(seedphrase)) {
                wordsDone = word_str;
                QApplication::quit();
            } else {
                QString error =
                    "Unfortunately, your words seem to be invalid. This is "
                    "most likely because one or more are misspelled. Please "
                    "double check your spelling and word order. If you "
                    "continue to have issue your words may not currently be in "
                    "our dictionary.";
                StartOptionsDialog dlg(error, this);
                dlg.exec();
            }

            break;
        }
    }
}
