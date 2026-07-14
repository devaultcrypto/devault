// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <wallet/rpcft.h>

#include <chainparams.h>
#include <config.h>
#include <consensus/activation.h>
#include <core_io.h>
#include <devault/ft.h>
#include <devault/ft_envelope.h>
#include <devault/ft_registry.h>
#include <key_io.h>
#include <node/transaction.h>
#include <policy/policy.h>
#include <primitives/token.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/rawtransaction.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/standard.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

/**
 * Default distance from the tip for a deploy's mint-window start, when the caller does not pick one.
 *
 * WHY THIS MATTERS (a consequence of spec O4, `start > deploy_height`): the deployer cannot know the
 * height their deploy will actually be mined at. If the transaction is not confirmed BEFORE its own
 * `start_height`, it becomes permanently invalid and can never confirm — the deploy must simply be
 * rebuilt with a later start. A margin of 6 blocks means the deploy stays valid as long as it
 * confirms within 5 blocks of being broadcast, which is generous in practice. The caller may pick
 * `start_height` explicitly to trade window-opening latency against that safety margin.
 */
constexpr int DEPLOY_START_MARGIN = 6;

void EnsureFtForkActive() {
    LOCK(cs_main);
    const auto &params = ::Params().GetConsensus();
    // The FT rules apply to the block AFTER the tip, so this asks exactly what a new transaction
    // would be validated against.
    if (!IsFTForkEnabledForHeightPrev(params, ::ChainActive().Height())) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           "The DeVault fungible-token system is not active yet on this chain");
    }
}

const UniValue *Locate(const UniValue &options, const std::string &key) {
    if (options.isNull()) {
        return nullptr;
    }
    return options.get_obj().locate(key);
}

bool ReadBool(const UniValue *options, const std::string &key, bool def) {
    if (!options || options->isNull()) {
        return def;
    }
    const UniValue *v = options->get_obj().locate(key);
    return v ? v->get_bool() : def;
}

//! Parse a token amount. Accepted as a JSON number or a decimal string (token amounts can reach
//! int64 max, beyond IEEE-754 exact integers, so a string is the safe form for large values).
int64_t ParseTokenAmount(const UniValue &v, const std::string &name, int64_t minimum = 0) {
    int64_t out = 0;
    if (v.isNum()) {
        out = v.get_int64();
    } else if (v.isStr()) {
        if (!ParseInt64(v.get_str(), &out)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, name + " is not a valid integer");
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, name + " must be an integer (number or string)");
    }
    if (out < minimum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("%s must be at least %d", name, minimum));
    }
    return out;
}

uint64_t ParseU64(const UniValue &v, const std::string &name, uint64_t minimum = 0) {
    const int64_t s = ParseTokenAmount(v, name, 0);
    if (uint64_t(s) < minimum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("%s must be at least %d", name, minimum));
    }
    return uint64_t(s);
}

CScript ScriptForAddress(CWallet *pwallet, const UniValue *recipient) {
    if (recipient && !recipient->isNull()) {
        const CTxDestination dest = DecodeDestination(recipient->get_str(), pwallet->chainParams);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recipient address");
        }
        return GetScriptForDestination(dest);
    }
    CReserveKey reservekey(pwallet);
    CPubKey vchPubKey;
    if (!reservekey.GetReservedKey(vchPubKey, true)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Keypool ran out, call keypoolrefill first");
    }
    reservekey.KeepKey();
    pwallet->LearnRelatedScripts(vchPubKey, OutputType::LEGACY);
    return GetScriptForDestination(vchPubKey.GetID());
}

//! An output carrying `amount` fungible tokens of `category`, with the minimum spendable postage.
CTxOut MakeFtOutput(const token::Id &category, int64_t amount, const CScript &spk) {
    auto safe = token::SafeAmount::fromInt(amount);
    if (!safe) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "token amount out of range");
    }
    token::OutputDataPtr td{token::OutputData(category, *safe)};
    CTxOut out(SATOSHI, spk, td);
    out.nValue = GetDustThreshold(out, dustRelayFee);
    return out;
}

