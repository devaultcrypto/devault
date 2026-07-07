// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/dnftrpc.h>

#include <config.h>
#include <interfaces/node.h>
#include <qt/walletmodel.h>

#include <QUrl>

#include <exception>

namespace DnftRpc {

std::optional<UniValue> call(WalletModel *model, const std::string &method, UniValue::Array params,
                             QString *errOut) {
    if (!model) {
        if (errOut) *errOut = QStringLiteral("no wallet");
        return std::nullopt;
    }
    // Scope the request to this wallet, exactly like the debug console does.
    const QByteArray encodedName = QUrl::toPercentEncoding(model->getWalletName());
    const std::string uri = "/wallet/" + std::string(encodedName.constData(), size_t(encodedName.length()));
    try {
        return model->node().executeRpc(::GetMutableConfig(), method, UniValue(std::move(params)), uri);
    } catch (const UniValue &objError) {
        // JSON-RPC errors are thrown as UniValue objects {code, message}.
        if (errOut) {
            const UniValue *msg = objError.locate("message");
            *errOut = msg && msg->isStr() ? QString::fromStdString(msg->get_str())
                                          : QStringLiteral("RPC error");
        }
    } catch (const std::exception &e) {
        if (errOut) *errOut = QString::fromStdString(e.what());
    }
    return std::nullopt;
}

} // namespace DnftRpc
