// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <chainparamsbase.h>
#include <logging.h>
#include <utilstrencodings.h>
#include <util.h>
#include <fs_util.h>
#include <wallet/wallettool.h>

#include <functional>
#include <stdio.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void SetupWalletToolArgs()
{
    SetupChainParamsBaseOptions();
    gArgs.AddArg("-datadir=<dir>", "Specify data directory",  true, OptionsCategory::OPTIONS);
    gArgs.AddArg("-wallet=<wallet-name>", "Specify wallet name",  true, OptionsCategory::OPTIONS);
    gArgs.AddArg("info", "Get wallet info", true, OptionsCategory::COMMANDS);
}

static bool WalletAppInit(int argc, char* argv[])
{
    SetupWalletToolArgs();
    if (argc < 2 || HelpRequested(gArgs)) {
        std::string usage = strprintf("%s bdb-check version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n\n" +
                                      "wallet-tool is an offline tool for examining DeVault wallet files.\n" +
                                      "By default wallet-tool will act on wallets in the default mainnet wallet directory in the datadir.\n" +
                                      "To change the target wallet, use the -datadir, -wallet arguments.\n\n" +
                                      "Usage:\n" +
                                     "  bdb-check [options] <command>\n\n" +
                                     gArgs.GetHelpMessage();

        tfm::format(std::cout, "%s", usage.c_str());
        return false;
    }

    BCLog::Logger &logger = GetLogger();
    logger.m_print_to_console = true; //gArgs.GetBoolArg("-printtoconsole", gArgs.GetBoolArg("-debug", false));

    if (!fs::is_directory(GetDataDir(false))) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
        return false;
    }
    SelectParams(gArgs.GetChainName());
    return true;
}

int main(int argc, char* argv[])
{
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();
    try {
        if (!WalletAppInit(argc, argv)) return EXIT_FAILURE;
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "WalletAppInit()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "WalletAppInit()");
        return EXIT_FAILURE;
    }

    std::string name = gArgs.GetArg("-wallet", "wallet.dat");
    fs::path wallet_path = GetDataDir() / "wallets" / name;
    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::GetWalletInfo(wallet_path.c_str()))
        return EXIT_FAILURE;
    ECC_Stop();
    return EXIT_SUCCESS;
}