/**
 * Fund (appending inputs and putting change LAST so the token/envelope output indices are
 * preserved), wallet-sign, optionally broadcast.
 *
 * Lock discipline (the 4D lesson): FundTransaction and BroadcastTransaction take the chain lock /
 * cs_wallet INTERNALLY and must be called WITHOUT them held; SignTransaction must be called WITH
 * them held. The caller must hold neither when calling this.
 *
 * `requireVin0` (when set) re-asserts after funding that the transaction's FIRST input is still the
 * one we chose. A DVFT deploy derives its category from `vin[0].prevout` (spec §6.2), so any
 * reordering by the funding/coin-selection path would silently deploy the WRONG category. The
 * wallet does not reorder pre-selected inputs today (and an explicit change position disables BIP69
 * output sorting), but this is consensus-critical enough to verify rather than assume.
 */
CTransactionRef FinalizeFtTx(const Config &config, CWallet *pwallet, CMutableTransaction &mtx,
                             bool broadcast, const COutPoint *requireVin0 = nullptr) {
    Amount feeRet;
    std::string err;
    // A default CCoinControl has m_allow_tokens == false, so the coin selector will NOT pull
    // token-bearing coins in as funding. That is load-bearing: a mint's net creation of its
    // category must be exactly Q, and an accidental token input would silently break it.
    CCoinControl cc;
    int changePos = int(mtx.vout.size()); // append change; keeps our output indices stable (in/out param)
    if (!pwallet->FundTransaction(mtx, feeRet, changePos, err, /*lockUnspents=*/false, {}, cc)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to fund the fungible-token transaction: " + err);
    }

    if (requireVin0 != nullptr && (mtx.vin.empty() || mtx.vin[0].prevout != *requireVin0)) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           "Internal error: funding reordered the transaction inputs; the deploy's "
                           "genesis input must remain vin[0]");
    }

    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);
        UniValue nullparam(UniValue::VNULL);
        SignTransaction(pwallet->chain(), mtx, nullparam, pwallet, false, nullparam);
    }
    for (const CTxIn &in : mtx.vin) {
        if (in.scriptSig.empty()) {
            throw JSONRPCError(RPC_WALLET_ERROR,
                               "Failed to sign the fungible-token transaction (missing keys?)");
        }
    }

    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    if (broadcast) {
        // Throws a detailed JSONRPCError on rejection (e.g. a bad-txns-ft-* reason).
        BroadcastTransaction(config, tx, /*allowhighfees=*/false);
    }
    return tx;
}

//! Resolve a deploy txid argument.
TxId ParseDeployTxid(const UniValue &v) {
    const std::string s = v.get_str();
    if (!IsHex(s) || s.size() != 64) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "deploy_txid must be a 32-byte hex transaction id");
    }
    return TxId(uint256S(s));
}

//! The mint marker's 32-byte payload is the deploy txid in INTERNAL byte order.
std::array<uint8_t, dnft::FT_DEPLOY_TXID_LENGTH> DeployTxidBytes(const TxId &txid) {
    std::array<uint8_t, dnft::FT_DEPLOY_TXID_LENGTH> out{};
    std::memcpy(out.data(), txid.begin(), out.size());
    return out;
}

//! Look up a deploy transaction and parse its DVFT envelope (for the display-only fields the
//! consensus registry deliberately does not store: symbol, name, metadata).
bool FetchDeployEnvelope(const TxId &txid, dnft::ParsedFtEnvelope &out) {
    CTransactionRef tx;
    BlockHash hashBlock;
    if (!GetTransaction(txid, tx, ::Params().GetConsensus(), hashBlock, true)) {
        return false;
    }
    for (const CTxOut &o : tx->vout) {
        if (dnft::IsFtEnvelope(o.scriptPubKey)) {
            out = dnft::ParseFtEnvelope(o.scriptPubKey);
            return out.valid;
        }
    }
    return false;
}

