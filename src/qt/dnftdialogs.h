// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <qt/dnftpage.h> // DnftPage::Item

#include <QByteArray>
#include <QDialog>
#include <QPixmap>
#include <QString>

class PlatformStyle;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

/**
 * DeVault 4G: mint a new DNFT. File picker with content-based mime autodetection, live
 * size/fee/postage preview, optional recipient (address book), advanced fields (delegate,
 * metadata, content-encoding). Confirms, unlocks the wallet if needed, then executes the
 * verified `mintnft` RPC.
 */
class DnftMintDialog : public QDialog {
    Q_OBJECT

public:
    DnftMintDialog(const PlatformStyle *platformStyle, WalletModel *model,
                   QWidget *parent = nullptr);

private Q_SLOTS:
    void onBrowse();
    void onAddressBook();
    void onMint();

private:
    void updatePreview();

    WalletModel *walletModel;
    const PlatformStyle *platformStyle;

    QByteArray content;
    QString filePath;

    QLineEdit *fileEdit = nullptr;
    QComboBox *typeCombo = nullptr;
    QLabel *previewLabel = nullptr;
    QLabel *sizeLabel = nullptr;
    QLabel *feeLabel = nullptr;
    QLabel *warnLabel = nullptr;
    QLineEdit *recipientEdit = nullptr;
    QGroupBox *advancedBox = nullptr;
    QLineEdit *delegateEdit = nullptr;
    QLineEdit *metadataEdit = nullptr;
    QLineEdit *encodingEdit = nullptr;
    QPushButton *mintBtn = nullptr;
};

/**
 * DeVault 4G: send a DNFT to an address (address-book integrated). Executes `sendnft`.
 */
class DnftSendDialog : public QDialog {
    Q_OBJECT

public:
    DnftSendDialog(const PlatformStyle *platformStyle, WalletModel *model,
                   const DnftPage::Item &item, const QPixmap &thumb, QWidget *parent = nullptr);

private Q_SLOTS:
    void onAddressBook();
    void onSend();

private:
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    DnftPage::Item item;
    QLineEdit *addressEdit = nullptr;
};

/**
 * DeVault 4G: burn a DNFT — the triple-confirm pattern. (1) an explicit warning dialog,
 * (2) type-to-confirm ("BURN"), (3) a final yes/no. Executes `burnnft`; the postage returns
 * to the wallet, the artifact is destroyed forever.
 */
class DnftBurnDialog : public QDialog {
    Q_OBJECT

public:
    DnftBurnDialog(WalletModel *model, const DnftPage::Item &item, const QPixmap &thumb,
                   QWidget *parent = nullptr);

private Q_SLOTS:
    void onConfirmTextChanged(const QString &text);
    void onBurn();

private:
    WalletModel *walletModel;
    DnftPage::Item item;
    QLineEdit *confirmEdit = nullptr;
    QPushButton *burnBtn = nullptr;
};
