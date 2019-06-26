// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>
#include <wallet/test/wallet_test_fixture.h>

#include <cstdint>

#define CATCH_CONFIG_MAIN  
#include <catch2/catch.hpp>


static void GetResults(CWallet *wallet,
                       std::map<Amount, CAccountingEntry> &results) {
    std::list<CAccountingEntry> aes;

    results.clear();
    REQUIRE(wallet->ReorderTransactions() == DBErrors::LOAD_OK);
    wallet->ListAccountCreditDebit("", aes);
    for (CAccountingEntry &ae : aes) {
        results[ae.nOrderPos * Amount::min_amount()] = ae;
    }
}

TEST_CASE("acc_orderupgrade") {
  WalletTestingSetup setup;
    std::vector<CWalletTx *> vpwtx;
    CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
    CAccountingEntry ae;
    std::map<Amount, CAccountingEntry> results;

    LOCK(setup.pwalletMain->cs_wallet);

    ae.strAccount = "";
    ae.nCreditDebit = Amount::min_amount();
    ae.nTime = 1333333333;
    ae.strOtherAccount = "b";
    ae.strComment = "";
    setup.pwalletMain->AddAccountingEntry(ae);

    wtx.mapValue["comment"] = "z";
    setup.pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&setup.pwalletMain->mapWallet.at(wtx.GetId()));
    vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
    vpwtx[0]->nOrderPos = -1;

    ae.nTime = 1333333336;
    ae.strOtherAccount = "c";
    setup.pwalletMain->AddAccountingEntry(ae);

    GetResults(setup.pwalletMain.get(), results);

    REQUIRE(setup.pwalletMain->nOrderPosNext == 3);
    REQUIRE(2 == results.size());
    REQUIRE(results[Amount::zero()].nTime == 1333333333);
    REQUIRE(results[Amount::zero()].strComment.empty());
    REQUIRE(1 == vpwtx[0]->nOrderPos);
    REQUIRE(results[2 * Amount::min_amount()].nTime == 1333333336);
    REQUIRE(results[2 * Amount::min_amount()].strOtherAccount == "c");

    ae.nTime = 1333333330;
    ae.strOtherAccount = "d";
    ae.nOrderPos = setup.pwalletMain->IncOrderPosNext();
    setup.pwalletMain->AddAccountingEntry(ae);

    GetResults(setup.pwalletMain.get(), results);

    REQUIRE(results.size() == 3);
    REQUIRE(setup.pwalletMain->nOrderPosNext == 4);
    REQUIRE(results[Amount::zero()].nTime == 1333333333);
    REQUIRE(1 == vpwtx[0]->nOrderPos);
    REQUIRE(results[2 * Amount::min_amount()].nTime == 1333333336);
    REQUIRE(results[3 * Amount::min_amount()].nTime == 1333333330);
    REQUIRE(results[3 * Amount::min_amount()].strComment.empty());

    wtx.mapValue["comment"] = "y";
    {
        CMutableTransaction tx(*wtx.tx);
        // Just to change the hash :)
        --tx.nLockTime;
        wtx.SetTx(MakeTransactionRef(std::move(tx)));
    }
    setup.pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&setup.pwalletMain->mapWallet.at(wtx.GetId()));
    vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

    wtx.mapValue["comment"] = "x";
    {
        CMutableTransaction tx(*wtx.tx);
        // Just to change the hash :)
        --tx.nLockTime;
        wtx.SetTx(MakeTransactionRef(std::move(tx)));
    }
    setup.pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&setup.pwalletMain->mapWallet.at(wtx.GetId()));
    vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
    vpwtx[2]->nOrderPos = -1;

    GetResults(setup.pwalletMain.get(), results);

    REQUIRE(results.size() == 3);
    REQUIRE(setup.pwalletMain->nOrderPosNext == 6);
    REQUIRE(0 == vpwtx[2]->nOrderPos);
    REQUIRE(results[Amount::min_amount()].nTime == 1333333333);
    REQUIRE(2 == vpwtx[0]->nOrderPos);
    REQUIRE(results[3 * Amount::min_amount()].nTime == 1333333336);
    REQUIRE(results[4 * Amount::min_amount()].nTime == 1333333330);
    REQUIRE(results[4 * Amount::min_amount()].strComment.empty());
    REQUIRE(5 == vpwtx[1]->nOrderPos);

    ae.nTime = 1333333334;
    ae.strOtherAccount = "e";
    ae.nOrderPos = -1;
    setup.pwalletMain->AddAccountingEntry(ae);

    GetResults(setup.pwalletMain.get(), results);

    REQUIRE(results.size() == 4);
    REQUIRE(setup.pwalletMain->nOrderPosNext == 7);
    REQUIRE(0 == vpwtx[2]->nOrderPos);
    REQUIRE(results[Amount::min_amount()].nTime == 1333333333);
    REQUIRE(2 == vpwtx[0]->nOrderPos);
    REQUIRE(results[3 * Amount::min_amount()].nTime == 1333333336);
    REQUIRE(results[3 * Amount::min_amount()].strComment.empty());
    REQUIRE(results[4 * Amount::min_amount()].nTime == 1333333330);
    REQUIRE(results[4 * Amount::min_amount()].strComment.empty());
    REQUIRE(results[5 * Amount::min_amount()].nTime == 1333333334);
    REQUIRE(6 == vpwtx[1]->nOrderPos);
}