//! Find a confirmed, spendable, non-token coin whose output index is 0. Its txid becomes the
//! deployed category (spec §6.2: X = vin[0].prevout.txid, with vin[0].prevout.n == 0).
COutPoint FindGenesisCoin(CWallet *pwallet, interfaces::Chain::Lock &locked_chain) {
    std::vector<COutput> coins;
    pwallet->AvailableCoins(locked_chain, coins, /*fOnlySafe=*/true);
    for (const COutput &o : coins) {
        if (o.i == 0 && o.fSpendable && (!o.tx || !o.tx->tx->vout[0].tokenDataPtr)) {
            return o.GetInputCoin().outpoint;
        }
    }
    throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
                       "No confirmed non-token vout-0 coin is available to start a new token "
                       "category. Receive a payment (or mine a block) to obtain one, then retry.");
}

//! Collect the wallet's spendable coins carrying fungible tokens, grouped by category.
std::map<token::Id, std::vector<COutput>> FtCoinsByCategory(CWallet *pwallet,
                                                            interfaces::Chain::Lock &locked_chain) {
    CCoinControl cc;
    cc.m_allow_tokens = true;
    cc.m_tokens_only = true;
    std::vector<COutput> coins;
    pwallet->AvailableCoins(locked_chain, coins, /*fOnlySafe=*/true, &cc);

    std::map<token::Id, std::vector<COutput>> out;
    for (const COutput &o : coins) {
        const CTxOut &txout = o.GetInputCoin().txout;
        const token::OutputDataPtr &td = txout.tokenDataPtr;
        // Fungible balances only: an NFT output carries no amount.
        if (!td || !td->HasAmount() || td->HasNFT()) {
            continue;
        }
        out[td->GetId()].push_back(o);
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------- deployft

UniValue deployft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() < 4 || request.params.size() > 5) {
        throw std::runtime_error(
            RPCHelpMan{
                "deployft",
                "\nCreate a new DeVault fungible token (DEVAULT_FT_SPEC.md §5).\n"
                "\nThe token's category is the txid of a confirmed vout-0 coin this wallet spends as "
                "the transaction's FIRST input (a CashTokens genesis).\n"
                "\nmode \"fixed\": the entire supply is created now and can never grow.\n"
                "mode \"open\": anyone may mint `quantity` tokens at a time, at most `per_block_limit` "
                "mints per block, from `start_height` until the cap is reached. Max supply is "
                "premine + max_mints * quantity.\n"
                "\nIMPORTANT (open mode): consensus requires start_height to be STRICTLY GREATER than "
                "the height this deploy is mined at. If the deploy is not confirmed before its own "
                "start_height it becomes permanently invalid and must be rebuilt with a later start. "
                "The default leaves a safety margin of a few blocks.\n",
                {
                    {"symbol", RPCArg::Type::STR, false, "", "Ticker, 1..16 bytes (not unique; the "
                                                             "explorer disambiguates with SYMBOL:nonce)"},
                    {"name", RPCArg::Type::STR, false, "", "Human name, 1..64 bytes"},
                    {"decimals", RPCArg::Type::NUM, false, "", "Display precision, 0..8"},
                    {"mode", RPCArg::Type::STR, false, "", "\"fixed\" or \"open\""},
                    {"options", RPCArg::Type::OBJ, true, "", "",
                     {
                         {"supply", RPCArg::Type::NUM, true, "", "fixed mode: the whole supply (required)"},
                         {"quantity", RPCArg::Type::NUM, true, "", "open mode: tokens per mint (required, >= 1)"},
                         {"per_block_limit", RPCArg::Type::NUM, true, "", "open mode: max mints per block (required, >= 1)"},
                         {"max_mints", RPCArg::Type::NUM, true, "", "open mode: total mints allowed (exclusive with end_height)"},
                         {"end_height", RPCArg::Type::NUM, true, "", "open mode: last height of the window (exclusive with max_mints)"},
                         {"start_height", RPCArg::Type::NUM, true, "", "open mode: first mintable height (default: tip + 6)"},
                         {"premine", RPCArg::Type::NUM, true, "0", "open mode: tokens created for the deployer now"},
                         {"recipient", RPCArg::Type::STR, true, "", "Address to receive the supply/premine (default: a new wallet address)"},
                         {"metadata", RPCArg::Type::STR_HEX, true, "", "36-byte DNFT item id pointing at a rich-metadata artifact"},
                         {"broadcast", RPCArg::Type::BOOL, true, "true", "Broadcast; if false, return the raw hex"},
                     }},
                }}
                .ToString());
    }

    EnsureFtForkActive();

    const std::string symbol = request.params[0].get_str();
    const std::string name = request.params[1].get_str();
    const int64_t decimals = request.params[2].get_int64();
    const std::string modeStr = request.params[3].get_str();
    const UniValue &options = request.params.size() > 4 ? request.params[4] : NullUniValue;

    if (symbol.empty() || symbol.size() > dnft::FT_MAX_SYMBOL_BYTES) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "symbol must be 1..16 bytes");
    }
    if (name.empty() || name.size() > dnft::FT_MAX_NAME_BYTES) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "name must be 1..64 bytes");
    }
    if (decimals < 0 || decimals > dnft::FT_MAX_DECIMALS) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "decimals must be 0..8");
    }
    if (modeStr != "fixed" && modeStr != "open") {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mode must be \"fixed\" or \"open\"");
    }
    const bool isOpen = (modeStr == "open");
    const bool broadcast = ReadBool(&options, "broadcast", true);

    dnft::FtDeployParams p;
    p.symbol = std::vector<uint8_t>(symbol.begin(), symbol.end());
    p.name = std::vector<uint8_t>(name.begin(), name.end());
    p.decimals = uint8_t(decimals);
    p.mode = isOpen ? dnft::FT_MODE_OPEN : dnft::FT_MODE_FIXED;

    const int tipHeight = [] {
        LOCK(cs_main);
        return ::ChainActive().Height();
    }();

    int64_t genesisAmount = 0; // the fungible amount of the new category this tx creates
    if (isOpen) {
        const UniValue *q = Locate(options, "quantity");
        const UniValue *m = Locate(options, "per_block_limit");
        if (!q || !m) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "open mode requires \"quantity\" and \"per_block_limit\"");
        }
        p.quantity_per_mint = ParseU64(*q, "quantity", 1);
        p.per_block_limit = ParseU64(*m, "per_block_limit", 1);

        const UniValue *maxMints = Locate(options, "max_mints");
        const UniValue *endHeight = Locate(options, "end_height");
        if ((maxMints != nullptr) == (endHeight != nullptr)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "open mode requires exactly one of \"max_mints\" or \"end_height\"");
        }

        // start_height: default with a safety margin (see DEPLOY_START_MARGIN). Consensus demands
        // start > deploy_height; a start of tip+1 could only ever be valid if the deploy were mined
        // BELOW the current tip, which is impossible — so reject it here rather than let the user
        // broadcast a transaction that can never confirm.
        int64_t start = int64_t(tipHeight) + DEPLOY_START_MARGIN;
        if (const UniValue *s = Locate(options, "start_height")) {
            start = s->get_int64();
        }
        if (start < int64_t(tipHeight) + 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("start_height must be at least %d (the deploy must confirm "
                                         "strictly before its own start height; the current tip is %d)",
                                         tipHeight + 2, tipHeight));
        }
        if (start > int64_t(std::numeric_limits<uint32_t>::max())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "start_height out of range");
        }
        p.start_height = uint32_t(start);

        if (maxMints) {
            p.max_mints = ParseU64(*maxMints, "max_mints", 1);
        } else {
            const int64_t e = endHeight->get_int64();
            if (e < start || e > int64_t(std::numeric_limits<uint32_t>::max())) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "end_height must be >= start_height and within the 32-bit range");
            }
            p.end_height = uint32_t(e);
        }

        if (const UniValue *pm = Locate(options, "premine")) {
            p.premine = ParseU64(*pm, "premine", 0);
        }
        genesisAmount = int64_t(p.premine);
    } else {
        const UniValue *supply = Locate(options, "supply");
        if (!supply) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "fixed mode requires \"supply\"");
        }
        genesisAmount = ParseTokenAmount(*supply, "supply", 1);
    }

    if (const UniValue *md = Locate(options, "metadata")) {
        const std::vector<uint8_t> bytes = ParseHex(md->get_str());
        if (!IsHex(md->get_str()) || bytes.size() != dnft::FT_METADATA_POINTER_LENGTH) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "metadata must be a 36-byte hex DNFT item id (txid || le32(index))");
        }
        std::array<uint8_t, dnft::FT_METADATA_POINTER_LENGTH> arr{};
        std::memcpy(arr.data(), bytes.data(), arr.size());
        p.metadata = arr;
    }

    // 1. Choose the genesis input + the recipient script (needs the wallet locks); release before
    //    funding/signing/broadcasting, which lock internally.
    COutPoint genesisCoin;
    CScript recipientSpk;
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);
        genesisCoin = FindGenesisCoin(pwallet, *locked_chain);
        recipientSpk = ScriptForAddress(pwallet, Locate(options, "recipient"));
    }
    const token::Id category(genesisCoin.GetTxId());

    // 2. Outputs: [0] the supply/premine (when non-zero), then the DVFT deploy envelope.
    CMutableTransaction mtx;
    mtx.vin.emplace_back(genesisCoin, CScript(), std::numeric_limits<uint32_t>::max() - 1);
    if (genesisAmount > 0) {
        mtx.vout.push_back(MakeFtOutput(category, genesisAmount, recipientSpk));
    }
    mtx.vout.emplace_back(Amount::zero(), dnft::BuildFtDeployEnvelope(p));

    const CTransactionRef tx = FinalizeFtTx(config, pwallet, mtx, broadcast, &genesisCoin);

    UniValue::Object result;
    result.emplace_back("txid", tx->GetId().GetHex());
    result.emplace_back("deploy_txid", tx->GetId().GetHex()); // what mintft references
    result.emplace_back("category", category.GetHex());
    result.emplace_back("symbol", symbol);
    result.emplace_back("name", name);
    result.emplace_back("decimals", decimals);
    result.emplace_back("mode", modeStr);
    if (isOpen) {
        result.emplace_back("quantity", int64_t(p.quantity_per_mint));
        result.emplace_back("per_block_limit", int64_t(p.per_block_limit));
        result.emplace_back("start_height", int64_t(p.start_height));
        if (p.max_mints) {
            result.emplace_back("max_mints", int64_t(*p.max_mints));
        } else {
            result.emplace_back("end_height", int64_t(*p.end_height));
        }
        result.emplace_back("premine", int64_t(p.premine));
    } else {
        result.emplace_back("supply", genesisAmount);
    }
    if (!broadcast) {
        result.emplace_back("hex", EncodeHexTx(*tx));
    }
    return UniValue(std::move(result));
}

