// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpcdnft.h>

#include <chainparams.h>
#include <config.h>
#include <consensus/activation.h>
#include <core_io.h>
#include <devault/dnft_envelope.h>
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
#include <string>
#include <vector>

namespace {

// Require the DU1 (DeVault Upgrade 1) fork to be active for the next block, else DNFT operations
// are meaningless (the token/binding rules are not enforced yet).
void EnsureDU1Active() {
    LOCK(cs_main);
    const auto &params = ::Params().GetConsensus();
    if (!IsDU1Enabled(params, ::ChainActive().Tip())) {
        throw JSONRPCError(RPC_INVALID_REQUEST,
                           "DeVault Upgrade 1 (onchain NFTs) is not active yet on this chain");
    }
}

std::vector<uint8_t> ParseHexField(const UniValue &v, const std::string &name) {
    if (!v.isStr() || (!v.get_str().empty() && !IsHex(v.get_str()))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, name + " must be a hex string");
    }
    return ParseHex(v.get_str());
}

CScript ScriptForAddress(CWallet *pwallet, const UniValue *recipient) {
    if (recipient && !recipient->isNull()) {
        const CTxDestination dest = DecodeDestination(recipient->get_str(), pwallet->chainParams);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recipient address");
        }
        return GetScriptForDestination(dest);
    }
    // default: a fresh wallet address so the wallet owns the item
    CReserveKey reservekey(pwallet);
    CPubKey vchPubKey;
    if (!reservekey.GetReservedKey(vchPubKey, true)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Keypool ran out, call keypoolrefill first");
    }
    reservekey.KeepKey();
    pwallet->LearnRelatedScripts(vchPubKey, OutputType::LEGACY);
    return GetScriptForDestination(vchPubKey.GetID());
}

// Build the DNFT envelope fields from the RPC options.
dnft::EnvelopeFields ParseEnvelopeFields(const std::string &contentType, const UniValue *options) {
    dnft::EnvelopeFields f;
    if (!contentType.empty()) {
        f.content_type = std::vector<uint8_t>(contentType.begin(), contentType.end());
    }
    if (!options || options->isNull()) {
        return f;
    }
    const UniValue::Object &o = options->get_obj();
    if (const UniValue *p = o.locate("parents")) {
        for (const UniValue &pv : p->get_array()) {
            std::vector<uint8_t> parent = ParseHexField(pv, "parent");
            if (parent.size() != dnft::PARENT_VALUE_LENGTH) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "each parent must be 65 bytes (category||commitment) hex");
            }
            f.parents.push_back(std::move(parent));
        }
    }
    if (const UniValue *d = o.locate("delegate")) {
        f.delegate = ParseHexField(*d, "delegate");
    }
    if (const UniValue *m = o.locate("metadata")) {
        f.metadata = ParseHexField(*m, "metadata");
    }
    if (const UniValue *ce = o.locate("content_encoding")) {
        const std::string s = ce->get_str();
        f.content_encoding = std::vector<uint8_t>(s.begin(), s.end());
    }
    return f;
}

// Fund (append inputs keeping vin[0]; change at `changePos`), wallet-sign, and optionally
// broadcast a DNFT transaction. IMPORTANT lock discipline (mirrors the wallet's own RPCs):
// FundTransaction and BroadcastTransaction acquire the chain lock / cs_wallet internally and must
// be called WITHOUT those held; SignTransaction must be called WITH them held. The caller must
// therefore NOT hold the chain lock or cs_wallet when calling this.
CTransactionRef FinalizeMint(const Config &config, CWallet *pwallet, CMutableTransaction &mtx,
                             int changePos, const std::set<int> &subtractFeeFrom, bool broadcast) {
    Amount feeRet;
    std::string err;
    CCoinControl cc; // FundTransaction sets fAllowOtherInputs and pre-selects the existing vins.
    if (!pwallet->FundTransaction(mtx, feeRet, changePos, err, false /* lockUnspents */,
                                  subtractFeeFrom, cc)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to fund DNFT transaction: " + err);
    }

    // Sign with the wallet keystore (reuses the signrawtransactionwithwallet machinery). It signs
    // mtx in place; a fully-signed input has a non-empty scriptSig.
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);
        UniValue nullparam(UniValue::VNULL);
        SignTransaction(pwallet->chain(), mtx, nullparam, pwallet, false, nullparam);
    }
    for (const CTxIn &in : mtx.vin) {
        if (in.scriptSig.empty()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Failed to sign the DNFT transaction (missing keys?)");
        }
    }

    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    if (broadcast) {
        // Throws a detailed JSONRPCError on rejection (e.g. a bad-txns-dnft-* reason).
        BroadcastTransaction(config, tx, false /* allowhighfees */);
    }
    return tx;
}

