#include "mnemonicwidget.h"
#include "ui_mnemonicwidget.h"
#include <qt/guiutil.h>
#include <QDebug>
#include <QFileDialog>
#include <iostream>
#include "random.h"
#include <wallet/wallet.h>
#include <wallet/walletutil.h>
#include "wallet/mnemonic.h"

MnemonicWidget::MnemonicWidget(QWidget *parent) :
QDialog(parent),
ui(new Ui::MnemonicWidget)
{
    ui->setupUi(this);
    this->parent = parent;

    //this->setStyleSheet(GUIUtil::loadStyleSheet());

    this->setWindowTitle("DeVault Wallet Setup");

    this->setContentsMargins(0,0,0,0);
    this->ui->containerLeft->setContentsMargins(0,0,0,0);
    ui->QStackTutorialContainer->setContentsMargins(0,0,0,10);

    // TODO: Complete this with the specific language words
    mnemonic::WordList words;
    words = mnemonic::getListOfAllWordInLanguage(language::en);

    for(unsigned long i=0; i< words.size() ; i++) {
        wordList.append(QString::fromStdString(words[i]));
    }

    tutorialMnemonicRevealed = new TutorialMnemonicRevealed(wordList, this);

    tutorialLanguageWidget = new TutorialLanguagesWidget(this);

    ui->QStackTutorialContainer->addWidget(tutorialMnemonicRevealed);
    ui->QStackTutorialContainer->addWidget(tutorialLanguageWidget);
    ui->QStackTutorialContainer->setCurrentWidget(tutorialLanguageWidget);

    loadLeftContainer(":/icons/img-start-logo", "Welcome to DeVault", "");
    ui->btnBack->setVisible(false);
    ui->btnLineSeparator->setVisible(false);

    ui->btnNext->setFocusPolicy(Qt::NoFocus);
    ui->btnBack->setFocusPolicy(Qt::NoFocus);

    connect(ui->btnNext, SIGNAL(clicked()), this, SLOT(on_next_triggered()));
    connect(ui->btnBack, SIGNAL(clicked()), this, SLOT(on_back_triggered()));

}

// left side
void MnemonicWidget::loadLeftContainer(QString imgPath, QString topMessage,  QString bottomMessage){
    QPixmap icShield(120,120);
    icShield.load(imgPath);
    ui->imgTutorial->setPixmap(icShield);
    ui->messageTop->setText(topMessage);
    ui->messageBottom->setText(bottomMessage);
}


