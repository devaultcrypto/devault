// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/dnftdialogs.h>

#include <feerate.h>
#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/dnftrpc.h>
#include <qt/dvtui.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <util/strencodings.h>
#include <validation.h> // DEFAULT_MIN_RELAY_TX_FEE_PER_KB

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

//! Spec Q2 (v1): an envelope must fit its 1 MB mint tx — effective content cap ~990 KB.
constexpr qint64 CONTENT_CAP = 990'000;

//! Estimate the mint network fee the way the node prices relay: the DeVault-quantized
//! CFeeRate::GetFee (0.5 DVT/kB with the 0.2 DVT floor, whole-spock rounding) over the
//! approximate mint tx size (content + envelope/token/tx overhead).
Amount estimateMintFee(qint64 contentSize) {
    const CFeeRate rate(DEFAULT_MIN_RELAY_TX_FEE_PER_KB);
    return rate.GetFee(size_t(contentSize + 600));
}

QString formatDvt(const Amount &amt) {
    // Simple DVT formatting (8 decimals, trimmed) — display-only.
    const int64_t sats = amt / SATOSHI;
    const int64_t coin = COIN / SATOSHI;
    QString s = QString::number(sats / coin) + QLatin1Char('.') +
                QStringLiteral("%1").arg(sats % coin, 8, 10, QLatin1Char('0'));
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s;
}

} // namespace

// ---------------------------------------------------------------- DnftMintDialog

DnftMintDialog::DnftMintDialog(const PlatformStyle *_platformStyle, WalletModel *model,
                               QWidget *parent)
    : QDialog(parent), walletModel(model), platformStyle(_platformStyle) {
    setWindowTitle(tr("Mint a digital artifact"));
    setMinimumWidth(560);

    auto *v = new QVBoxLayout(this);
    v->setSpacing(10);

    auto *intro = new QLabel(
        tr("The file's bytes are inscribed onchain inside the mint transaction and become a "
           "consensus-validated NFT owned by this wallet. Content is public and immutable "
           "forever."));
    intro->setWordWrap(true);
    intro->setStyleSheet("color: " + DVTUI::s_grey + ";");
    v->addWidget(intro);

    auto *form = new QFormLayout();
    form->setSpacing(8);

    auto *fileRow = new QHBoxLayout();
    fileEdit = new QLineEdit();
    fileEdit->setReadOnly(true);
    fileEdit->setPlaceholderText(tr("Choose a file to inscribe…"));
    fileRow->addWidget(fileEdit, 1);
    auto *browse = new QPushButton(tr("Browse…"));
    browse->setCursor(Qt::PointingHandCursor);
    connect(browse, &QPushButton::clicked, this, &DnftMintDialog::onBrowse);
    fileRow->addWidget(browse);
    form->addRow(tr("File"), fileRow);

    typeCombo = new QComboBox();
    typeCombo->setEditable(true);
    typeCombo->addItems({QStringLiteral("image/png"), QStringLiteral("image/jpeg"),
                         QStringLiteral("image/gif"), QStringLiteral("image/webp"),
                         QStringLiteral("image/svg+xml"), QStringLiteral("text/plain"),
                         QStringLiteral("text/html"), QStringLiteral("application/json"),
                         QStringLiteral("application/octet-stream")});
    form->addRow(tr("Content type"), typeCombo);

    recipientEdit = new QLineEdit();
    recipientEdit->setPlaceholderText(tr("optional — a fresh wallet address is used if empty"));
    auto *recipRow = new QHBoxLayout();
    recipRow->addWidget(recipientEdit, 1);
    auto *bookBtn = new QPushButton();
    bookBtn->setIcon(platformStyle->SingleColorIcon(":/icons/address-book"));
    bookBtn->setToolTip(tr("Choose a previously used address"));
    bookBtn->setCursor(Qt::PointingHandCursor);
    connect(bookBtn, &QPushButton::clicked, this, &DnftMintDialog::onAddressBook);
    recipRow->addWidget(bookBtn);
    form->addRow(tr("Recipient"), recipRow);
    v->addLayout(form);

    previewLabel = new QLabel();
    previewLabel->setMinimumHeight(140);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("background: " + DVTUI::s_Darker + "; border: 1px solid " +
                                DVTUI::s_Dark + "; border-radius: 6px;");
    v->addWidget(previewLabel);

    auto *info = new QHBoxLayout();
    sizeLabel = new QLabel();
    feeLabel = new QLabel();
    for (QLabel *l : {sizeLabel, feeLabel}) {
        l->setStyleSheet("color: " + DVTUI::s_Light + ";");
        info->addWidget(l);
    }
    info->addStretch();
    v->addLayout(info);

    warnLabel = new QLabel();
    warnLabel->setWordWrap(true);
    warnLabel->setStyleSheet("color: #ff6b6b;");
    warnLabel->hide();
    v->addWidget(warnLabel);

    advancedBox = new QGroupBox(tr("Advanced"));
    advancedBox->setCheckable(true);
    advancedBox->setChecked(false);
    {
        auto *af = new QFormLayout(advancedBox);
        delegateEdit = new QLineEdit();
        delegateEdit->setPlaceholderText(tr("delegate item id bytes, hex (renders another item's content)"));
        af->addRow(tr("Delegate"), delegateEdit);
        metadataEdit = new QLineEdit();
        metadataEdit->setPlaceholderText(tr("CBOR metadata, hex"));
        af->addRow(tr("Metadata"), metadataEdit);
        encodingEdit = new QLineEdit();
        encodingEdit->setPlaceholderText(tr("content-encoding, e.g. br"));
        af->addRow(tr("Encoding"), encodingEdit);
        // Collapse/expand behavior for the checkable group (clamp the frame when collapsed).
        const auto applyCollapsed = [this] {
            const bool open = advancedBox->isChecked();
            for (QWidget *w : advancedBox->findChildren<QWidget *>())
                w->setVisible(open);
            advancedBox->setMaximumHeight(open ? QWIDGETSIZE_MAX : 24);
        };
        connect(advancedBox, &QGroupBox::toggled, this, applyCollapsed);
        applyCollapsed();
    }
    v->addWidget(advancedBox);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancelBtn);
    mintBtn = new QPushButton(tr("Mint"));
    mintBtn->setDefault(true);
    mintBtn->setEnabled(false);
    mintBtn->setStyleSheet("QPushButton { background: " + DVTUI::s_DVTBlue +
                           "; color: #fff; } QPushButton:hover { background: " + DVTUI::s_LBlue +
                           "; } QPushButton:disabled { background: " + DVTUI::s_Dark + "; }");
    connect(mintBtn, &QPushButton::clicked, this, &DnftMintDialog::onMint);
    buttons->addWidget(mintBtn);
    v->addLayout(buttons);

    updatePreview();
}

