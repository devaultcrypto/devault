// Copyright (c) 2019 The DeVault developers
// Copyright (c) 2019 Jon Spock
// Copyright (c) 2026 The DeVault developers (V2 port)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <devault/budget.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <key_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/standard.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

// Group Address with % for safety/clarity
struct BudgetStruct {
    std::string MainNetAddress;
    std::string TestNetAddress;
    std::string RegTestNetAddress;
    std::string Purpose;
    int64_t percent;
};

struct BudgetPayouts {
    int32_t changeSuperBlock; // Where to switch between old DAO addresses and new DAO addresses
    std::vector<BudgetStruct> budget;
};

// Reward Address & Percentage (verbatim from legacy DeVault devault/budget.cpp:30-55).
// The RegTestNetAddress column is V2-only (legacy had none — its regtest superblocks were broken
// because the dvtest: strings do not decode under the dvreg: prefix, leaving empty payout scripts).
// Each regtest address is the SAME hash160 payload as the testnet address, re-encoded with the
// dvreg: prefix, so regtest superblocks are mineable/validatable in tests. Mainnet/testnet payouts
// are untouched (guarded by budget_tests).
const BudgetPayouts Payouts[] = {
    {
        5, // change on 5th Superblock
        {{"devault:pp2ghv9ya7fs98rvz3gzuqmen608dh6g2y5d5dxrtp", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "dvreg:pr2jg83w445mrwqwkczgdevyeetatckhg5le2mty0u", "Community", 15},
         {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "dvreg:pr49tdjhuktpp440edg9cej7d9hcvnrluvpxqst25f", "CoreDevs", 10},
         {"devault:pqws2sgc2y22x2gkcnmw72edpa0u0kscdsqp29e530", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "dvreg:qzlg7mlrz56hnnwddc8k0a2w397sqjgegghhpljds4", "WebDevs", 5},
         {"devault:pzgux6zlzpw45hm45fwcj5d7mf5fn7pa2ydjzx5nxw", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "dvreg:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9lu606sqezg", "BusDevs", 5},
         {"devault:prutq74qks5aez2a4mhrcm26t0r3pjzpxyapq0kdjk", "dvtest:qqltstfypfuftlgrvqdfml4js0y3yxdpgya2krkhk5", "dvreg:qqltstfypfuftlgrvqdfml4js0y3yxdpgy3s85sf8s", "Marketing", 5},
         {"devault:prcljsfamr0hsc2jn4mr5et9xx5u9lm8rvl976n0zm", "dvtest:qpw43t63nxmufurn5qn9jyurc27xntvdz5n7ujhdaq", "dvreg:qpw43t63nxmufurn5qn9jyurc27xntvdz5lyd93nvy", "Support", 5}},
    },
    {
        15,
        {{"devault:pp2ghv9ya7fs98rvz3gzuqmen608dh6g2y5d5dxrtp", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "dvreg:pr2jg83w445mrwqwkczgdevyeetatckhg5le2mty0u", "Community", 15},
         {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "dvreg:pr49tdjhuktpp440edg9cej7d9hcvnrluvpxqst25f", "CoreDevs", 10},
         {"devault:pqws2sgc2y22x2gkcnmw72edpa0u0kscdsqp29e530", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "dvreg:qzlg7mlrz56hnnwddc8k0a2w397sqjgegghhpljds4", "WebDevs/Support", 10},
         {"devault:pzgux6zlzpw45hm45fwcj5d7mf5fn7pa2ydjzx5nxw", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "dvreg:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9lu606sqezg", "BusDev/Marketing", 10}},
    },
    {
        std::numeric_limits<int32_t>::max(), // change if we add more to superblock change
        {{"devault:pqqqaf843zj992fkqr483zptyp0r8kfg7g5enjt05d", "dvtest:pr2jg83w445mrwqwkczgdevyeetatckhg5nrmvd67c", "dvreg:pr2jg83w445mrwqwkczgdevyeetatckhg5le2mty0u", "Community", 15},
         {"devault:prg2wlmzj7kzy8ps7pfnkf39nze49yh8fsk0yfslw0", "dvtest:pr49tdjhuktpp440edg9cej7d9hcvnrluvdu38d59d", "dvreg:pr49tdjhuktpp440edg9cej7d9hcvnrluvpxqst25f", "CoreDevs", 10},
         {"devault:prgtl5yust3v2m76c3t4vsnuwuufaht0euqm2smja2", "dvtest:qzlg7mlrz56hnnwddc8k0a2w397sqjgeggmdsg5np3", "dvreg:qzlg7mlrz56hnnwddc8k0a2w397sqjgegghhpljds4", "WebDevs/Support", 10},
         {"devault:pz3htku32554kntjvzpwp8nuhmksvp73f5wwmxts8h", "dvtest:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9luk4t8x8nv", "dvreg:qpzn5j40a3r0kaznf8y6jpaa4z7stmg9lu606sqezg", "BusDev/Marketing", 10}},
    }};