bool ReadBool(const UniValue *options, const std::string &key, bool def) {
    if (!options || options->isNull()) return def;
    const UniValue *v = options->get_obj().locate(key);
    return v ? v->get_bool() : def;
}

const UniValue *Locate(const UniValue &options, const std::string &key) {
    return options.isObject() ? options.get_obj().locate(key) : nullptr;
}

} // namespace

UniValue mintnft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3) {
        throw std::runtime_error(
            RPCHelpMan{"mintnft",
                "\nMint a new onchain DNFT in a NEW collection (a single-item genesis). The content "
                "is written onchain; ownership is a native token, so the NFT can never be spent as "
                "money by accident.\n",
                {
                    {"content", RPCArg::Type::STR_HEX, false, "", "The NFT content as a hex string"},
                    {"content_type", RPCArg::Type::STR, false, "", "The MIME type of the content (e.g. \"image/png\")"},
                    {"options", RPCArg::Type::OBJ, true, "", "",
                     {
                         {"recipient", RPCArg::Type::STR, true, "", "Address to receive the NFT (default: a new wallet address)"},
                         {"parents", RPCArg::Type::ARR, true, "",
                          "Parent item claims (each must be spent as an input)",
                          {
                              {"parent", RPCArg::Type::STR_HEX, true, "",
                               "65-byte category||commitment, hex"},
                          }},
                         {"delegate", RPCArg::Type::STR_HEX, true, "", "Delegate item id bytes (explorer-resolved)"},
                         {"metadata", RPCArg::Type::STR_HEX, true, "", "CBOR metadata bytes, hex"},
                         {"content_encoding", RPCArg::Type::STR, true, "", "Body content-encoding, e.g. \"br\""},
                         {"broadcast", RPCArg::Type::BOOL, true, "true", "Broadcast the transaction; if false, return the raw hex"},
                     }},
                }}
                .ToString());
    }

    const std::vector<uint8_t> content = ParseHexField(request.params[0], "content");
    const std::string contentType = request.params[1].get_str();
    const UniValue &options = request.params.size() > 2 ? request.params[2] : NullUniValue;
    const bool broadcast = ReadBool(&options, "broadcast", true);

    EnsureDU1Active();

    // A parent claim is only valid if the parent NFT is spent as an input of this mint (§7);
    // mintnft does not yet add/re-emit parent inputs. Build such mints as raw transactions for now
    // (see test/functional/dvt_m9_dnft_binding.py for the shape).
    const dnft::EnvelopeFields fields = ParseEnvelopeFields(contentType, &options);
    if (!fields.parents.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "minting with parent claims is not yet supported by mintnft "
                           "(the parent must be spent as an input); build the mint as a raw transaction");
    }

    // 1. Find the genesis input + build the recipient script (needs the wallet locks); extract the
    // plain values and release before funding/signing/broadcasting (which lock internally).
    COutPoint gcoinOutpoint;
    CScript nftSpk;
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);

        // A confirmed, spendable vout-0 coin becomes the genesis input: its txid becomes the
        // collection category, and (as vin[0]) it also salts the content binding.
        std::vector<COutput> coins;
        pwallet->AvailableCoins(*locked_chain, coins, true /* only safe */);
        const COutput *genesis = nullptr;
        for (const COutput &o : coins) {
            if (o.i == 0 && o.fSpendable && (!o.tx || !o.tx->tx->vout[0].tokenDataPtr)) {
                genesis = &o;
                break;
            }
        }
        if (!genesis) {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
                               "No confirmed non-token vout-0 coin available to start a new "
                               "collection. Receive a payment or mine a block to obtain one, then retry.");
        }
        gcoinOutpoint = genesis->GetInputCoin().outpoint;
        nftSpk = ScriptForAddress(pwallet, Locate(options, "recipient"));
    }
    const token::Id category(gcoinOutpoint.GetTxId());

    // 2. Build the envelope and the salted commitment (token output will be at vout 0).
    const CScript envSpk = dnft::BuildDnftEnvelope(fields, Span<const uint8_t>(content.data(), content.size()));
    const std::array<uint8_t, dnft::COMMITMENT_LENGTH> commitment =
        dnft::ComputeDnftCommitment(envSpk, gcoinOutpoint, 0);

    // 3. Outputs: [0] the inscribed NFT (immutable, no amount), [1] the envelope.
    token::OutputDataPtr td{token::OutputData(category, token::SafeAmount::fromInt(0).value(),
                                              token::NFTCommitment(commitment.begin(), commitment.end()),
                                              /*hasNFT*/ true)};
    CTxOut tokenOut(SATOSHI, nftSpk, td);
    tokenOut.nValue = GetDustThreshold(tokenOut, dustRelayFee); // minimal spendable postage
    CTxOut envOut(Amount::zero(), envSpk);

    CMutableTransaction mtx;
    mtx.vin.emplace_back(gcoinOutpoint, CScript(), std::numeric_limits<uint32_t>::max() - 1);
    mtx.vout.push_back(std::move(tokenOut)); // vout 0
    mtx.vout.push_back(std::move(envOut));   // vout 1

    // 4. Fund (change appended at the end so the token/envelope indices are preserved), sign, send.
    const CTransactionRef tx = FinalizeMint(config, pwallet, mtx, int(mtx.vout.size()), {}, broadcast);

    UniValue::Object result;
    result.emplace_back("txid", tx->GetId().GetHex());
    result.emplace_back("category", category.GetHex());
    result.emplace_back("commitment", HexStr(commitment));
    result.emplace_back("item_id", tx->GetId().GetHex() + "i0");
    if (!broadcast) {
        result.emplace_back("hex", EncodeHexTx(*tx));
    }
    return UniValue(std::move(result));
}

