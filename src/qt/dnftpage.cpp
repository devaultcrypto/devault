// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/dnftpage.h>

#include <core_io.h>
#include <devault/dnft_envelope.h>
#include <primitives/transaction.h>
#include <qt/dnftdialogs.h>
#include <qt/dnftrpc.h>
#include <qt/dvtui.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <util/strencodings.h>

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int THUMB_SIZE = 96;
//! Bodies larger than this are dropped after thumbnailing; "Save content" re-resolves on demand.
constexpr qint64 KEEP_BODY_MAX = 256 * 1024;

QString shortHex(const QString &hex, int keep = 10) {
    return hex.size() <= keep ? hex : hex.left(keep) + QStringLiteral("…");
}

QString mimeTop(const QString &contentType) {
    const int slash = contentType.indexOf(QLatin1Char('/'));
    return slash < 0 ? contentType : contentType.left(slash);
}

QString mimeSub(const QString &contentType) {
    const int slash = contentType.indexOf(QLatin1Char('/'));
    QString sub = slash < 0 ? contentType : contentType.mid(slash + 1);
    const int plus = sub.indexOf(QLatin1Char('+')); // e.g. svg+xml -> svg
    if (plus > 0) sub = sub.left(plus);
    return sub;
}

//! Paint the standard placeholder / text-snippet card used by the grid and the detail preview.
QPixmap paintCard(const QSize &size, const QString &big, const QString &small,
                  const QString &snippet = QString()) {
    QPixmap pm(size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(DVTUI::c_Dark);
    p.drawRoundedRect(QRectF(0.5, 0.5, size.width() - 1, size.height() - 1), 8, 8);
    if (!snippet.isEmpty()) {
        QFont f = GUIUtil::fixedPitchFont();
        f.setPixelSize(std::max(9, size.height() / 11));
        p.setFont(f);
        p.setPen(DVTUI::c_Light);
        const QRectF r(8, 8, size.width() - 16, size.height() - 16);
        p.drawText(r, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                   p.fontMetrics().elidedText(snippet, Qt::ElideRight, int(r.width() * 8)));
    } else {
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(size.height() / 4);
        p.setFont(f);
        p.setPen(DVTUI::c_LBlue);
        p.drawText(QRectF(0, size.height() * 0.18, size.width(), size.height() * 0.42),
                   Qt::AlignCenter, big);
        f.setBold(false);
        f.setPixelSize(std::max(9, size.height() / 9));
        p.setFont(f);
        p.setPen(DVTUI::c_Light);
        p.drawText(QRectF(4, size.height() * 0.62, size.width() - 8, size.height() * 0.3),
                   Qt::AlignCenter,
                   p.fontMetrics().elidedText(small, Qt::ElideMiddle, size.width() - 12));
    }
    return pm;
}

//! Render content into a pixmap of the given size (image decode / text snippet / typed card).
QPixmap renderContent(const DnftPage::Item &item, const QSize &size) {
    if (!item.resolved) {
        return paintCard(size, QStringLiteral("?"),
                         item.contentType.isEmpty() ? QObject::tr("content unavailable")
                                                    : item.contentType);
    }
    const QString top = mimeTop(item.contentType);
    if (top == QLatin1String("image") && !item.body.isEmpty() &&
        item.contentType != QLatin1String("image/svg+xml")) {
        QImage img = QImage::fromData(item.body);
        if (!img.isNull()) {
            QPixmap pm(size);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            const QImage scaled =
                img.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            p.drawImage(QPointF((size.width() - scaled.width()) / 2.0,
                                (size.height() - scaled.height()) / 2.0),
                        scaled);
            return pm;
        }
    }
    if ((top == QLatin1String("text") || item.contentType == QLatin1String("application/json")) &&
        !item.body.isEmpty()) {
        const QString snippet = QString::fromUtf8(item.body.left(400));
        return paintCard(size, QString(), QString(), snippet);
    }
    return paintCard(size, mimeSub(item.contentType).toUpper(),
                     GUIUtil::formatBytes(uint64_t(std::max<qint64>(item.contentLength, 0))));
}

} // namespace