// Get Array Size at Compile time for Loops
const int ChangeSize = sizeof(Payouts) / sizeof(Payouts[0]);

int getPayoutIndexFromHeight(int SuperBlockNumber) {
    int i = 0;
    for (i = 0; i < ChangeSize; i++) {
        if (Payouts[i].changeSuperBlock > SuperBlockNumber) {
            break;
        }
    }
    return i;
}

} // namespace

// Compute the (script, amount) budget payouts for a superblock at nHeight; empty if nHeight is NOT a
// superblock. The single source of truth shared by CheckSuperBlockBudget (validate) and
// FillSuperBlockBudget (mine), so the two can never disagree. CalculateSuperBlockRewards ported
// verbatim: DeVault's `.toInt()` is BCHN's `/ SATOSHI`; trailing `* COIN` converts the integer coin
// amount back to an Amount.
namespace {
std::vector<std::pair<CScript, Amount>>
ComputeSuperBlockPayments(int nHeight, const Amount &nBlockSubsidy, const CChainParams &chainparams) {
    std::vector<std::pair<CScript, Amount>> payments;
    const Consensus::Params &consensus = chainparams.GetConsensus();
    if (!consensus.IsSuperBlock(nHeight)) {
        return payments;
    }
    const int64_t nBlocksPerPeriod = consensus.nBlocksPerYear / 12;
    const std::string &netID = chainparams.NetworkIDString();
    const bool fRegTest = (netID == "regtest");
    const bool fTestNet = (netID != "main");
    const int Index = getPayoutIndexFromHeight(int(nHeight / nBlocksPerPeriod));

    // Sum of the %s -> scale factor (DeVault budget is a fraction of the miner's portion).
    int PerCentSum = 0;
    for (const auto &p : Payouts[Index].budget) {
        PerCentSum += p.percent;
    }
    const int ScaleFactor = (100 - PerCentSum);

    for (const auto &b : Payouts[Index].budget) {
        const std::string &addr =
            fRegTest ? b.RegTestNetAddress : (fTestNet ? b.TestNetAddress : b.MainNetAddress);
        CScript script = GetScriptForDestination(DecodeDestination(addr, chainparams));
        Amount payment =
            (((b.percent * nBlocksPerPeriod * (nBlockSubsidy / SATOSHI)) / (ScaleFactor * (COIN / SATOSHI))) *
             COIN);
        payments.emplace_back(std::move(script), payment);
    }
    return payments;
}
} // namespace

bool CheckSuperBlockBudget(const CBlock &block, int nHeight, const Amount &nBlockSubsidy,
                           const CChainParams &chainparams, Amount &nBudgetReward) {
    const auto payments = ComputeSuperBlockPayments(nHeight, nBlockSubsidy, chainparams);
    if (payments.empty()) { // not a superblock
        nBudgetReward = Amount::zero();
        return true;
    }

    Amount refRewards = Amount::zero();
    for (const auto &[script, payment] : payments) {
        refRewards += payment;
    }

    // Verify the coinbase pays each budget address exactly its expected amount (CBudget::Validate).
    bool fPaymentOK = true;
    Amount nSumReward = Amount::zero();
    for (const auto &out : block.vtx[0]->vout) {
        for (const auto &[script, payment] : payments) {
            if (out.scriptPubKey == script) {
                if (out.nValue != payment) {
                    fPaymentOK = false;
                } else {
                    nSumReward += payment;
                }
            }
        }
    }
    if (nSumReward != refRewards) {
        fPaymentOK = false;
    }
    nBudgetReward = nSumReward;
    return fPaymentOK;
}

bool FillSuperBlockBudget(CMutableTransaction &txNew, int nHeight, const Amount &nBlockSubsidy,
                          const CChainParams &chainparams) {
    const auto payments = ComputeSuperBlockPayments(nHeight, nBlockSubsidy, chainparams);
    if (payments.empty()) {
        return false; // not a superblock -> caller may add a cold reward instead
    }
    for (const auto &[script, payment] : payments) {
        txNew.vout.emplace_back(payment, script);
    }
    return true;
}