namespace {

// Locate the wallet's UTXO holding the inscribed NFT (category, commitment). Requires cs_wallet.
COutput FindNftCoin(CWallet *pwallet, interfaces::Chain::Lock &locked_chain, const token::Id &category,
                    const std::vector<uint8_t> &commitment) {
    CCoinControl cc;
    cc.m_allow_tokens = true;
    cc.m_tokens_only = true;
    std::vector<COutput> coins;
    pwallet->AvailableCoins(locked_chain, coins, true, &cc);
    for (const COutput &o : coins) {
        const token::OutputDataPtr &ptd = o.GetInputCoin().txout.tokenDataPtr;
        if (ptd && ptd->IsImmutableNFT() && ptd->GetId() == category &&
            ptd->GetCommitment().size() == commitment.size() &&
            std::equal(ptd->GetCommitment().begin(), ptd->GetCommitment().end(), commitment.begin())) {
            return o;
        }
    }
    throw JSONRPCError(RPC_WALLET_ERROR, "No spendable NFT with that category and commitment in the wallet");
}

std::pair<token::Id, std::vector<uint8_t>> ParseNftId(const JSONRPCRequest &request) {
    if (!IsHex(request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "category must be hex");
    }
    const token::Id category(uint256S(request.params[0].get_str()));
    const std::vector<uint8_t> commitment = ParseHexField(request.params[1], "commitment");
    return {category, commitment};
}

} // namespace

UniValue sendnft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 3) {
        throw std::runtime_error(
            RPCHelpMan{"sendnft",
                "\nSend (transfer) an inscribed DNFT to an address. Monetary coins are never touched "
                "except to pay the fee; the NFT's content and identity are preserved.\n",
                {
                    {"category", RPCArg::Type::STR_HEX, false, "", "The NFT's collection category"},
                    {"commitment", RPCArg::Type::STR_HEX, false, "", "The NFT's commitment (its stable item identity)"},
                    {"address", RPCArg::Type::STR, false, "", "The destination DeVault address"},
                }}
                .ToString());
    }

    EnsureDU1Active();

    const auto [category, commitment] = ParseNftId(request);
    const CTxDestination dest = DecodeDestination(request.params[2].get_str(), pwallet->chainParams);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid destination address");
    }

    COutPoint nftOutpoint;
    Amount nftValue;
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);
        const CInputCoin in = FindNftCoin(pwallet, *locked_chain, category, commitment).GetInputCoin();
        nftOutpoint = in.outpoint;
        nftValue = in.txout.nValue;
    }

    // Move: same category+commitment, capability none, no amount, to the new address; preserve the
    // NFT's DVT value. The fee is funded from monetary coins.
    token::OutputDataPtr td{token::OutputData(category, token::SafeAmount::fromInt(0).value(),
                                              token::NFTCommitment(commitment.begin(), commitment.end()),
                                              /*hasNFT*/ true)};
    CMutableTransaction mtx;
    mtx.vin.emplace_back(nftOutpoint, CScript(), std::numeric_limits<uint32_t>::max() - 1);
    mtx.vout.emplace_back(nftValue, GetScriptForDestination(dest), td); // vout 0

    const CTransactionRef tx = FinalizeMint(config, pwallet, mtx, int(mtx.vout.size()), {}, true);

    UniValue::Object result;
    result.emplace_back("txid", tx->GetId().GetHex());
    return UniValue(std::move(result));
}

