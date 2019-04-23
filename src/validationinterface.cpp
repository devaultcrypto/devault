// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "validationinterface.h"
#include "init.h"
#include "scheduler.h"
#include "sync.h"
#include "txmempool.h"
#include "util.h"
#include "validation.h"

#include <atomic>
#include <functional> // for bind
#include <future>
#include <list>
#include <memory>

#include "signals-cpp/signals.h"

struct MainSignalsInstance {
    sigs::signal<void(const CBlockIndex *, const CBlockIndex *,
                                 bool fInitialDownload)>
        UpdatedBlockTip;
    sigs::signal<void(const CTransactionRef &)>
        TransactionAddedToMempool;
    sigs::signal<void(const std::shared_ptr<const CBlock> &,
                                 const CBlockIndex *pindex,
                                 const std::vector<CTransactionRef> &)>
        BlockConnected;
    sigs::signal<void(const std::shared_ptr<const CBlock> &)>
        BlockDisconnected;
    sigs::signal<void(const CTransactionRef &)>
        TransactionRemovedFromMempool;
    sigs::signal<void(const CBlockLocator &)> SetBestChain;
    sigs::signal<void(const uint256 &)> Inventory;
    sigs::signal<void(int64_t nBestBlockTime, CConnman *connman)>
        Broadcast;
    sigs::signal<void(const CBlock &, const CValidationState &)>
        BlockChecked;
    sigs::signal<void(const CBlockIndex *,
                                 const std::shared_ptr<const CBlock> &)>
        NewPoWValidBlock;

    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    SingleThreadedSchedulerClient m_schedulerClient;

    explicit MainSignalsInstance(CScheduler *pscheduler)
        : m_schedulerClient(pscheduler) {}
};

static CMainSignals g_signals;

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler &scheduler) {
    assert(!m_internals);
    m_internals = std::make_unique<MainSignalsInstance>(&scheduler);
}

void CMainSignals::UnregisterBackgroundSignalScheduler() {
    m_internals.reset(nullptr);
}

