// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2021 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinunits.h>

#include <primitives/transaction.h>

#include <limits>

#include <QLocale>
#include <QStringList>

BitcoinUnits::BitcoinUnits(QObject *parent)
    : QAbstractListModel(parent), unitlist(availableUnits()) {}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits() {
    QList<BitcoinUnits::Unit> unitlist;
    // DeVault: only DVT (3 decimals) and mDVT (whole spocks) are offered. µDVT and sat are sub-spock
    // (DeVault's smallest unit is the spock = 0.001 DVT = 1 mDVT), so they are intentionally omitted.
    unitlist.append(BCH);
    unitlist.append(mBCH);
    return unitlist;
}

bool BitcoinUnits::valid(int unit) {
    switch (unit) {
        case BCH:
        case mBCH:
            return true;
        default:
            // DeVault: uBCH (µDVT) and SAT are sub-spock and no longer offered; treat them as invalid
            // so any stale saved display-unit setting falls back to the default unit.
            return false;
    }
}

QString BitcoinUnits::ticker(int unit) {
    switch (unit) {
        case BCH:
            return QString("DVT");
        case mBCH:
            return QString("mDVT");
        case uBCH:
            return QString::fromUtf8("μDVT");
        case SAT:
            return QString("sat");
        default:
            return QString("???");
    }
}

QString BitcoinUnits::description(int unit) {
    constexpr auto thinUtf8 = BitcoinSpaces::thinUtf8;
    switch (unit) {
        case BCH:
            return QObject::tr("DeVault");
        case mBCH:
            return QObject::tr("milli-DeVault") + " (1 / 1" + thinUtf8 + "000)";
        case uBCH:
            return QObject::tr("micro-DeVault/bits") + " (1 / 1" + thinUtf8 + "000" + thinUtf8 + "000)";
        case SAT:
            return QObject::tr("satoshis") + " (1 / 100" + thinUtf8 + "000" + thinUtf8 + "000)";
        default:
            return QString("???");
    }
}

qint64 BitcoinUnits::factor(int unit) {
    switch (unit) {
        case BCH:
            return 100000000;
        case mBCH:
            return 100000;
        case uBCH:
            return 100;
        case SAT:
            return 1;
        default:
            return 100000000;
    }
}

int BitcoinUnits::decimals(int unit) {
    switch (unit) {
        case BCH:
            return 3; // DVT: spock granularity (0.001 DVT) -- never satoshi (8-digit) precision
        case mBCH:
            return 0; // mDVT == 1 spock exactly, so whole numbers only
        case uBCH:
            return 2;
        case SAT:
            return 0;
        default:
            return 0;
    }
}

bool BitcoinUnits::decimalSeparatorIsComma() {
    // Considering that:
    // * bitcoin is an international currency;
    // * Bitcoin-Qt uses only spaces as group separator, as recommended by SI;
    // * Bitcoin-Qt traditionally displays amounts with the dot as decimal separator;
    // * Bitcoin-Qt traditionally accepts both dots and commas as decimal separators in input amounts;
    // * some locales use dots as group separator rather than decimal separator;
    // * some locales have different decimal separators for currency amounts and other numbers;
    // * one cannot retrieve the decimal separator for currency amounts from QLocale;
    // * decimal separators other than dot and comma are rare, especially on computers;
    // this function suggests the dot as decimal separator for displaying amounts,
    // with comma as the fallback if a reader could mistake the dot for a group separator.
    return QString(QLocale().groupSeparator()) == ".";
}