DnftPage::DnftPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent), platformStyle(_platformStyle) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(16, 12, 16, 12);
    outer->setSpacing(10);

    // ---- header ----
    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("Digital Artifacts"));
    title->setObjectName("dnft_title");
    title->setStyleSheet("#dnft_title { color: qlineargradient(x0:0, y0:0, x1: 1, y1: 0, stop: 0 " +
                         DVTUI::s_LBlue + ", stop: 0.3 " + DVTUI::s_green +
                         "); font-size: 24px; }");
    header->addWidget(title);
    countLabel = new QLabel();
    countLabel->setStyleSheet("color: " + DVTUI::s_grey + "; font-size: 14px; margin-left: 8px;");
    header->addWidget(countLabel);
    header->addStretch();
    refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(refreshButton);
    mintButton = new QPushButton(tr("Mint…"));
    mintButton->setCursor(Qt::PointingHandCursor);
    mintButton->setStyleSheet("QPushButton { background: " + DVTUI::s_DVTBlue +
                              "; color: #fff; } QPushButton:hover { background: " +
                              DVTUI::s_LBlue + "; }");
    header->addWidget(mintButton);
    outer->addLayout(header);

    // ---- stacked: empty state / grid+detail ----
    stack = new QStackedWidget(this);
    outer->addWidget(stack, 1);

    // Empty state
    emptyPage = new QWidget();
    {
        auto *v = new QVBoxLayout(emptyPage);
        v->addStretch();
        auto *icon = new QLabel();
        icon->setPixmap(platformStyle->SingleColorIcon(":/icons/nft").pixmap(72, 72));
        icon->setAlignment(Qt::AlignCenter);
        v->addWidget(icon);
        auto *txt = new QLabel(tr("No digital artifacts in this wallet yet."));
        txt->setAlignment(Qt::AlignCenter);
        txt->setStyleSheet("font-size: 18px; color: " + DVTUI::s_grey + ";");
        v->addWidget(txt);
        auto *hint = new QLabel(tr("Mint an image, text, or any file onchain — it becomes a "
                                   "consensus-validated NFT owned by this wallet."));
        hint->setAlignment(Qt::AlignCenter);
        hint->setWordWrap(true);
        hint->setStyleSheet("color: " + DVTUI::s_grey + ";");
        v->addWidget(hint);
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        auto *mintFirst = new QPushButton(tr("Mint your first artifact"));
        mintFirst->setCursor(Qt::PointingHandCursor);
        connect(mintFirst, &QPushButton::clicked, this, &DnftPage::onMintClicked);
        btnRow->addWidget(mintFirst);
        btnRow->addStretch();
        v->addLayout(btnRow);
        v->addStretch();
    }
    stack->addWidget(emptyPage);

    // Grid + detail
    gridPage = new QWidget();
    {
        auto *h = new QHBoxLayout(gridPage);
        h->setContentsMargins(0, 0, 0, 0);
        auto *split = new QSplitter(Qt::Horizontal, gridPage);
        h->addWidget(split);

        grid = new QListWidget();
        grid->setViewMode(QListView::IconMode);
        grid->setIconSize(QSize(THUMB_SIZE, THUMB_SIZE));
        grid->setResizeMode(QListView::Adjust);
        grid->setMovement(QListView::Static);
        grid->setSelectionMode(QAbstractItemView::SingleSelection);
        grid->setWordWrap(false); // single elided caption line under each thumbnail
        grid->setTextElideMode(Qt::ElideRight);
        grid->setSpacing(8);
        grid->setFrameShape(QFrame::NoFrame);
        grid->setStyleSheet("QListWidget { background: " + DVTUI::s_Darker +
                            "; } QListWidget::item { color: " + DVTUI::s_Light +
                            "; border-radius: 6px; padding: 2px; } QListWidget::item:selected { "
                            "background: " + DVTUI::s_highlight_dark_midgrey + "; color: " +
                            DVTUI::s_LBlue + "; }");
        split->addWidget(grid);

        // Detail panel
        detailPanel = new QFrame();
        detailPanel->setObjectName("dnft_detail");
        detailPanel->setStyleSheet("#dnft_detail { background: " + DVTUI::s_hightlight_dark +
                                   "; border: 1px solid " + DVTUI::s_Dark +
                                   "; border-radius: 8px; }");
        auto *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setMinimumWidth(340);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        {
            auto *dv = new QVBoxLayout(detailPanel);
            dv->setContentsMargins(14, 14, 14, 14);
            dv->setSpacing(8);

            previewLabel = new QLabel();
            previewLabel->setMinimumSize(260, 200);
            previewLabel->setAlignment(Qt::AlignCenter);
            previewLabel->setStyleSheet("background: " + DVTUI::s_Darker +
                                        "; border: 1px solid " + DVTUI::s_Dark +
                                        "; border-radius: 6px;");
            dv->addWidget(previewLabel);

            // Item id: elided display (the 66-char id has no break points), full id in the
            // tooltip, one-click copy.
            auto *idRow = new QHBoxLayout();
            itemIdValue = new QLabel();
            itemIdValue->setFont(GUIUtil::fixedPitchFont());
            itemIdValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
            itemIdValue->setStyleSheet("color: " + DVTUI::s_LBlue + "; font-size: 13px;");
            idRow->addWidget(itemIdValue, 1);
            auto *copyIdBtn = new QPushButton(tr("Copy"));
            copyIdBtn->setCursor(Qt::PointingHandCursor);
            copyIdBtn->setToolTip(tr("Copy the full item id"));
            connect(copyIdBtn, &QPushButton::clicked, this, [this] {
                if (const Item *it = selectedItem()) {
                    GUIUtil::setClipboard(it->itemId.isEmpty() ? it->commitment : it->itemId);
                }
            });
            idRow->addWidget(copyIdBtn);
            dv->addLayout(idRow);

            verifiedBadge = new QLabel();
            dv->addWidget(verifiedBadge);

            auto addRow = [&](const QString &name, QLabel *&value) {
                auto *row = new QHBoxLayout();
                auto *k = new QLabel(name);
                k->setStyleSheet("color: " + DVTUI::s_grey + ";");
                k->setMinimumWidth(96);
                row->addWidget(k);
                value = new QLabel();
                value->setTextInteractionFlags(Qt::TextSelectableByMouse);
                value->setWordWrap(true);
                row->addWidget(value, 1);
                dv->addLayout(row);
            };
            addRow(tr("Collection"), categoryValue);
            categoryValue->setFont(GUIUtil::fixedPitchFont());
            addRow(tr("Type"), typeValue);
            addRow(tr("Size"), sizeValue);
            addRow(tr("Confirmations"), confsValue);
            addRow(tr("Supply"), collectionValue);
            addRow(tr("Postage"), postageValue);

            dv->addStretch();

            auto *actions = new QHBoxLayout();
            sendButton = new QPushButton(tr("Send…"));
            saveButton = new QPushButton(tr("Save…"));
            burnButton = new QPushButton(tr("Burn…"));
            burnButton->setStyleSheet("QPushButton { color: #ff6b6b; } QPushButton:hover { "
                                      "background: #7a1f1f; border: 1px solid #ff6b6b; }");
            for (QPushButton *b : {sendButton, saveButton, burnButton}) {
                b->setCursor(Qt::PointingHandCursor);
                actions->addWidget(b);
            }
            dv->addLayout(actions);
        }
        scroll->setWidget(detailPanel);
        split->addWidget(scroll);
        split->setStretchFactor(0, 1);
        split->setStretchFactor(1, 0);
        split->setChildrenCollapsible(false);
        split->setSizes({560, 360});
    }
    stack->addWidget(gridPage);
    stack->setCurrentWidget(emptyPage);

    connect(refreshButton, &QPushButton::clicked, this, &DnftPage::refresh);
    connect(mintButton, &QPushButton::clicked, this, &DnftPage::onMintClicked);
    connect(sendButton, &QPushButton::clicked, this, &DnftPage::onSendClicked);
    connect(saveButton, &QPushButton::clicked, this, &DnftPage::onSaveClicked);
    connect(burnButton, &QPushButton::clicked, this, &DnftPage::onBurnClicked);
    connect(grid, &QListWidget::itemSelectionChanged, this, &DnftPage::onSelectionChanged);
}