// ---------------------------------------------------------------- mintft

UniValue mintft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.empty() || request.params.size() > 3) {
        throw std::runtime_error(
            RPCHelpMan{
                "mintft",
                "\nMint from an open-mint fungible token (DEVAULT_FT_SPEC.md §6.3).\n"
                "\nEach mint creates exactly the deploy's `quantity` tokens. Minting is permissionless: "
                "any wallet may mint while the emission window is open and the block's per-deploy "
                "allowance is not exhausted.\n"
                "\nOne mint per transaction (spec Q4); `count` simply builds and broadcasts that many "
                "transactions. Note that a block accepts at most `per_block_limit` mints of a given "
                "deploy — surplus mints stay in the mempool and confirm in later blocks.\n",
                {
                    {"deploy_txid", RPCArg::Type::STR_HEX, false, "", "The txid of the deploy transaction"},
                    {"count", RPCArg::Type::NUM, true, "1", "How many mint transactions to build"},
                    {"address", RPCArg::Type::STR, true, "", "Address to receive the tokens (default: a new wallet address)"},
                }}
                .ToString());
    }

    EnsureFtForkActive();

    const TxId deployTxid = ParseDeployTxid(request.params[0]);
    const int64_t count = request.params.size() > 1 && !request.params[1].isNull()
                              ? request.params[1].get_int64()
                              : 1;
    if (count < 1 || count > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "count must be between 1 and 100");
    }

    // Resolve the deploy in the consensus registry and pre-check the emission window, so the caller
    // gets an actionable error instead of a raw consensus rejection.
    token::Id category;
    int64_t quantity = 0;
    uint64_t allowanceNextBlock = 0;
    {
        LOCK(cs_main);
        if (!g_ftRegistry) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "The fungible-token deploy registry is unavailable");
        }
        const FtDeployRecord *rec = g_ftRegistry->Lookup(deployTxid);
        if (rec == nullptr) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               "No open-mint deploy is registered for that transaction id. (A "
                               "fixed-supply token can never be minted, and a deploy is only "
                               "registered once it has confirmed.)");
        }
        const int nextHeight = ::ChainActive().Height() + 1;
        allowanceNextBlock = dnft::FtScheduleLimit(*rec, nextHeight);
        if (allowanceNextBlock == 0) {
            throw JSONRPCError(RPC_INVALID_REQUEST,
                               strprintf("The emission window for this deploy is not open at height "
                                         "%d (it starts at %d and closes once all %d mints are used)",
                                         nextHeight, rec->startHeight, rec->maxMints));
        }
        category = rec->category;
        quantity = int64_t(rec->quantity);
    }

    const UniValue *addr = request.params.size() > 2 ? &request.params[2] : nullptr;
    const auto markerBytes = DeployTxidBytes(deployTxid);
    const CScript marker = dnft::BuildFtMintEnvelope(markerBytes);

    UniValue::Array txids;
    for (int64_t i = 0; i < count; ++i) {
        CScript spk;
        {
            auto locked_chain = pwallet->chain().lock();
            LOCK(pwallet->cs_wallet);
            EnsureWalletIsUnlocked(pwallet);
            spk = ScriptForAddress(pwallet, addr);
        }
        // Outputs: [0] exactly `quantity` new tokens, [1] the DVFT mint marker naming the deploy.
        // Funding inputs are chosen with a default CCoinControl, which never selects token-bearing
        // coins — so the transaction's NET creation of this category is exactly `quantity`.
        CMutableTransaction mtx;
        mtx.vout.push_back(MakeFtOutput(category, quantity, spk));
        mtx.vout.emplace_back(Amount::zero(), marker);
        const CTransactionRef tx = FinalizeFtTx(config, pwallet, mtx, /*broadcast=*/true);
        txids.emplace_back(tx->GetId().GetHex());
    }

    UniValue::Object result;
    result.emplace_back("deploy_txid", deployTxid.GetHex());
    result.emplace_back("category", category.GetHex());
    result.emplace_back("quantity_each", quantity);
    result.emplace_back("minted", int64_t(count));
    result.emplace_back("allowance_next_block", int64_t(allowanceNextBlock));
    result.emplace_back("txids", std::move(txids));
    return UniValue(std::move(result));
}

