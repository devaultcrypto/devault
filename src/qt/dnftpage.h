// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <QByteArray>
#include <QHash>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include <vector>

class PlatformStyle;
class WalletModel;

QT_BEGIN_NAMESPACE
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
QT_END_NAMESPACE

/**
 * DeVault 4G: the DNFT ("Digital Artifacts") wallet tab.
 *
 * Left: a thumbnail grid of the wallet's inscribed DNFTs (listnfts). Thumbnails are rendered by
 * LOCAL decode of the envelope body — the mint transaction is fetched over the wallet RPC path
 * (gettransaction / getrawtransaction) and the binding hash is recomputed with the consensus
 * codec before anything is displayed (belt over consensus' suspenders; foreign items whose mint
 * tx is unavailable get a typed placeholder). Right: a detail panel (item id, category,
 * collection status via the optional -nftindex, provenance) with Send / Save / Burn actions.
 * All actions execute the verified RPCs via DnftRpc — never a parallel implementation.
 */
class DnftPage : public QWidget {
    Q_OBJECT

public:
    explicit DnftPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);

    void setWalletModel(WalletModel *walletModel);

    //! One wallet-held inscribed DNFT, with its locally resolved content.
    struct Item {
        QString category;   // hex (display order, as the RPCs emit)
        QString commitment; // hex, 0x01-prefixed binding commitment
        QString txid;       // current outpoint txid
        int vout = -1;      // current outpoint index
        int confirmations = 0;
        QString amount; // postage, as the RPC formatted it

        // Content, resolved from the mint tx (empty/false when unavailable):
        bool resolved = false;
        bool verified = false; // binding hash recomputed and matched
        QString contentType;
        qint64 contentLength = -1;
        QByteArray body;
        QString mintTxid;
        int mintVout = -1;
        QString itemId; // "<mint_txid>i<n>" when the mint is known
    };

public Q_SLOTS:
    void refresh();

private Q_SLOTS:
    void onSelectionChanged();
    void onMintClicked();
    void onSendClicked();
    void onBurnClicked();
    void onSaveClicked();
    void scheduleRefresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void resolveContent(Item &item);
    QPixmap thumbnailFor(const Item &item);
    void updateDetail();
    const Item *selectedItem() const;

    WalletModel *walletModel = nullptr;
    const PlatformStyle *platformStyle;

    std::vector<Item> m_items;
    QHash<QString, QPixmap> m_thumbCache; // commitment -> thumbnail
    bool m_dirty = true;

    // Header
    QLabel *countLabel = nullptr;
    QPushButton *mintButton = nullptr;
    QPushButton *refreshButton = nullptr;

    // Body
    QStackedWidget *stack = nullptr;
    QWidget *emptyPage = nullptr;
    QWidget *gridPage = nullptr;
    QListWidget *grid = nullptr;

    // Detail panel
    QLabel *previewLabel = nullptr;
    QLabel *itemIdValue = nullptr;
    QLabel *verifiedBadge = nullptr;
    QLabel *categoryValue = nullptr;
    QLabel *typeValue = nullptr;
    QLabel *sizeValue = nullptr;
    QLabel *confsValue = nullptr;
    QLabel *collectionValue = nullptr;
    QLabel *postageValue = nullptr;
    QPushButton *sendButton = nullptr;
    QPushButton *saveButton = nullptr;
    QPushButton *burnButton = nullptr;
    QWidget *detailPanel = nullptr;
};