void DnftPage::setWalletModel(WalletModel *model) {
    walletModel = model;
    if (walletModel) {
        // New wallet transactions (mints, receives, sends) change the holdings.
        connect(walletModel, &WalletModel::balanceChanged, this, &DnftPage::scheduleRefresh);
    }
    m_dirty = true;
}

void DnftPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_dirty) {
        refresh();
    }
}

void DnftPage::scheduleRefresh() {
    if (isVisible()) {
        refresh();
    } else {
        m_dirty = true;
    }
}

void DnftPage::resolveContent(Item &item) {
    // Resolve the item's content by fetching a candidate mint transaction over the verified RPC
    // path and recomputing the §6.4 binding hash with the consensus codec. Never display bytes
    // whose hash does not match the commitment the wallet holds.
    const std::vector<uint8_t> commitBytes = ParseHex(item.commitment.toStdString());

    const auto tryTx = [&](const QString &txid, int tokenVout) -> bool {
        QString err;
        std::string hex;
        UniValue::Array p1;
        p1.emplace_back(txid.toStdString());
        if (auto r = DnftRpc::call(walletModel, "gettransaction", std::move(p1), &err)) {
            if (const UniValue *h = r->locate("hex"); h && h->isStr()) hex = h->get_str();
        }
        if (hex.empty()) { // not a wallet tx — try the (txindex/mempool-served) node path
            UniValue::Array p2;
            p2.emplace_back(txid.toStdString());
            if (auto r = DnftRpc::call(walletModel, "getrawtransaction", std::move(p2), &err)) {
                if (r->isStr()) hex = r->get_str();
            }
        }
        if (hex.empty()) return false;
        CMutableTransaction mtx;
        if (!DecodeHexTx(mtx, hex) || mtx.vin.empty() || tokenVout < 0 ||
            size_t(tokenVout) >= mtx.vout.size()) {
            return false;
        }
        // Find the envelope whose recomputed binding hash matches our commitment; count its
        // position among the tx's envelopes (pairing rule) for the ord-style item index.
        int envIndex = -1, seen = 0;
        for (size_t n = 0; n < mtx.vout.size(); ++n) {
            const CScript &spk = mtx.vout[n].scriptPubKey;
            if (!dnft::IsDnftEnvelope(spk)) continue;
            const auto computed =
                dnft::ComputeDnftCommitment(spk, mtx.vin[0].prevout, uint32_t(tokenVout));
            if (commitBytes.size() == computed.size() &&
                std::equal(computed.begin(), computed.end(), commitBytes.begin())) {
                envIndex = int(n);
                break;
            }
            ++seen;
        }
        if (envIndex < 0) return false;
        const dnft::ParsedEnvelope pe = dnft::ParseDnftEnvelope(mtx.vout[envIndex].scriptPubKey);
        if (!pe.valid) return false;
        item.resolved = true;
        item.verified = true;
        item.mintTxid = txid;
        item.mintVout = tokenVout;
        item.itemId = txid + QStringLiteral("i") + QString::number(seen);
        if (pe.content_type) {
            item.contentType = QString::fromUtf8(
                reinterpret_cast<const char *>(pe.content_type->data()), int(pe.content_type->size()));
        }
        item.body = QByteArray(reinterpret_cast<const char *>(pe.body.data()), int(pe.body.size()));
        item.contentLength = qint64(pe.body.size());
        return true;
    };

    // 1) The common case: the token never moved, so its current tx IS the mint tx.
    if (tryTx(item.txid, item.vout)) return;

    // 2) The token moved: ask the optional -nftindex for the mint location, then fetch that tx.
    QString err;
    UniValue::Array p;
    p.emplace_back(item.category.toStdString());
    p.emplace_back(item.commitment.toStdString());
    if (auto r = DnftRpc::call(walletModel, "getnftitem", std::move(p), &err)) {
        if (const UniValue *ct = r->locate("content_type"); ct && ct->isStr()) {
            item.contentType = QString::fromStdString(ct->get_str());
        }
        if (const UniValue *cl = r->locate("content_length"); cl && cl->isNum()) {
            item.contentLength = cl->get_int64();
        }
        const UniValue *mt = r->locate("mint_txid");
        const UniValue *mv = r->locate("mint_vout");
        if (mt && mt->isStr() && mv && mv->isNum()) {
            item.mintTxid = QString::fromStdString(mt->get_str());
            item.mintVout = mv->get_int();
            tryTx(item.mintTxid, item.mintVout); // sets resolved/verified on success
        }
    }
}