// ---------------------------------------------------------------- sendft

UniValue sendft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 3) {
        throw std::runtime_error(
            RPCHelpMan{
                "sendft",
                "\nSend fungible tokens of a category to an address.\n"
                "\nThe amount is in BASE UNITS (the raw onchain integer). `decimals` is display-only "
                "metadata and is NOT applied here.\n",
                {
                    {"category", RPCArg::Type::STR_HEX, false, "", "The token category"},
                    {"amount", RPCArg::Type::NUM, false, "", "Amount in base units"},
                    {"address", RPCArg::Type::STR, false, "", "Destination address"},
                }}
                .ToString());
    }

    EnsureFtForkActive();

    const std::string catStr = request.params[0].get_str();
    if (!IsHex(catStr) || catStr.size() != 64) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "category must be a 32-byte hex id");
    }
    const token::Id category{uint256S(catStr)};
    const int64_t amount = ParseTokenAmount(request.params[1], "amount", 1);

    const CTxDestination dest = DecodeDestination(request.params[2].get_str(), pwallet->chainParams);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid destination address");
    }
    const CScript destSpk = GetScriptForDestination(dest);

    CMutableTransaction mtx;
    int64_t selected = 0;
    Amount tokenInValue = Amount::zero(); // the DVT postage riding on the token coins we spend
    CScript changeSpk;
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);

        const auto byCat = FtCoinsByCategory(pwallet, *locked_chain);
        auto it = byCat.find(category);
        if (it == byCat.end()) {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
                               "This wallet holds no tokens of that category");
        }
        // Spend token coins until the requested amount is covered.
        for (const COutput &o : it->second) {
            const CInputCoin coin = o.GetInputCoin();
            mtx.vin.emplace_back(coin.outpoint, CScript(), std::numeric_limits<uint32_t>::max() - 1);
            selected += coin.txout.tokenDataPtr->GetAmount().getint64();
            tokenInValue += coin.txout.nValue;
            if (selected >= amount) {
                break;
            }
        }
        if (selected < amount) {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
                               strprintf("Insufficient token balance: have %d, need %d", selected,
                                         amount));
        }
        changeSpk = ScriptForAddress(pwallet, nullptr);
    }

    CTxOut destOut = MakeFtOutput(category, amount, destSpk);
    std::optional<CTxOut> changeOut;
    if (selected > amount) {
        // Token change back to this wallet (CashTokens conservation would otherwise burn it).
        changeOut = MakeFtOutput(category, selected - amount, changeSpk);
    }

    // Absorb the spent coins' DVT postage into OUR OWN outputs.
    //
    // Every token UTXO carries the dust minimum (~0.6 DVT) as postage. Spending several of them
    // brings in more DVT than the one or two token outputs need, and that leftover would come back
    // as a tiny DVT change output — below the dust floor, making the whole transaction non-standard
    // ("dust"). Pushing the excess into the token change (or, when the amount matches exactly, into
    // the destination — the same value-preserving behaviour as sendnft) guarantees the token side of
    // the balance is self-funding, so the coin selector only ever has to cover the FEE from a
    // monetary coin, which always leaves a healthy (non-dust) change.
    const Amount absorbed = destOut.nValue + (changeOut ? changeOut->nValue : Amount::zero());
    if (tokenInValue > absorbed) {
        const Amount excess = tokenInValue - absorbed;
        if (changeOut) {
            changeOut->nValue += excess;
        } else {
            destOut.nValue += excess;
        }
    }

    mtx.vout.push_back(std::move(destOut));
    if (changeOut) {
        mtx.vout.push_back(std::move(*changeOut));
    }

    const CTransactionRef tx = FinalizeFtTx(config, pwallet, mtx, /*broadcast=*/true);

    UniValue::Object result;
    result.emplace_back("txid", tx->GetId().GetHex());
    result.emplace_back("category", category.GetHex());
    result.emplace_back("amount", amount);
    result.emplace_back("token_change", selected - amount);
    return UniValue(std::move(result));
}

