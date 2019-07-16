// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Copyright (c) 2019 Jon Spock developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <setpassphrasedialog.h>
#include <ui_setpassphrasedialog.h>

#include <guiconstants.h>
#include <dvtui.h>

#include <support/allocators/secure.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

SetPassphraseDialog::SetPassphraseDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::SetPassphraseDialog), 
      fCapsLock(false) {
    password = "";
    ui->setupUi(this);
    if(DVTUI::customThemeIsSet()) {
        QString appstyle = "fusion";
        QApplication::setStyle(appstyle);
        setStyleSheet(DVTUI::styleSheetString);
    } 

    ui->passEdit1->setMinimumSize(ui->passEdit1->sizeHint());
    ui->passEdit2->setMinimumSize(ui->passEdit2->sizeHint());
    ui->passEdit3->setMinimumSize(ui->passEdit3->sizeHint());

    ui->passEdit1->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit2->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit3->setMaxLength(MAX_PASSPHRASE_SIZE);

    // Setup Caps Lock detection.
    ui->passEdit1->installEventFilter(this);
    ui->passEdit2->installEventFilter(this);
    ui->passEdit3->installEventFilter(this);

    ui->welcomeTitle->setText(
        tr("Welcome to the <b>DeVault</b> Decentralized Community! "
           ""));
    ui->welcomeLabel->setText(
        tr("First we're going to set a secure <b>passphrase</b> for your new wallet."
           "Then we will move on to creating your wallets unique <b>memnomic seed phrase!</b> "));
  /*  ui->welcomeLabel2->setText(
        tr("Remember that the DeVault team has <b>no way to restore lost passwords or seeds!</b> "
           "<br/>It's very important that you store these safely yourself."
           "Note: Your passphrase will always be able to unlock your wallet (without the seed) & Your Seed "
           "<br/>will always be able to restore the wallet from scratch (without the passphrase).")); */
    ui->warningLabel->setText(
        tr("Please enter a strong passphrase for encrypting your wallet. We suggest using a "
           "passphrase of <b>ten or more random characters</b>, or even <b>eight or more words</b>. "));
    ui->passLabel1->hide();
    ui->passEdit1->hide();
    setWindowTitle(tr("Welcome to DeVault - Set Wallet password"));

    textChanged();
    connect(ui->passEdit1, SIGNAL(textChanged(QString)), this,
            SLOT(textChanged()));
    connect(ui->passEdit2, SIGNAL(textChanged(QString)), this,
            SLOT(textChanged()));
    connect(ui->passEdit3, SIGNAL(textChanged(QString)), this,
            SLOT(textChanged()));
}

SetPassphraseDialog::~SetPassphraseDialog() {
    secureClearPassFields();
    delete ui;
}

void SetPassphraseDialog::accept() {
    SecureString oldpass, newpass1, newpass2;
    oldpass.reserve(MAX_PASSPHRASE_SIZE);
    newpass1.reserve(MAX_PASSPHRASE_SIZE);
    newpass2.reserve(MAX_PASSPHRASE_SIZE);
    // TODO: get rid of this .c_str() by implementing
    // SecureString::operator=(std::string)
    // Alternately, find a way to make this input mlock()'d to begin with.
    oldpass.assign(ui->passEdit1->text().toStdString().c_str());
    newpass1.assign(ui->passEdit2->text().toStdString().c_str());
    newpass2.assign(ui->passEdit3->text().toStdString().c_str());

    secureClearPassFields();

    if (newpass1.empty() || newpass2.empty() || (newpass1.size() < 4)) {
        if (newpass1.size() < 4) QMessageBox::critical(this, tr("failed"), tr("The supplied passphrase is too short, it must be at least 4 characters"));
        // Cannot encrypt with empty passphrase or less < 4 characters
        newpass1.clear();
        return;
    }
    if (newpass1 == newpass2) {
        password = newpass1;
        //QMessageBox::warning(this, tr("Success"), tr("Password verified").arg(tr(PACKAGE_NAME)) + "<br><br><b></b></qt>");
        close();
    } else {
        QMessageBox::critical(this, tr("failed"), tr("The supplied passphrases do not match."));
    }
}

void SetPassphraseDialog::textChanged() {
    int len =  ui->passEdit2->text().toStdString().length();
    bool len_ok = (len > 3);
    // Validate input, set Ok button to enabled when acceptable
    // (i.e both fields not empty, matching and greater than minimum length
    bool acceptable = ui->passEdit2->text() == ui->passEdit3->text() && !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty() && len_ok;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}

bool SetPassphraseDialog::event(QEvent *event) {
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
        if (fCapsLock) {
            ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
        } else {
            ui->capsLabel->clear();
        }
    }
    return QWidget::event(event);
}

bool SetPassphraseDialog::eventFilter(QObject *object, QEvent *event) {
    /* Detect Caps Lock.
     * There is no good OS-independent way to check a key state in Qt, but we
     * can detect Caps Lock by checking for the following condition:
     * Shift key is down and the result is a lower case character, or
     * Shift key is not down and the result is an upper case character.
     */
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QString str = ke->text();
        if (str.length() != 0) {
            const QChar *psz = str.unicode();
            bool fShift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if ((fShift && *psz >= 'a' && *psz <= 'z') ||
                (!fShift && *psz >= 'A' && *psz <= 'Z')) {
                fCapsLock = true;
                ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
            } else if (psz->isLetter()) {
                fCapsLock = false;
                ui->capsLabel->clear();
            }
        }
    }
    return QDialog::eventFilter(object, event);
}

static void SecureClearQLineEdit(QLineEdit *edit) {
    // Attempt to overwrite text so that they do not linger around in memory
    edit->setText(QString(" ").repeated(edit->text().size()));
    edit->clear();
}

void SetPassphraseDialog::secureClearPassFields() {
    SecureClearQLineEdit(ui->passEdit1);
    SecureClearQLineEdit(ui->passEdit2);
    SecureClearQLineEdit(ui->passEdit3);
}