QPixmap DnftPage::thumbnailFor(const Item &item) {
    auto it = m_thumbCache.find(item.commitment);
    if (it != m_thumbCache.end()) return it.value();
    const QPixmap pm = renderContent(item, QSize(THUMB_SIZE, THUMB_SIZE));
    m_thumbCache.insert(item.commitment, pm);
    return pm;
}

void DnftPage::refresh() {
    if (!walletModel) return;
    m_dirty = false;
    QApplication::setOverrideCursor(Qt::WaitCursor);

    const QString prevSelected = selectedItem() ? selectedItem()->commitment : QString();
    m_items.clear();

    QString err;
    if (auto r = DnftRpc::call(walletModel, "listnfts", UniValue::Array{}, &err)) {
        if (r->isArray()) {
            for (size_t i = 0; i < r->size(); ++i) {
                const UniValue &e = (*r)[i];
                Item item;
                if (const UniValue *v = e.locate("category"); v && v->isStr())
                    item.category = QString::fromStdString(v->get_str());
                if (const UniValue *v = e.locate("commitment"); v && v->isStr())
                    item.commitment = QString::fromStdString(v->get_str());
                if (const UniValue *v = e.locate("txid"); v && v->isStr())
                    item.txid = QString::fromStdString(v->get_str());
                if (const UniValue *v = e.locate("vout"); v && v->isNum())
                    item.vout = v->get_int();
                if (const UniValue *v = e.locate("confirmations"); v && v->isNum())
                    item.confirmations = v->get_int();
                if (const UniValue *v = e.locate("amount"))
                    item.amount = QString::fromStdString(v->getValStr());
                resolveContent(item);
                m_items.push_back(std::move(item));
            }
        }
    }

    // Newest first (fewest confirmations at the front) — stable for equal depths.
    std::stable_sort(m_items.begin(), m_items.end(),
                     [](const Item &a, const Item &b) { return a.confirmations < b.confirmations; });

    grid->clear();
    for (size_t i = 0; i < m_items.size(); ++i) {
        Item &item = m_items[i];
        auto *cell = new QListWidgetItem(QIcon(thumbnailFor(item)),
                                         item.itemId.isEmpty() ? shortHex(item.commitment)
                                                               : shortHex(item.itemId, 12));
        cell->setData(Qt::UserRole, int(i));
        cell->setToolTip(item.itemId.isEmpty() ? item.commitment : item.itemId);
        // Explicit cell size: thumbnail + one caption line (gridSize/uniform sizing clips).
        cell->setSizeHint(QSize(THUMB_SIZE + 28, THUMB_SIZE + 28));
        cell->setTextAlignment(Qt::AlignHCenter);
        grid->addItem(cell);
        // Keep memory bounded: big bodies are re-resolved on demand (Save / detail preview).
        if (item.body.size() > KEEP_BODY_MAX) {
            item.body.clear();
        }
    }

    countLabel->setText(m_items.empty() ? QString() : tr("%n item(s)", "", int(m_items.size())));
    stack->setCurrentWidget(m_items.empty() ? emptyPage : gridPage);

    // Restore selection (or select the first item).
    if (!m_items.empty()) {
        int row = 0;
        for (int i = 0; i < grid->count(); ++i) {
            const auto &it = m_items[size_t(grid->item(i)->data(Qt::UserRole).toInt())];
            if (it.commitment == prevSelected) {
                row = i;
                break;
            }
        }
        grid->setCurrentRow(row);
    }
    updateDetail();
    QApplication::restoreOverrideCursor();
}