QString BitcoinUnits::format(int unit, const Amount nIn, bool fPlus,
                             SeparatorStyle separators) {
    // Note: not using straight sprintf here because we do NOT want
    // standard localized number formatting.
    if (!valid(unit)) {
        // Refuse to format invalid unit
        return QString();
    }
    qint64 n = qint64(nIn / SATOSHI);
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    QString quotient_str = QString::number(quotient);

    // Use SI-style thin space separators as these are locale independent and
    // can't be confused with the decimal marker.
    int q_size = quotient_str.size();
    if (separators == separatorAlways ||
        (separators == separatorStandard && q_size > 4)) {
        for (int i = 3; i < q_size; i += 3) {
            quotient_str.insert(q_size - i, BitcoinSpaces::thin);
        }
    }

    if (n < 0) {
        quotient_str.insert(0, '-');
    } else if (fPlus && n > 0) {
        quotient_str.insert(0, '+');
    }

    if (num_decimals > 0) {
        qint64 remainder = n_abs % coin;
        // DeVault: show exactly num_decimals fractional digits (3 for DVT = spock granularity), not the
        // full satoshi remainder. `coin` is satoshis-per-unit (a power of ten); scale the remainder down
        // so only the displayed digits remain. Sub-spock satoshis (which valid amounts never carry) are
        // dropped here, matching the CLI's ValueFromAmount and legacy DeVault.
        qint64 scale = coin;
        for (int i = 0; i < num_decimals; ++i) {
            scale /= 10; // coin / 10^num_decimals
        }
        const qint64 frac = scale > 0 ? remainder / scale : 0;
        QString remainder_str =
            QString::number(frac).rightJustified(num_decimals, '0');
        return quotient_str + (decimalSeparatorIsComma() ? ',' : '.') + remainder_str;
    } else {
        return quotient_str;
    }
}

// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString BitcoinUnits::formatWithUnit(int unit, const Amount amount,
                                     bool plussign, SeparatorStyle separators) {
    return format(unit, amount, plussign, separators) + " " + ticker(unit);
}

QString BitcoinUnits::formatHtmlWithUnit(int unit, const Amount amount,
                                         bool plussign,
                                         SeparatorStyle separators) {
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(BitcoinSpaces::thin, QString(BitcoinSpaces::thinHtml));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

std::optional<Amount> BitcoinUnits::parse(int unit, bool allowComma, const QString& value) {
    if (!valid(unit) || value.isEmpty()) {
        // Refuse to parse invalid unit or empty string
        return std::nullopt;
    }
    const int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QString trimmed = removeSpaces(value);
    // If comma is allowed, accept both comma and dot as decimal separators
    if (allowComma) {
        trimmed.replace(',', '.');
    }
    const QStringList parts = trimmed.split('.');

    if (parts.size() > 2) {
        // More than one decimal separator
        return std::nullopt;
    }
    const QString& whole = parts[0];
    QString decimals;

    if (parts.size() > 1) {
        decimals = parts[1];
    }
    if (decimals.size() > num_decimals) {
        // Exceeds max precision
        return std::nullopt;
    }

    const QString str = whole + decimals.leftJustified(num_decimals, '0');
    if (str.size() > 18) {
        // Longer numbers will exceed 63 bits
        return std::nullopt;
    }
    bool ok = false;
    // `str` is the amount in units of 10^-num_decimals of the selected unit -- e.g. for DVT
    // (num_decimals=3) "25.113" -> "25113". Scale up to satoshis: each least-displayed digit is
    // (factor / 10^num_decimals) satoshis (== one spock, 1e5, for both DVT and mDVT).
    const int64_t combined = int64_t(str.toLongLong(&ok));
    if (!ok) {
        // String-to-integer conversion failed
        return std::nullopt;
    }
    int64_t scale = factor(unit);
    for (int i = 0; i < num_decimals; ++i) {
        scale /= 10;
    }
    if (scale != 0 &&
        (combined > std::numeric_limits<int64_t>::max() / scale ||
         combined < std::numeric_limits<int64_t>::min() / scale)) {
        // Would overflow int64 satoshis
        return std::nullopt;
    }
    return (combined * scale) * SATOSHI;
}

QString BitcoinUnits::getAmountColumnTitle(int unit) {
    QString amountTitle = QObject::tr("Amount");
    if (BitcoinUnits::valid(unit)) {
        amountTitle += " (" + BitcoinUnits::ticker(unit) + ")";
    }
    return amountTitle;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitcoinUnits::data(const QModelIndex &index, int role) const {
    int row = index.row();
    if (row >= 0 && row < unitlist.size()) {
        Unit unit = unitlist.at(row);
        switch (role) {
            case Qt::EditRole:
            case Qt::DisplayRole:
                return QVariant(ticker(unit));
            case Qt::ToolTipRole:
                return QVariant(description(unit));
            case UnitRole:
                return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}