// Actions
std::vector<std::string> mnemonicWordList;
void MnemonicWidget::on_next_triggered(){
    QWidget *qWidget = nullptr;
    switch (position) {
        case 0:
        {

            strLanguageSelection = QString::fromStdString(tutorialLanguageWidget->GetLanguageSelection());

            tutorialCreateWallet = new TutorialCreateWalletWidget(this);
            ui->QStackTutorialContainer->addWidget(tutorialCreateWallet);
            qWidget = tutorialCreateWallet;
            //img-start-wallet
            loadLeftContainer(":/icons/img-wallet","Setup your\nDeVault wallet","");
            ui->btnLineSeparator->setVisible(true);
            ui->btnBack->setVisible(true);
            break;
        }
        case 1:
        {
            if (tutorialCreateWallet && tutorialCreateWallet->GetButtonClicked()) {
                switch (tutorialCreateWallet->GetButtonClicked()) {
                    case 1:
                    {
                        //Generate mnemonic phrase from fresh entropy
                        mnemonic = "";
                        std::vector<uint8_t> keyHDMasterKey(16);
                        std::vector<uint8_t> hashWords;
                        mnemonic::WordList words;
                        GetStrongRandBytes(keyHDMasterKey.data(), keyHDMasterKey.size());
                        mnemonicWordList = mnemonic::mapBitsToMnemonic(keyHDMasterKey, language::en);
                        mnemonic = join(mnemonicWordList," ");

                        tutorialMnemonicCode = new TutorialMnemonicCode(mnemonicWordList, this);
                        ui->QStackTutorialContainer->addWidget(tutorialMnemonicCode);
                        qWidget = tutorialMnemonicCode;
                        loadLeftContainer(":/icons/img-start-backup","Backup your \n recovery seed phrase","Your 24-word seed phrase \n can  be used to restore your wallet.");
                        break;
                    }
                    case 2:
                    {
                        qWidget = tutorialMnemonicRevealed;
                        loadLeftContainer(":/icons/img-start-confirm","Enter your \n seed phrase","");
                        break;
                    }
                    case 3:
                    {
                        //TODO: Import wallet from file. This code probably doesn't work.
                        // This isn't used rn
                        /*
                        QFile walletFile(QFileDialog::getOpenFileName(0, "Choose wallet file"));
                        if(!walletFile.exists())
                            return;
                        fs::path walletPath = walletFile.fileName().toStdString();
                        CWallet::CreateWalletFromFile(walletFile.fileName().toStdString(), walletPath,);
                        shutdown = false;
                        accept();
                        return;
                         */
                    }
                }
            }

            break;
        }
        case 2:
        {
            if (tutorialMnemonicRevealed && tutorialCreateWallet->GetButtonClicked() == 2) {
                mnemonic = "";
                std::string strWalletFile = "wallet.dat";
                std::list<QString> q_word_list = tutorialMnemonicRevealed->getOrderedStrings();
                for (QString &q_word : q_word_list) {
                    if (mnemonic.empty())
                        mnemonic = q_word.toStdString();
                    else
                        mnemonic += " " + q_word.toStdString();
                }

                shutdown = false;
                accept();
            } else {
                qWidget = tutorialMnemonicRevealed;
                loadLeftContainer(":/icons/img-start-confirm","Confirm your \n seed phrase","");
            }
            break;
        }
        case 3:
        {
            if (tutorialCreateWallet->GetButtonClicked() == 2) {
                shutdown = false;
                accept();
                return;
            } else {
                // Check mnemonic first
                std::list<QString> q_word_list = tutorialMnemonicRevealed->getOrderedStrings();
                std::string mnemonicTest;
                for (QString &q_word : q_word_list) {
                    if (mnemonicTest.empty())
                        mnemonicTest = q_word.toStdString();
                    else
                        mnemonicTest += " " + q_word.toStdString();
                }

                if(mnemonicTest != mnemonic){
                    // displays a error if the return value doesn't match to inform the user
                    std::string notice = "Invalid mnemonic, please write the words on the same order as before";
                    DisplayWalletMnemonic(notice);
                    return;
                }
                shutdown = false;
                accept();
            }
            break;
        }
        case 4:
            shutdown = false;
            accept();
            break;

    }
    if(qWidget){
        position++;
        if(ui->QStackTutorialContainer->currentWidget() != qWidget) {
            ui->QStackTutorialContainer->setCurrentWidget(qWidget);
        }
    }
}

void MnemonicWidget::on_back_triggered(){
    if(position != 0){
        QWidget *qWidget = nullptr;
        position--;
        switch (position) {
            case 0:
            {
                qWidget = tutorialLanguageWidget;
                loadLeftContainer(":/icons/img-start-logo", "Welcome to DeVault", "");
                ui->btnBack->setVisible(false);
                ui->btnLineSeparator->setVisible(false);
                break;
            }
            case 1:
            {
                qWidget = tutorialCreateWallet;
                loadLeftContainer(":/icons/img-start-wallet","Setup your\nDeVault wallet","");
                ui->btnLineSeparator->setVisible(true);
                ui->btnBack->setVisible(true);
                break;
            }
            case 2:
            {
                if (tutorialMnemonicRevealed && tutorialCreateWallet->GetButtonClicked() == 2) {
                    qWidget = tutorialMnemonicRevealed;
                    loadLeftContainer(":/icons/img-start-confirm","Enter your \n seed phrase","");
                } else {
                    qWidget = tutorialMnemonicCode;
                    loadLeftContainer(":/icons/img-start-confirm","Confirm your \n seed phrase","");
                }
                break;
            }
            case 3:
            {
                qWidget = tutorialMnemonicRevealed;
                loadLeftContainer(":/icons/img-start-password","Encrypt your wallet","");
                break;
            }

        }
        if(qWidget != nullptr){
            if(ui->QStackTutorialContainer->currentWidget() != qWidget) {
                ui->QStackTutorialContainer->setCurrentWidget(qWidget);
            }
        }
    }
}

std::string MnemonicWidget::GetLanguageSelection() const
{
    return strLanguageSelection.toStdString();
}

MnemonicWidget::~MnemonicWidget()
{
    delete ui;
}