const DnftPage::Item *DnftPage::selectedItem() const {
    const auto sel = grid->selectedItems();
    if (sel.isEmpty()) return nullptr;
    const int idx = sel.first()->data(Qt::UserRole).toInt();
    if (idx < 0 || size_t(idx) >= m_items.size()) return nullptr;
    return &m_items[size_t(idx)];
}

void DnftPage::onSelectionChanged() {
    updateDetail();
}

void DnftPage::updateDetail() {
    const Item *item = selectedItem();
    const bool have = item != nullptr;
    for (QPushButton *b : {sendButton, saveButton, burnButton}) b->setEnabled(have);
    if (!have) {
        previewLabel->clear();
        itemIdValue->clear();
        verifiedBadge->clear();
        for (QLabel *l : {categoryValue, typeValue, sizeValue, confsValue, collectionValue,
                          postageValue}) l->clear();
        return;
    }

    // Preview: re-resolve when the (large) body was dropped after thumbnailing.
    Item preview = *item;
    if (preview.resolved && preview.body.isEmpty() && preview.contentLength > 0) {
        resolveContent(preview);
    }
    previewLabel->setPixmap(renderContent(preview, QSize(280, 220)));

    const QString fullId = item->itemId.isEmpty() ? item->commitment : item->itemId;
    itemIdValue->setText(fullId.size() > 28
                             ? fullId.left(18) + QStringLiteral("…") + fullId.right(8)
                             : fullId);
    itemIdValue->setToolTip(fullId);
    if (item->verified) {
        verifiedBadge->setText(tr("✔ binding verified"));
        verifiedBadge->setStyleSheet("color: " + DVTUI::s_green + "; font-size: 13px;");
    } else {
        verifiedBadge->setText(tr("⚠ content not locally verifiable (mint tx unavailable)"));
        verifiedBadge->setStyleSheet("color: #e0a030; font-size: 13px;");
    }
    categoryValue->setText(shortHex(item->category, 16));
    categoryValue->setToolTip(item->category);
    typeValue->setText(item->contentType.isEmpty() ? tr("unknown") : item->contentType);
    sizeValue->setText(item->contentLength >= 0
                           ? GUIUtil::formatBytes(uint64_t(item->contentLength))
                           : tr("unknown"));
    confsValue->setText(QString::number(item->confirmations));
    postageValue->setText(item->amount + QStringLiteral(" DVT"));

    // Collection truth from the optional -nftindex (4E); degrade gracefully without it.
    collectionValue->setText(tr("—"));
    collectionValue->setToolTip(QString());
    QString err;
    UniValue::Array p;
    p.emplace_back(item->category.toStdString());
    if (auto r = DnftRpc::call(walletModel, "getnftcollection", std::move(p), &err)) {
        const UniValue *minted = r->locate("minted");
        const UniValue *open = r->locate("open");
        if (minted && minted->isNum() && open && open->isBool()) {
            collectionValue->setText(tr("%1 minted — %2")
                                         .arg(minted->get_int())
                                         .arg(open->get_bool() ? tr("open") : tr("closed")));
        }
    } else {
        collectionValue->setToolTip(err); // e.g. -nftindex not enabled
    }
}