void CMainSignals::FlushBackgroundCallbacks() {
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CMainSignals::CallbacksPending() {
    if (!m_internals) {
        return 0;
    }
    return m_internals->m_schedulerClient.CallbacksPending();
}

void CMainSignals::RegisterWithMempoolSignals(CTxMemPool &pool) {
    pool.NotifyEntryRemoved.connect(
        std::bind(&CMainSignals::MempoolEntryRemoved, this, std::placeholders::_1, std::placeholders::_2));
}

void CMainSignals::UnregisterWithMempoolSignals(CTxMemPool &pool) {
    pool.NotifyEntryRemoved.disconnect_all(true);
}

CMainSignals &GetMainSignals() {
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface *pwalletIn) {
    g_signals.m_internals->UpdatedBlockTip.connect(std::bind(
        &CValidationInterface::UpdatedBlockTip, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    g_signals.m_internals->TransactionAddedToMempool.connect(std::bind(
        &CValidationInterface::TransactionAddedToMempool, pwalletIn, std::placeholders::_1));
    g_signals.m_internals->BlockConnected.connect(std::bind(
        &CValidationInterface::BlockConnected, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    g_signals.m_internals->BlockDisconnected.connect(
        std::bind(&CValidationInterface::BlockDisconnected, pwalletIn, std::placeholders::_1));
    g_signals.m_internals->TransactionRemovedFromMempool.connect(std::bind(
        &CValidationInterface::TransactionRemovedFromMempool, pwalletIn, std::placeholders::_1));
    g_signals.m_internals->SetBestChain.connect(
        std::bind(&CValidationInterface::SetBestChain, pwalletIn, std::placeholders::_1));
    g_signals.m_internals->Inventory.connect(
        std::bind(&CValidationInterface::Inventory, pwalletIn, std::placeholders::_1));
    g_signals.m_internals->Broadcast.connect(std::bind(
        &CValidationInterface::ResendWalletTransactions, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    g_signals.m_internals->BlockChecked.connect(
        std::bind(&CValidationInterface::BlockChecked, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    g_signals.m_internals->NewPoWValidBlock.connect(std::bind(
        &CValidationInterface::NewPoWValidBlock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
}

void UnregisterValidationInterface(CValidationInterface *pwalletIn) {
    g_signals.m_internals->BlockChecked.disconnect_all(true);
    g_signals.m_internals->Broadcast.disconnect_all(true);
    g_signals.m_internals->Inventory.disconnect_all(true);
    g_signals.m_internals->SetBestChain.disconnect_all(true);
    g_signals.m_internals->TransactionAddedToMempool.disconnect_all(true);
    g_signals.m_internals->BlockConnected.disconnect_all(true);
    g_signals.m_internals->BlockDisconnected.disconnect_all(true);
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect_all(true);
    g_signals.m_internals->UpdatedBlockTip.disconnect_all(true);
    g_signals.m_internals->NewPoWValidBlock.disconnect_all(true);
}

void UnregisterAllValidationInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->BlockChecked.disconnect_all(true);
    g_signals.m_internals->Broadcast.disconnect_all(true);
    g_signals.m_internals->Inventory.disconnect_all(true);
    g_signals.m_internals->SetBestChain.disconnect_all(true);
    g_signals.m_internals->TransactionAddedToMempool.disconnect_all(true);
    g_signals.m_internals->BlockConnected.disconnect_all(true);
    g_signals.m_internals->BlockDisconnected.disconnect_all(true);
    g_signals.m_internals->TransactionRemovedFromMempool.disconnect_all(true);
    g_signals.m_internals->UpdatedBlockTip.disconnect_all(true);
    g_signals.m_internals->NewPoWValidBlock.disconnect_all(true);
}

void CallFunctionInValidationInterfaceQueue(std::function<void()> func) {
    g_signals.m_internals->m_schedulerClient.AddToProcessQueue(std::move(func));
}

void SyncWithValidationInterfaceQueue() {
    AssertLockNotHeld(cs_main);
    // Block until the validation queue drains
    std::promise<void> promise;
    CallFunctionInValidationInterfaceQueue([&promise] { promise.set_value(); });
    promise.get_future().wait();
}

void CMainSignals::MempoolEntryRemoved(CTransactionRef ptx,
                                       MemPoolRemovalReason reason) {
    if (reason != MemPoolRemovalReason::BLOCK &&
        reason != MemPoolRemovalReason::CONFLICT) {
        m_internals->m_schedulerClient.AddToProcessQueue(
            [ptx, this] { m_internals->TransactionRemovedFromMempool.fire(ptx); });
    }
}

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew,
                                   const CBlockIndex *pindexFork,
                                   bool fInitialDownload) {
    m_internals->UpdatedBlockTip.fire(pindexNew, pindexFork, fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const CTransactionRef &ptx) {
    m_internals->TransactionAddedToMempool.fire(ptx);
}

void CMainSignals::BlockConnected(
    const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex,
    const std::vector<CTransactionRef> &vtxConflicted) {
    m_internals->BlockConnected.fire(pblock, pindex, vtxConflicted);
}

void CMainSignals::BlockDisconnected(
    const std::shared_ptr<const CBlock> &pblock) {
    m_internals->BlockDisconnected.fire(pblock);
}

void CMainSignals::SetBestChain(const CBlockLocator &locator) {
    m_internals->SetBestChain.fire(locator);
}

void CMainSignals::Inventory(const uint256 &hash) {
    m_internals->Inventory.fire(hash);
}

void CMainSignals::Broadcast(int64_t nBestBlockTime, CConnman *connman) {
    m_internals->Broadcast.fire(nBestBlockTime, connman);
}

void CMainSignals::BlockChecked(const CBlock &block,
                                const CValidationState &state) {
    m_internals->BlockChecked.fire(block, state);
}

void CMainSignals::NewPoWValidBlock(
    const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    m_internals->NewPoWValidBlock.fire(pindex, block);
}
