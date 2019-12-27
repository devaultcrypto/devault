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
#include <util/system.h>
#include <util/fs_util.h>
#include <wallet/wallettool.h>

#include <functional>
#include <stdio.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static bool WalletAppInit(int argc, char* argv[]) {
    SetupChainParamsBaseOptions();
    if (HelpRequested(gArgs)) {
        std::string usage = strprintf("%s bdb-check version", "DeVault Core") + " " + FormatFullVersion() + "\n\n" +
            "bdb-check is an offline tool for examining DeVault wallet files.\n" +
            "It will act on wallets in the default mainnet wallet directory.\n" +
            "With some source code modification it could be made to handle non-default options.\n\n" +
            "Usage:\n" +
            "  bdb-check\n\n" +
            gArgs.GetHelpMessage();

        tfm::format(std::cout, "%s", usage.c_str());
        return false;
    }
    
    BCLog::Logger &logger = GetLogger();
    logger.m_print_to_console = true; //gArgs.GetBoolArg("-printtoconsole", gArgs.GetBoolArg("-debug", false));
    
    gArgs.AddArg("-testnet", _("Use the test chain"), false, OptionsCategory::OPTIONS);
    
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        fprintf(stderr, "Error parsing command line arguments: %s\n",
                error.c_str());
        return EXIT_FAILURE;
    }


    bool fTestNet =  gArgs.IsArgSet("-testnet");
    
    if (fTestNet) SelectParams(CBaseChainParams::TESTNET);
    
    if (!fs::is_directory(GetDataDir(fTestNet))) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
        return false;
    }
    SelectParams(gArgs.GetChainName());
    return true;
}

int main(int argc, char* argv[]) {
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
    
    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::GetWalletInfo())
        return EXIT_FAILURE;
    ECC_Stop();
    return EXIT_SUCCESS;
}
