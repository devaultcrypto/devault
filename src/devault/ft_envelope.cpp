// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/ft_envelope.h>

#include <crypto/common.h>
#include <devault/envelope_util.h> // NextElement (shared with the DNFT codec)

#include <cstring>

namespace dnft {

namespace {

constexpr const char *ERR_MALFORMED = "bad-txns-ft-envelope-malformed";
constexpr const char *ERR_INCOMPLETE = "bad-txns-ft-envelope-incomplete-field";
constexpr const char *ERR_DUPLICATE = "bad-txns-ft-envelope-duplicate-field";
constexpr const char *ERR_UNKNOWN_EVEN = "bad-txns-ft-envelope-unknown-even-tag";
constexpr const char *ERR_HAS_BODY = "bad-txns-ft-envelope-has-body";
constexpr const char *ERR_ROLE = "bad-txns-ft-envelope-role";
constexpr const char *ERR_MISSING = "bad-txns-ft-envelope-missing-field";
constexpr const char *ERR_SCHEDULE = "bad-txns-ft-envelope-schedule";

//! Fixed-width little-endian integer reads; nullopt on a wrong-width value.
std::optional<uint64_t> ReadU64(Span<const uint8_t> v) {
    if (v.size() != 8) return std::nullopt;
    return ReadLE64(v.data());
}
std::optional<uint32_t> ReadU32(Span<const uint8_t> v) {
    if (v.size() != 4) return std::nullopt;
    return ReadLE32(v.data());
}
std::optional<uint8_t> ReadU8(Span<const uint8_t> v) {
    if (v.size() != 1) return std::nullopt;
    return v[0];
}

ParsedFtEnvelope Reject(const char *reason) {
    ParsedFtEnvelope r;
    r.error = reason;
    return r;
}

} // namespace

bool IsFtEnvelope(const CScript &spk) {
    const uint8_t *pc = spk.data();
    const uint8_t *const end = pc + spk.size();
    if (pc >= end || *pc != OP_RETURN) return false;
    ++pc;
    bool isPush = false;
    Span<const uint8_t> data;
    return NextElement(pc, end, isPush, data) && isPush && data.size() == FT_MAGIC.size() &&
           std::memcmp(data.data(), FT_MAGIC.data(), FT_MAGIC.size()) == 0;
}

ParsedFtEnvelope ParseFtEnvelope(const CScript &spk) {
    // --- OP_RETURN + magic ---
    const uint8_t *pc = spk.data();
    const uint8_t *const end = pc + spk.size();
    if (pc >= end || *pc != OP_RETURN) {
        return Reject("not-a-dvft-envelope");
    }
    ++pc;
    bool isPush = false;
    Span<const uint8_t> data;
    if (!NextElement(pc, end, isPush, data) || !isPush || data.size() != FT_MAGIC.size() ||
        std::memcmp(data.data(), FT_MAGIC.data(), FT_MAGIC.size()) != 0) {
        return Reject("not-a-dvft-envelope");
    }

    // --- collect the remaining pushes; any non-push opcode or truncated push is malformed ---
    std::vector<Span<const uint8_t>> pushes;
    while (pc < end) {
        if (!NextElement(pc, end, isPush, data) || !isPush) {
            return Reject(ERR_MALFORMED);
        }
        pushes.push_back(data);
    }

    // --- no body: an empty push at an even element index (the DNFT body separator) is invalid ---
    for (size_t i = 0; i < pushes.size(); i += 2) {
        if (pushes[i].empty()) {
            return Reject(ERR_HAS_BODY);
        }
    }

    // --- fields come in (tag, value) pairs ---
    if (pushes.size() % 2 != 0) {
        return Reject(ERR_INCOMPLETE);
    }

    ParsedFtEnvelope r;
    bool sawSymbol = false, sawName = false, sawDecimals = false, sawMode = false;
    bool sawQuantity = false, sawPerBlock = false, sawStart = false, sawPremine = false;
    bool sawMetadata = false, sawMint = false;

    for (size_t k = 0; k < pushes.size(); k += 2) {
        const Span<const uint8_t> &tag = pushes[k];
        const Span<const uint8_t> &val = pushes[k + 1];
        if (tag.size() == 1) {
            switch (tag[0]) {
                case FT_TAG_SYMBOL: {
                    if (sawSymbol) return Reject(ERR_DUPLICATE);
                    sawSymbol = true;
                    if (val.empty() || val.size() > FT_MAX_SYMBOL_BYTES) {
                        return Reject("bad-txns-ft-envelope-bad-symbol");
                    }
                    r.symbol.assign(val.begin(), val.end());
                    continue;
                }
                case FT_TAG_NAME: {
                    if (sawName) return Reject(ERR_DUPLICATE);
                    sawName = true;
                    if (val.empty() || val.size() > FT_MAX_NAME_BYTES) {
                        return Reject("bad-txns-ft-envelope-bad-name");
                    }
                    r.name.assign(val.begin(), val.end());
                    continue;
                }
                case FT_TAG_DECIMALS: {
                    if (sawDecimals) return Reject(ERR_DUPLICATE);
                    sawDecimals = true;
                    const auto v = ReadU8(val);
                    if (!v || *v > FT_MAX_DECIMALS) {
                        return Reject("bad-txns-ft-envelope-bad-decimals");
                    }
                    r.decimals = *v;
                    continue;
                }
                case FT_TAG_MODE: {
                    if (sawMode) return Reject(ERR_DUPLICATE);
                    sawMode = true;
                    const auto v = ReadU8(val);
                    if (!v || (*v != FT_MODE_FIXED && *v != FT_MODE_OPEN)) {
                        return Reject("bad-txns-ft-envelope-bad-mode");
                    }
                    r.mode = *v;
                    continue;
                }
                case FT_TAG_QUANTITY: {
                    if (sawQuantity) return Reject(ERR_DUPLICATE);
                    sawQuantity = true;
                    const auto v = ReadU64(val);
                    if (!v || *v == 0) {
                        return Reject("bad-txns-ft-envelope-bad-quantity");
                    }
                    r.quantity_per_mint = *v;
                    continue;
                }
                case FT_TAG_PER_BLOCK_LIMIT: {
                    if (sawPerBlock) return Reject(ERR_DUPLICATE);
                    sawPerBlock = true;
                    const auto v = ReadU64(val);
                    if (!v || *v == 0) {
                        return Reject("bad-txns-ft-envelope-bad-per-block-limit");
                    }
                    r.per_block_limit = *v;
                    continue;
                }
                case FT_TAG_START_HEIGHT: {
                    if (sawStart) return Reject(ERR_DUPLICATE);
                    sawStart = true;
                    const auto v = ReadU32(val);
                    if (!v) {
                        return Reject("bad-txns-ft-envelope-bad-start-height");
                    }
                    r.start_height = *v;
                    continue;
                }
                case FT_TAG_MAX_MINTS: {
                    if (r.max_mints) return Reject(ERR_DUPLICATE);
                    const auto v = ReadU64(val);
                    if (!v || *v == 0) {
                        return Reject("bad-txns-ft-envelope-bad-max-mints");
                    }
                    r.max_mints = *v;
                    continue;
                }
                case FT_TAG_END_HEIGHT: {
                    if (r.end_height) return Reject(ERR_DUPLICATE);
                    const auto v = ReadU32(val);
                    if (!v) {
                        return Reject("bad-txns-ft-envelope-bad-end-height");
                    }
                    r.end_height = *v;
                    continue;
                }
                case FT_TAG_PREMINE: {
                    if (sawPremine) return Reject(ERR_DUPLICATE);
                    sawPremine = true;
                    const auto v = ReadU64(val);
                    if (!v) {
                        return Reject("bad-txns-ft-envelope-bad-premine");
                    }
                    r.premine = *v; // 0 is allowed (== absent)
                    continue;
                }
                case FT_TAG_METADATA: {
                    if (r.metadata) return Reject(ERR_DUPLICATE);
                    if (val.size() != FT_METADATA_POINTER_LENGTH) {
                        return Reject("bad-txns-ft-envelope-bad-metadata");
                    }
                    std::array<uint8_t, FT_METADATA_POINTER_LENGTH> m{};
                    std::memcpy(m.data(), val.data(), m.size());
                    r.metadata = m;
                    continue;
                }
                case FT_TAG_MINT: {
                    if (sawMint) return Reject(ERR_DUPLICATE);
                    sawMint = true;
                    if (val.size() != FT_DEPLOY_TXID_LENGTH) {
                        return Reject("bad-txns-ft-envelope-bad-mint");
                    }
                    std::memcpy(r.mint_deploy_txid.data(), val.data(), r.mint_deploy_txid.size());
                    continue;
                }
                case FT_TAG_NOP:
                    continue; // ignored (duplicates allowed)
                default:
                    break; // fall through to unknown-tag handling
            }
        }
        // Unknown tag: odd first byte -> ignorable; even first byte -> the envelope is invalid
        // (the same forward-compat rule as DNFT). `tag` is non-empty (the even-index empty-push
        // scan above already rejected empties).
        if ((tag[0] & 1u) == 0u) {
            return Reject(ERR_UNKNOWN_EVEN);
        }
    }

    // --- role: exactly one of DEPLOY (mode present) / MINT (tag 23 present) ---
    if (sawMode == sawMint) { // both, or neither
        return Reject(ERR_ROLE);
    }
    const bool anyDeployField = sawSymbol || sawName || sawDecimals || sawQuantity || sawPerBlock ||
                                sawStart || r.max_mints.has_value() || r.end_height.has_value() ||
                                sawPremine || r.metadata.has_value();
    if (sawMint) {
        // A mint marker carries ONLY the deploy txid (spec §8).
        if (anyDeployField) {
            return Reject(ERR_ROLE);
        }
        r.is_mint = true;
        r.valid = true;
        return r;
    }

    // --- deploy: required fields ---
    if (!sawSymbol || !sawName || !sawDecimals) {
        return Reject(ERR_MISSING);
    }
    const bool anyOpenField = sawQuantity || sawPerBlock || sawStart || r.max_mints.has_value() ||
                              r.end_height.has_value() || sawPremine;
    if (r.mode == FT_MODE_OPEN) {
        if (!sawQuantity || !sawPerBlock || !sawStart) {
            return Reject(ERR_MISSING);
        }
        // Exactly one of max_mints / end_height (spec §5.2); with end_height, the window must not
        // be inverted (context-free half of the window rule; start > deploy-height is 5C's).
        if (r.max_mints.has_value() == r.end_height.has_value()) {
            return Reject(ERR_SCHEDULE);
        }
        if (r.end_height && *r.end_height < r.start_height) {
            return Reject(ERR_SCHEDULE);
        }
    } else { // FT_MODE_FIXED
        // The schedule tags (and premine) have no meaning on a fixed deploy: strict absence, so
        // no fixed token can masquerade as mintable.
        if (anyOpenField) {
            return Reject(ERR_SCHEDULE);
        }
    }
    r.is_deploy = true;
    r.valid = true;
    return r;
}

namespace {

void PushTag(CScript &s, uint8_t tag, const std::vector<uint8_t> &value) {
    s << std::vector<uint8_t>{tag} << value;
}

std::vector<uint8_t> U64Bytes(uint64_t v) {
    std::vector<uint8_t> b(8);
    WriteLE64(b.data(), v);
    return b;
}

std::vector<uint8_t> U32Bytes(uint32_t v) {
    std::vector<uint8_t> b(4);
    WriteLE32(b.data(), v);
    return b;
}

} // namespace

CScript BuildFtDeployEnvelope(const FtDeployParams &params) {
    CScript s;
    s << OP_RETURN << std::vector<uint8_t>(FT_MAGIC.begin(), FT_MAGIC.end());
    PushTag(s, FT_TAG_SYMBOL, params.symbol);
    PushTag(s, FT_TAG_NAME, params.name);
    PushTag(s, FT_TAG_DECIMALS, {params.decimals});
    PushTag(s, FT_TAG_MODE, {params.mode});
    if (params.mode == FT_MODE_OPEN) {
        PushTag(s, FT_TAG_QUANTITY, U64Bytes(params.quantity_per_mint));
        PushTag(s, FT_TAG_PER_BLOCK_LIMIT, U64Bytes(params.per_block_limit));
        PushTag(s, FT_TAG_START_HEIGHT, U32Bytes(params.start_height));
        assert(params.max_mints.has_value() != params.end_height.has_value());
        if (params.max_mints) {
            PushTag(s, FT_TAG_MAX_MINTS, U64Bytes(*params.max_mints));
        } else {
            PushTag(s, FT_TAG_END_HEIGHT, U32Bytes(*params.end_height));
        }
        if (params.premine > 0) {
            PushTag(s, FT_TAG_PREMINE, U64Bytes(params.premine));
        }
    }
    if (params.metadata) {
        PushTag(s, FT_TAG_METADATA,
                std::vector<uint8_t>(params.metadata->begin(), params.metadata->end()));
    }
    return s;
}

CScript BuildFtMintEnvelope(const std::array<uint8_t, FT_DEPLOY_TXID_LENGTH> &deployTxid) {
    CScript s;
    s << OP_RETURN << std::vector<uint8_t>(FT_MAGIC.begin(), FT_MAGIC.end());
    PushTag(s, FT_TAG_MINT, std::vector<uint8_t>(deployTxid.begin(), deployTxid.end()));
    return s;
}

} // namespace dnft