// ---------------------------------------------------------------- getftinfo

UniValue getftinfo(const Config &config, const JSONRPCRequest &request) {
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{
                "getftinfo",
                "\nShow what the chain knows about a fungible-token deploy.\n"
                "\nThe consensus deploy registry holds only OPEN-mint deploys (a fixed-supply token "
                "can never be minted, so nothing ever needs to look it up). Display fields "
                "(symbol/name/metadata) come from the deploy transaction itself, so they require the "
                "transaction to be retrievable (-txindex, or a recent/mempool transaction).\n"
                "\nCirculating supply and mints-so-far are NOT reported here: they are derived data "
                "and arrive with the optional -ftindex.\n",
                {
                    {"deploy_txid", RPCArg::Type::STR_HEX, false, "", "The txid of the deploy transaction"},
                }}
                .ToString());
    }

    const TxId deployTxid = ParseDeployTxid(request.params[0]);

    UniValue::Object result;
    result.emplace_back("deploy_txid", deployTxid.GetHex());

    // The authoritative consensus view.
    FtDeployRecord rec;
    bool registered = false;
    int nextHeight = 0;
    {
        LOCK(cs_main);
        nextHeight = ::ChainActive().Height() + 1;
        if (g_ftRegistry) {
            if (const FtDeployRecord *r = g_ftRegistry->Lookup(deployTxid)) {
                rec = *r;
                registered = true;
            }
        }
    }

    // The display-only fields the registry deliberately does not store.
    dnft::ParsedFtEnvelope env;
    const bool haveTx = FetchDeployEnvelope(deployTxid, env);
    if (haveTx && env.is_deploy) {
        result.emplace_back("symbol", std::string(env.symbol.begin(), env.symbol.end()));
        result.emplace_back("name", std::string(env.name.begin(), env.name.end()));
        result.emplace_back("decimals", int64_t(env.decimals));
        if (env.metadata) {
            result.emplace_back("metadata", HexStr(*env.metadata));
        }
    }

    if (registered) {
        // maxSupply = premine + N*Q; the deploy-time check guarantees this fits int64.
        const __int128 maxSupply =
            __int128(rec.premine) + __int128(rec.maxMints) * __int128(rec.quantity);
        const uint64_t allowance = dnft::FtScheduleLimit(rec, nextHeight);
        result.emplace_back("mode", "open");
        result.emplace_back("category", uint256(rec.category).GetHex());
        result.emplace_back("quantity", int64_t(rec.quantity));
        result.emplace_back("per_block_limit", int64_t(rec.perBlockLimit));
        result.emplace_back("start_height", int64_t(rec.startHeight));
        result.emplace_back("max_mints", int64_t(rec.maxMints));
        result.emplace_back("premine", int64_t(rec.premine));
        result.emplace_back("deploy_height", int64_t(rec.deployHeight));
        result.emplace_back("max_supply", int64_t(maxSupply));
        result.emplace_back("window_open", allowance > 0);
        result.emplace_back("allowance_next_block", int64_t(allowance));
        result.emplace_back("next_height", int64_t(nextHeight));
    } else if (haveTx && env.is_deploy && env.mode == dnft::FT_MODE_FIXED) {
        result.emplace_back("mode", "fixed");
        result.emplace_back("note",
                            "Fixed-supply tokens are not registered (they can never be minted); the "
                            "supply is whatever the deploy transaction created.");
    } else if (!haveTx) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "No open-mint deploy is registered for that transaction id, and the "
                           "transaction could not be retrieved (try -txindex).");
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           "That transaction does not carry a valid DVFT deploy envelope");
    }
    return UniValue(std::move(result));
}

