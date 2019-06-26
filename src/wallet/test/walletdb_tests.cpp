// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <wallet/wallet.h>

#include <test/test_bitcoin.h>
#include <wallet/test/wallet_test_fixture.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

namespace {
static std::unique_ptr<CWalletDBWrapper> TmpDB(const fs::path &pathTemp,
                                               const std::string &testname) {
    fs::path dir = pathTemp / testname;
    // fs::create_directory(dir);
    // FIXTHIS    REQUIRE(fs::create_directory(dir),  "Unable to create a
    // directory for test " + testname);
    fs::path path =
        dir / strprintf("testwallet%i", static_cast<int>(GetRand(1000000)));
    return std::unique_ptr<CWalletDBWrapper>(
        new CWalletDBWrapper(&bitdb, path.string()));
}

static std::unique_ptr<CWallet> LoadWallet(CWalletDB *db) {
    std::unique_ptr<CWallet> wallet(new CWallet(Params()));
    DBErrors res = db->LoadWallet(wallet.get());
    REQUIRE(res == DBErrors::LOAD_OK);
    return wallet;
}
} // namespace

TEST_CASE("write_erase_name") {
    WalletTestingSetup setup;
    auto walletdbwrapper = TmpDB(setup.pathTemp, "write_erase_name");
    CWalletDB walletdb(*walletdbwrapper, "cr+");

    CTxDestination dst1 = CKeyID(uint160S("c0ffee"));
    CTxDestination dst2 = CKeyID(uint160S("f00d"));

    REQUIRE(walletdb.WriteName(dst1, "name1"));
    REQUIRE(walletdb.WriteName(dst2, "name2"));
    {
        auto w = LoadWallet(&walletdb);
        REQUIRE(1 == w->mapAddressBook.count(dst1));
        REQUIRE("name1" == w->mapAddressBook[dst1].name);
        REQUIRE("name2" == w->mapAddressBook[dst2].name);
    }

    walletdb.EraseName(dst1);

    {
        auto w = LoadWallet(&walletdb);
        REQUIRE(0 == w->mapAddressBook.count(dst1));
        REQUIRE(1 == w->mapAddressBook.count(dst2));
    }
}

TEST_CASE("write_erase_purpose") {
    WalletTestingSetup setup;
    auto walletdbwrapper = TmpDB(setup.pathTemp, "write_erase_purpose");
    CWalletDB walletdb(*walletdbwrapper, "cr+");

    CTxDestination dst1 = CKeyID(uint160S("c0ffee"));
    CTxDestination dst2 = CKeyID(uint160S("f00d"));

    REQUIRE(walletdb.WritePurpose(dst1, "purpose1"));
    REQUIRE(walletdb.WritePurpose(dst2, "purpose2"));
    {
        auto w = LoadWallet(&walletdb);
        REQUIRE(1 == w->mapAddressBook.count(dst1));
        REQUIRE("purpose1" == w->mapAddressBook[dst1].purpose);
        REQUIRE("purpose2" == w->mapAddressBook[dst2].purpose);
    }

    walletdb.ErasePurpose(dst1);

    {
        auto w = LoadWallet(&walletdb);
        REQUIRE(0 == w->mapAddressBook.count(dst1));
        REQUIRE(1 == w->mapAddressBook.count(dst2));
    }
}

TEST_CASE("write_erase_destdata") {
    WalletTestingSetup setup;
    auto walletdbwrapper = TmpDB(setup.pathTemp, "write_erase_destdata");
    CWalletDB walletdb(*walletdbwrapper, "cr+");

    CTxDestination dst1 = CKeyID(uint160S("c0ffee"));
    CTxDestination dst2 = CKeyID(uint160S("f00d"));

    REQUIRE(walletdb.WriteDestData(dst1, "key1", "value1"));
    REQUIRE(walletdb.WriteDestData(dst1, "key2", "value2"));
    REQUIRE(walletdb.WriteDestData(dst2, "key1", "value3"));
    REQUIRE(walletdb.WriteDestData(dst2, "key2", "value4"));
    {
        auto w = LoadWallet(&walletdb);
        std::string val;
        REQUIRE(w->GetDestData(dst1, "key1", &val));
        REQUIRE("value1" == val);
        REQUIRE(w->GetDestData(dst1, "key2", &val));
        REQUIRE("value2" == val);
        REQUIRE(w->GetDestData(dst2, "key1", &val));
        REQUIRE("value3" == val);
        REQUIRE(w->GetDestData(dst2, "key2", &val));
        REQUIRE("value4" == val);
    }

    walletdb.EraseDestData(dst1, "key2");

    {
        auto w = LoadWallet(&walletdb);
        std::string dummy;
        REQUIRE(w->GetDestData(dst1, "key1", &dummy));
        REQUIRE(!w->GetDestData(dst1, "key2", &dummy));
        REQUIRE(w->GetDestData(dst2, "key1", &dummy));
        REQUIRE(w->GetDestData(dst2, "key2", &dummy));
    }
}

TEST_CASE("no_dest_fails") {
    WalletTestingSetup setup;
    auto walletdbwrapper = TmpDB(setup.pathTemp, "no_dest_fails");
    CWalletDB walletdb(*walletdbwrapper, "cr+");

    CTxDestination dst = CNoDestination{};
    REQUIRE(!walletdb.WriteName(dst, "name"));
    REQUIRE(!walletdb.WritePurpose(dst, "purpose"));
}
