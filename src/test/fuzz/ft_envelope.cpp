// Copyright (c) 2026 The DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <devault/ft_envelope.h>
#include <script/script.h>
#include <span.h>

#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <vector>

// The DVFT envelope parser is attacker-facing consensus input (from 5C it runs in
// ConnectBlock/ATMP on every deploy/mint marker). This target asserts it never crashes/OOBs on
// arbitrary bytes and that its invariants hold: a valid parse implies the magic detector, exactly
// one role, in-bounds fields, and a lossless round-trip through the builder.

static void ExerciseScript(const CScript &script) {
    const dnft::ParsedFtEnvelope p = dnft::ParseFtEnvelope(script);
    if (!p.valid) {
        assert(!p.error.empty());
        return;
    }

    // A valid parse implies the cheap detector and exactly one role.
    assert(dnft::IsFtEnvelope(script));
    assert(p.is_deploy != p.is_mint);

    if (p.is_mint) {
        // Round-trip: rebuild the canonical mint marker and re-parse.
        const dnft::ParsedFtEnvelope p2 =
            dnft::ParseFtEnvelope(dnft::BuildFtMintEnvelope(p.mint_deploy_txid));
        assert(p2.valid && p2.is_mint);
        assert(p2.mint_deploy_txid == p.mint_deploy_txid);
        return;
    }

    // Deploy invariants: every parsed field is within the codec bounds.
    assert(!p.symbol.empty() && p.symbol.size() <= dnft::FT_MAX_SYMBOL_BYTES);
    assert(!p.name.empty() && p.name.size() <= dnft::FT_MAX_NAME_BYTES);
    assert(p.decimals <= dnft::FT_MAX_DECIMALS);
    assert(p.mode == dnft::FT_MODE_FIXED || p.mode == dnft::FT_MODE_OPEN);
    if (p.mode == dnft::FT_MODE_OPEN) {
        assert(p.quantity_per_mint >= 1 && p.per_block_limit >= 1);
        assert(p.max_mints.has_value() != p.end_height.has_value());
        if (p.end_height) {
            assert(*p.end_height >= p.start_height);
        }
        if (p.max_mints) {
            assert(*p.max_mints >= 1);
        }
    } else {
        // Fixed deploys carry no schedule and no premine.
        assert(p.quantity_per_mint == 0 && p.per_block_limit == 0 && p.premine == 0);
        assert(!p.max_mints && !p.end_height);
    }

    // Round-trip: rebuild from the parsed fields and re-parse -> identical logical result.
    dnft::FtDeployParams f;
    f.symbol = p.symbol;
    f.name = p.name;
    f.decimals = p.decimals;
    f.mode = p.mode;
    f.quantity_per_mint = p.quantity_per_mint;
    f.per_block_limit = p.per_block_limit;
    f.start_height = p.start_height;
    f.max_mints = p.max_mints;
    f.end_height = p.end_height;
    f.premine = p.premine;
    f.metadata = p.metadata;
    const dnft::ParsedFtEnvelope p2 = dnft::ParseFtEnvelope(dnft::BuildFtDeployEnvelope(f));
    assert(p2.valid && p2.is_deploy);
    assert(p2.symbol == p.symbol);
    assert(p2.name == p.name);
    assert(p2.decimals == p.decimals);
    assert(p2.mode == p.mode);
    assert(p2.quantity_per_mint == p.quantity_per_mint);
    assert(p2.per_block_limit == p.per_block_limit);
    assert(p2.start_height == p.start_height);
    assert(p2.max_mints == p.max_mints);
    assert(p2.end_height == p.end_height);
    assert(p2.premine == p.premine);
    assert(p2.metadata == p.metadata);
}

void test_one_input(Span<const uint8_t> buffer) {
    // (a) the raw bytes as a scriptPubKey
    ExerciseScript(CScript(buffer.begin(), buffer.end()));

    // (b) a well-formed OP_RETURN+magic prefix followed by the fuzz bytes, to drive the field
    //     parser deeper (past the magic gate) more often.
    CScript env;
    env << OP_RETURN << std::vector<uint8_t>(dnft::FT_MAGIC.begin(), dnft::FT_MAGIC.end());
    env.insert(env.end(), buffer.begin(), buffer.end());
    ExerciseScript(env);
}