void DnftPage::onMintClicked() {
    if (!walletModel) return;
    DnftMintDialog dlg(platformStyle, walletModel, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void DnftPage::onSendClicked() {
    const Item *item = selectedItem();
    if (!walletModel || !item) return;
    DnftSendDialog dlg(platformStyle, walletModel, *item, thumbnailFor(*item), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void DnftPage::onBurnClicked() {
    const Item *item = selectedItem();
    if (!walletModel || !item) return;
    DnftBurnDialog dlg(walletModel, *item, thumbnailFor(*item), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void DnftPage::onSaveClicked() {
    const Item *item = selectedItem();
    if (!walletModel || !item) return;

    Item full = *item;
    if (full.body.isEmpty() && full.contentLength != 0) {
        resolveContent(full);
    }
    if (!full.resolved) {
        QMessageBox::warning(this, tr("Save content"),
                             tr("The content bytes are not available locally (the mint "
                                "transaction could not be fetched)."));
        return;
    }
    QString suffix = QMimeDatabase().mimeTypeForName(full.contentType).preferredSuffix();
    if (suffix.isEmpty()) suffix = QStringLiteral("bin");
    const QString base = full.itemId.isEmpty() ? shortHex(full.commitment, 16) : full.itemId;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save artifact content"), base + QLatin1Char('.') + suffix);
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(full.body) != full.body.size()) {
        QMessageBox::critical(this, tr("Save content"), tr("Could not write %1.").arg(path));
        return;
    }
    f.close();
}