// ---------------------------------------------------------------- listfttokens / getftbalance

UniValue listfttokens(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || !request.params.empty()) {
        throw std::runtime_error(
            RPCHelpMan{"listfttokens",
                       "\nList this wallet's fungible-token balances, one entry per category. "
                       "Amounts are in base units (decimals are display-only).\n",
                       {}}
                .ToString());
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    UniValue::Array arr;
    for (const auto &[category, coins] : FtCoinsByCategory(pwallet, *locked_chain)) {
        int64_t total = 0;
        for (const COutput &o : coins) {
            total += o.GetInputCoin().txout.tokenDataPtr->GetAmount().getint64();
        }
        UniValue::Object entry;
        entry.emplace_back("category", uint256(category).GetHex());
        entry.emplace_back("amount", total);
        entry.emplace_back("utxos", int64_t(coins.size()));
        arr.emplace_back(std::move(entry));
    }
    return UniValue(std::move(arr));
}

UniValue getftbalance(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{"getftbalance",
                       "\nThis wallet's spendable balance of one token category, in base units.\n",
                       {
                           {"category", RPCArg::Type::STR_HEX, false, "", "The token category"},
                       }}
                .ToString());
    }

    const std::string catStr = request.params[0].get_str();
    if (!IsHex(catStr) || catStr.size() != 64) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "category must be a 32-byte hex id");
    }
    const token::Id category{uint256S(catStr)};

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    int64_t total = 0;
    const auto byCat = FtCoinsByCategory(pwallet, *locked_chain);
    if (auto it = byCat.find(category); it != byCat.end()) {
        for (const COutput &o : it->second) {
            total += o.GetInputCoin().txout.tokenDataPtr->GetAmount().getint64();
        }
    }
    return UniValue(total);
}