UniValue burnnft(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 2) {
        throw std::runtime_error(
            RPCHelpMan{"burnnft",
                "\nPermanently destroy an inscribed DNFT the wallet holds (its DVT value, minus fee, "
                "is returned to the wallet). The onchain content remains in the block history.\n",
                {
                    {"category", RPCArg::Type::STR_HEX, false, "", "The NFT's collection category"},
                    {"commitment", RPCArg::Type::STR_HEX, false, "", "The NFT's commitment (its stable item identity)"},
                }}
                .ToString());
    }

    EnsureDU1Active();

    const auto [category, commitment] = ParseNftId(request);
    COutPoint nftOutpoint;
    Amount nftValue;
    CScript refundSpk;
    {
        auto locked_chain = pwallet->chain().lock();
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);
        const CInputCoin in = FindNftCoin(pwallet, *locked_chain, category, commitment).GetInputCoin();
        nftOutpoint = in.outpoint;
        nftValue = in.txout.nValue;
        refundSpk = ScriptForAddress(pwallet, nullptr);
    }

    // Spend the NFT into a plain (no-token) output back to the wallet; the token is not re-created,
    // so it is burned. The NFT's DVT value is rescued in full and the fee is funded from monetary
    // coins (the postage is near dust, so subtracting the fee from it would leave a dust output).
    CMutableTransaction mtx;
    mtx.vin.emplace_back(nftOutpoint, CScript(), std::numeric_limits<uint32_t>::max() - 1);
    mtx.vout.emplace_back(nftValue, refundSpk); // vout 0, no token

    const CTransactionRef tx = FinalizeMint(config, pwallet, mtx, int(mtx.vout.size()), {}, true);

    UniValue::Object result;
    result.emplace_back("txid", tx->GetId().GetHex());
    return UniValue(std::move(result));
}

UniValue listnfts(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            RPCHelpMan{"listnfts",
                "\nList the inscribed DNFTs held by this wallet (by current UTXO). The mint-based "
                "item id and content are provided by the DNFT explorer / -nftindex.\n",
                {}}
                .ToString());
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    CCoinControl cc;
    cc.m_allow_tokens = true;
    cc.m_tokens_only = true;
    std::vector<COutput> coins;
    pwallet->AvailableCoins(*locked_chain, coins, true, &cc);

    UniValue::Array arr;
    for (const COutput &o : coins) {
        const CInputCoin coin = o.GetInputCoin();
        const token::OutputDataPtr &ptd = coin.txout.tokenDataPtr;
        if (!ptd || !ptd->IsImmutableNFT()) {
            continue;
        }
        const auto &commit = ptd->GetCommitment();
        if (commit.empty() || commit[0] != dnft::BINDING_VERSION) {
            continue; // not an inscribed DNFT
        }
        UniValue::Object item;
        item.emplace_back("category", ptd->GetId().GetHex());
        item.emplace_back("commitment", HexStr(commit));
        // Full current outpoint (machine-consumable; COutPoint::ToString truncates the txid)
        item.emplace_back("txid", coin.outpoint.GetTxId().GetHex());
        item.emplace_back("vout", int64_t(coin.outpoint.GetN()));
        item.emplace_back("amount", ValueFromAmount(coin.txout.nValue));
        item.emplace_back("confirmations", o.nDepth);
        arr.emplace_back(std::move(item));
    }
    return UniValue(std::move(arr));
}

UniValue getnftinfo(const Config &config, const JSONRPCRequest &request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet *const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return UniValue();
    }
    if (request.fHelp || request.params.size() != 2) {
        throw std::runtime_error(
            RPCHelpMan{"getnftinfo",
                "\nShow wallet detail for one inscribed DNFT identified by (category, commitment).\n",
                {
                    {"category", RPCArg::Type::STR_HEX, false, "", "The NFT's collection category"},
                    {"commitment", RPCArg::Type::STR_HEX, false, "", "The NFT's commitment"},
                }}
                .ToString());
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    const auto [category, commitment] = ParseNftId(request);
    const COutput o = FindNftCoin(pwallet, *locked_chain, category, commitment);
    const CInputCoin coin = o.GetInputCoin();

    UniValue::Object result;
    result.emplace_back("category", category.GetHex());
    result.emplace_back("commitment", HexStr(commitment));
    // Full current outpoint (machine-consumable; COutPoint::ToString truncates the txid)
    result.emplace_back("txid", coin.outpoint.GetTxId().GetHex());
    result.emplace_back("vout", int64_t(coin.outpoint.GetN()));
    result.emplace_back("amount", ValueFromAmount(coin.txout.nValue));
    result.emplace_back("confirmations", o.nDepth);
    CTxDestination dest;
    if (ExtractDestination(coin.txout.scriptPubKey, dest, 0)) {
        result.emplace_back("address", EncodeDestination(dest, config));
    }
    return UniValue(std::move(result));
}
