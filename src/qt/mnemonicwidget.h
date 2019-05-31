#ifndef DEVAULT_MNEMONICWIDGET_H
#define DEVAULT_MNEMONICWIDGET_H

#include "wallet/walletinitflags.h"

#include <QWidget>
#include <QDialog>
#include <uint256.h>

#include "tutorialcreatewalletwidget.h"
#include "tutoriallanguageswidget.h"
#include "tutorialmnemoniccode.h"
#include "tutorialmnemonicrevealed.h"

class WalletModel;

namespace Ui {
    class MnemonicWidget;
}

class MnemonicWidget : public QDialog
{
    Q_OBJECT

public:
    explicit MnemonicWidget(QWidget *parent = nullptr);
    ~MnemonicWidget();

    bool ShutdownRequested() const { return shutdown; }
    MnemonicWalletInitFlags GetSelection() const { return selection; }
    void SetLanguageSelection(QString strLanguage) { this->strLanguageSelection = strLanguage; }
    std::string GetLanguageSelection() const;
    std::string GetMnemonic() const {return mnemonic; }

private Q_SLOTS:
            void on_next_triggered();

    void on_back_triggered();
private:
    Ui::MnemonicWidget *ui;
    QWidget* parent;
    int position = 1;

    QStringList wordList;

    bool shutdown = true;
    MnemonicWalletInitFlags selection;

    TutorialMnemonicCode *tutorialMnemonicCode;
    TutorialMnemonicRevealed *tutorialMnemonicRevealed;
    TutorialLanguagesWidget *tutorialLanguageWidget;
    TutorialCreateWalletWidget *tutorialCreateWallet;

    QString strLanguageSelection;
    std::string mnemonic;

    void loadLeftContainer(QString imgPath, QString topMessage,  QString bottomMessage);
};

#endif //DEVAULT_MNEMONICWIDGET_H