void DnftMintDialog::onBrowse() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Choose the file to inscribe"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Mint"), tr("Could not read %1.").arg(path));
        return;
    }
    content = f.readAll();
    filePath = path;
    fileEdit->setText(path);
    // Content-based mime detection (falls back to the extension, then octet-stream).
    const QMimeType mt = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchContent);
    typeCombo->setCurrentText(mt.isValid() && !mt.isDefault() ? mt.name()
                                                              : QStringLiteral("application/octet-stream"));
    updatePreview();
}

void DnftMintDialog::updatePreview() {
    const qint64 size = content.size();
    sizeLabel->setText(tr("Size: %1").arg(GUIUtil::formatBytes(uint64_t(size))));
    feeLabel->setText(tr("Estimated fee: ~%1 DVT (+ ~0.6 DVT postage locked with the artifact)")
                          .arg(formatDvt(estimateMintFee(size))));

    bool ok = size > 0;
    if (size == 0) {
        previewLabel->setText(tr("No file selected"));
    } else if (size > CONTENT_CAP) {
        ok = false;
        warnLabel->setText(tr("This file is %1 — the v1 onchain cap is %2 (the mint must fit a "
                              "1 MB transaction).")
                               .arg(GUIUtil::formatBytes(uint64_t(size)),
                                    GUIUtil::formatBytes(uint64_t(CONTENT_CAP))));
        warnLabel->show();
    } else {
        warnLabel->hide();
    }
    if (size > 0) {
        const QString type = typeCombo->currentText();
        if (type.startsWith(QLatin1String("image/")) && type != QLatin1String("image/svg+xml")) {
            const QImage img = QImage::fromData(content);
            if (!img.isNull()) {
                previewLabel->setPixmap(QPixmap::fromImage(
                    img.scaled(QSize(360, 136), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                previewLabel->setText(tr("(image preview unavailable)"));
            }
        } else if (type.startsWith(QLatin1String("text/")) ||
                   type == QLatin1String("application/json")) {
            previewLabel->setText(QString::fromUtf8(content.left(300)));
        } else {
            previewLabel->setText(tr("%1 — %2")
                                      .arg(type, GUIUtil::formatBytes(uint64_t(size))));
        }
    }
    mintBtn->setEnabled(ok);
}

void DnftMintDialog::onAddressBook() {
    if (!walletModel) return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection,
                        AddressBookPage::ReceivingTab, this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec()) {
        recipientEdit->setText(dlg.getReturnValue());
    }
}

void DnftMintDialog::onMint() {
    if (content.isEmpty() || content.size() > CONTENT_CAP) return;

    const QString recipient = recipientEdit->text().trimmed();
    if (!recipient.isEmpty() && !walletModel->validateAddress(recipient)) {
        QMessageBox::warning(this, tr("Mint"), tr("The recipient address is not valid."));
        return;
    }
    const QString type = typeCombo->currentText().trimmed();

    // Confirm (the artifact is public + immutable; the fee is real money).
    const QString summary =
        tr("Inscribe <b>%1</b> (%2, %3) onchain?<br><br>Estimated fee ~%4 DVT. The content "
           "becomes public and can never be altered or deleted.")
            .arg(QFileInfo(filePath).fileName().toHtmlEscaped(), type.toHtmlEscaped(),
                 GUIUtil::formatBytes(uint64_t(content.size())),
                 formatDvt(estimateMintFee(content.size())));
    if (QMessageBox::question(this, tr("Confirm mint"), summary,
                              QMessageBox::Yes | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Yes) {
        return;
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    UniValue::Array params;
    params.emplace_back(HexStr(std::vector<uint8_t>(content.begin(), content.end())));
    params.emplace_back(type.toStdString());
    UniValue::Object options;
    if (!recipient.isEmpty()) options.emplace_back("recipient", recipient.toStdString());
    if (advancedBox->isChecked()) {
        const QString delegate = delegateEdit->text().trimmed();
        const QString metadata = metadataEdit->text().trimmed();
        const QString encoding = encodingEdit->text().trimmed();
        if (!delegate.isEmpty()) options.emplace_back("delegate", delegate.toStdString());
        if (!metadata.isEmpty()) options.emplace_back("metadata", metadata.toStdString());
        if (!encoding.isEmpty()) options.emplace_back("content_encoding", encoding.toStdString());
    }
    if (!options.empty()) params.emplace_back(std::move(options));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    const auto result = DnftRpc::call(walletModel, "mintnft", std::move(params), &err);
    QApplication::restoreOverrideCursor();
    if (!result) {
        QMessageBox::critical(this, tr("Mint failed"), err);
        return;
    }
    QString itemId;
    if (const UniValue *v = result->locate("item_id"); v && v->isStr()) {
        itemId = QString::fromStdString(v->get_str());
    }
    QMessageBox done(QMessageBox::Information, tr("Artifact minted"),
                     tr("Minted <b>%1</b>.<br>It will appear with its first confirmation.")
                         .arg(itemId.toHtmlEscaped()),
                     QMessageBox::Ok, this);
    auto *copyBtn = done.addButton(tr("Copy item id"), QMessageBox::ActionRole);
    done.exec();
    if (done.clickedButton() == copyBtn) {
        GUIUtil::setClipboard(itemId);
    }
    accept();
}

// ---------------------------------------------------------------- DnftSendDialog

DnftSendDialog::DnftSendDialog(const PlatformStyle *_platformStyle, WalletModel *model,
                               const DnftPage::Item &_item, const QPixmap &thumb, QWidget *parent)
    : QDialog(parent), walletModel(model), platformStyle(_platformStyle), item(_item) {
    setWindowTitle(tr("Send digital artifact"));
    setMinimumWidth(520);

    auto *v = new QVBoxLayout(this);
    v->setSpacing(10);

    auto *head = new QHBoxLayout();
    auto *pic = new QLabel();
    pic->setPixmap(thumb);
    head->addWidget(pic);
    auto *what = new QLabel(
        tr("<b>%1</b><br>%2")
            .arg((item.itemId.isEmpty() ? item.commitment.left(16) : item.itemId).toHtmlEscaped(),
                 item.contentType.toHtmlEscaped()));
    what->setTextFormat(Qt::RichText);
    head->addWidget(what, 1);
    v->addLayout(head);

    auto *form = new QFormLayout();
    auto *row = new QHBoxLayout();
    addressEdit = new QLineEdit();
    addressEdit->setPlaceholderText(tr("devault:… address of the receiver"));
    row->addWidget(addressEdit, 1);
    auto *bookBtn = new QPushButton(tr("Book…"));
    bookBtn->setCursor(Qt::PointingHandCursor);
    connect(bookBtn, &QPushButton::clicked, this, &DnftSendDialog::onAddressBook);
    row->addWidget(bookBtn);
    form->addRow(tr("Send to"), row);
    v->addLayout(form);

    auto *note = new QLabel(tr("The artifact's token (with its %1 DVT postage) moves to the "
                               "receiver; the network fee is paid from your balance.")
                                .arg(item.amount));
    note->setWordWrap(true);
    note->setStyleSheet("color: " + DVTUI::s_grey + ";");
    v->addWidget(note);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancelBtn);
    auto *sendBtn = new QPushButton(tr("Send"));
    sendBtn->setDefault(true);
    connect(sendBtn, &QPushButton::clicked, this, &DnftSendDialog::onSend);
    buttons->addWidget(sendBtn);
    v->addLayout(buttons);
}

void DnftSendDialog::onAddressBook() {
    // Same picker the coin Send screen uses.
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::SendingTab,
                        this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec()) {
        addressEdit->setText(dlg.getReturnValue());
    }
}

void DnftSendDialog::onSend() {
    const QString addr = addressEdit->text().trimmed();
    if (!walletModel->validateAddress(addr)) {
        QMessageBox::warning(this, tr("Send artifact"), tr("This is not a valid DeVault address."));
        return;
    }
    const QString name = item.itemId.isEmpty() ? item.commitment.left(16) : item.itemId;
    if (QMessageBox::question(
            this, tr("Confirm send"),
            tr("Send <b>%1</b> to<br><span style='font-family: monospace'>%2</span>?")
                .arg(name.toHtmlEscaped(), addr.toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) {
        return;
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    UniValue::Array params;
    params.emplace_back(item.category.toStdString());
    params.emplace_back(item.commitment.toStdString());
    params.emplace_back(addr.toStdString());
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    const auto result = DnftRpc::call(walletModel, "sendnft", std::move(params), &err);
    QApplication::restoreOverrideCursor();
    if (!result) {
        QMessageBox::critical(this, tr("Send failed"), err);
        return;
    }
    accept();
}

// ---------------------------------------------------------------- DnftBurnDialog

DnftBurnDialog::DnftBurnDialog(WalletModel *model, const DnftPage::Item &_item,
                               const QPixmap &thumb, QWidget *parent)
    : QDialog(parent), walletModel(model), item(_item) {
    setWindowTitle(tr("Burn digital artifact"));
    setMinimumWidth(520);

    auto *v = new QVBoxLayout(this);
    v->setSpacing(10);

    // Confirmation 1: the explicit warning, red-framed.
    auto *warn = new QLabel(
        tr("<b>You are about to permanently destroy this artifact.</b><br>The token is spent "
           "into a plain output: ownership ends, the postage returns to this wallet, and the "
           "item can never be transferred again. The onchain content remains public."));
    warn->setWordWrap(true);
    warn->setStyleSheet("color: #ff6b6b; border: 1px solid #ff6b6b; border-radius: 6px; "
                        "padding: 10px; background: #2a1515;");
    v->addWidget(warn);

    auto *head = new QHBoxLayout();
    auto *pic = new QLabel();
    pic->setPixmap(thumb);
    head->addWidget(pic);
    auto *what = new QLabel(
        tr("<b>%1</b><br>%2 — %3 confirmations")
            .arg((item.itemId.isEmpty() ? item.commitment.left(16) : item.itemId).toHtmlEscaped(),
                 item.contentType.toHtmlEscaped(), QString::number(item.confirmations)));
    what->setTextFormat(Qt::RichText);
    head->addWidget(what, 1);
    v->addLayout(head);

    // Confirmation 2: type-to-confirm.
    auto *form = new QFormLayout();
    confirmEdit = new QLineEdit();
    confirmEdit->setPlaceholderText(tr("type BURN to enable the button"));
    connect(confirmEdit, &QLineEdit::textChanged, this, &DnftBurnDialog::onConfirmTextChanged);
    form->addRow(tr("Confirmation"), confirmEdit);
    v->addLayout(form);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setDefault(true);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancelBtn);
    burnBtn = new QPushButton(tr("Burn forever"));
    burnBtn->setEnabled(false);
    burnBtn->setStyleSheet("QPushButton { color: #ff6b6b; } QPushButton:hover { background: "
                           "#7a1f1f; border: 1px solid #ff6b6b; } QPushButton:disabled { color: " +
                           DVTUI::s_grey + "; }");
    connect(burnBtn, &QPushButton::clicked, this, &DnftBurnDialog::onBurn);
    buttons->addWidget(burnBtn);
    v->addLayout(buttons);
}

void DnftBurnDialog::onConfirmTextChanged(const QString &text) {
    burnBtn->setEnabled(text.trimmed().compare(QLatin1String("BURN")) == 0);
}

void DnftBurnDialog::onBurn() {
    // Confirmation 3: the final yes/no.
    const QString name = item.itemId.isEmpty() ? item.commitment.left(16) : item.itemId;
    if (QMessageBox::warning(
            this, tr("Final confirmation"),
            tr("Burn <b>%1</b>? This cannot be undone.").arg(name.toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) {
        return;
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    UniValue::Array params;
    params.emplace_back(item.category.toStdString());
    params.emplace_back(item.commitment.toStdString());
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    const auto result = DnftRpc::call(walletModel, "burnnft", std::move(params), &err);
    QApplication::restoreOverrideCursor();
    if (!result) {
        QMessageBox::critical(this, tr("Burn failed"), err);
        return;
    }
    accept();
}
