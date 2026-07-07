// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <univalue.h>

#include <QString>

#include <optional>
#include <string>

class WalletModel;

/**
 * DeVault 4G: the GUI's single gateway to DNFT functionality. Every DNFT action drives the
 * verified wallet/node RPCs (mintnft, sendnft, burnnft, listnfts, getnftinfo, getnftcollection,
 * getnftitem, gettransaction, ...) through the node's RPC dispatcher — the same path the debug
 * console uses — never a parallel implementation (the 2H lesson).
 */
namespace DnftRpc {

/**
 * Execute an RPC through the dispatcher, scoped to the model's wallet (URI "/wallet/<name>").
 * Returns std::nullopt on error and, if errOut is non-null, fills it with the RPC error message.
 * Must be called from the GUI thread with a valid wallet model.
 */
std::optional<UniValue> call(WalletModel *model, const std::string &method, UniValue::Array params,
                             QString *errOut = nullptr);

} // namespace DnftRpc
