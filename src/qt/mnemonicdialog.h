#ifndef DEVAULT_MNEMONICDIALOG_H
#define DEVAULT_MNEMONICDIALOG_H

#include "wallet/walletinitflags.h"

#include <QDialog>
#include <QThread>

namespace Ui {
    class MnemonicDialog;
}

class MnemonicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MnemonicDialog(QWidget *parent = 0);
    ~MnemonicDialog();
    bool ShutdownRequested() const { return shutdown; }
    MnemonicWalletInitFlags GetSelection() const { return selection; }

private Q_SLOTS:
    void radioSelection();
    void buttonClicked();

private:
    Ui::MnemonicDialog *ui;
    bool shutdown;
    MnemonicWalletInitFlags selection;
};


#endif //DEVAULT_MNEMONICDIALOG_H
