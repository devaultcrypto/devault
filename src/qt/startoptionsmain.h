//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QWidget>
#include <QDialog>

#include "startoptions.h"
#include <startoptionsrevealed.h>
#include <startoptionssort.h>
#include <startoptionsrestore.h>


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
    explicit StartOptionsMain(QWidget *parent);
    ~StartOptionsMain();

    std::vector<std::string> getWords() { return wordsDone; }


public slots:
    void on_RestoreWallet_clicked();
    void on_Next_clicked();
    void on_Back_clicked();
    void on_NewWallet_clicked();

private:
    Ui::StartOptionsMain *ui;
    QWidget* parent;
    int pageNum = 1;
    int rows;
    mnemonic::WordList words;
    mnemonic::WordList wordsDone;

    StartOptions *startOptions;
    StartOptionsRevealed *startOptionsRevealed;
    StartOptionsSort *startOptionsSort;
    StartOptionsRestore *startOptionsRestore;
    std::string words_empty_str;
    std::string words_mnemonic;


};


