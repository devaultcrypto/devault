//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QDialog>
#include <QWidget>

#include "startoptions.h"
#include <startoptionsrestore.h>
#include <startoptionsrevealed.h>
#include <startoptionssort.h>

#include "wallet/mnemonic.h"

class WalletModel;

namespace Ui {
class StartOptionsMain;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptionsMain : public QDialog {
    Q_OBJECT

public:
    enum Pages { StartPage, CreateOrRestorePage, OrderWordsPage, CheckWordsPage };
    explicit StartOptionsMain(QWidget *parent);
    ~StartOptionsMain();

    std::vector<std::string> getWords() { return wordsDone; }
    bool is_bls() { return bls;}

public slots:
    void on_RestoreWallet_clicked();
    void on_RestoreLegacyWallet_clicked();
    void on_Next_clicked();
    void on_Back_clicked();
    void on_NewWallet_clicked();

private:
    Ui::StartOptionsMain *ui;
    Pages pageNum = StartPage;
    int rows;
    bool bls;
    QStringList qWordList;
    mnemonic::WordList wordsList;
    mnemonic::WordList words;
    mnemonic::WordList wordsDone;

    StartOptions *startOptions;
    StartOptionsRevealed *startOptionsRevealed;
    StartOptionsSort *startOptionsSort;
    StartOptionsRestore *startOptionsRestore;
    std::string words_empty_str;
    std::string words_mnemonic;
};
