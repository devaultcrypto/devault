// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h>

#include <chainparams.h>
#include <clientversion.h>
#include <compat.h>
#include <config.h>
#include <httprpc.h>
#include <httpserver.h>
#include <init.h>
#include <noui.h>
#include <rpc/server.h>
#include <util/fs_util.h>
#include <shutdown.h>
#include <util/strencodings.h>
#include <walletinitinterface.h>
#include <support/allocators/secure.h>

#include <cstdint>
#include <thread>
#include <cstdio>

const std::function<std::string(const char *)> G_TRANSLATION_FUN = nullptr;

/* Introduction text for doxygen: */

/*! \mainpage Developer documentation
 *
 * \section intro_sec Introduction
 *
 * This is the developer documentation of DeVault
 * (https://www.devault.cc/). DeVault Core is a client for the digital
 * currency called Devault, which enables, instant payments to anyone, anywhere in the world.
 * DeVault uses peer-to-peer technology to operate with no central authority: managing
 * transactions and issuing money are carried out collectively by the network.
 *
 * The software is a community-driven open source project, released under the
 * MIT license.
 *
 * \section Navigation
 * Use the buttons <code>Namespaces</code>, <code>Classes</code> or
 * <code>Files</code> at the top of the page to start navigating the code.
 */

void WaitForShutdown() {
    while (!ShutdownRequested()) {
        MilliSleep(200);
    }
    Interrupt();
}

void getPassphrase(SecureString& walletPassphrase) {
  std::cout << "Enter a Wallet Encryption password (at least 4 characters and upto 40 characters long)\n";
  SecureString pass1;
  SecureString pass2;
  int char_count=0; // This is used to handle Ctrl-C which would create
  // an infinite loop and crash below otherwise
  char c='0';
  const int min_char_count = 2*4 + 1;
  do {
    do {
      std::cin.get(c);
      char_count++;
      if (char_count++ > 81) {
        // Don't print message, just exit because it can be due to Ctrl-C
        exit(0);
      }
      if (c != '\n') pass1.push_back(c);
    } while (c != '\n');
    if (char_count < min_char_count) {
      std::cout << "Password must be at least 4 characters long, please restart and retry\n";
      exit(0);
    }
  } while (char_count < min_char_count);
  c = '0';
  char_count = 0;
  std::cout << "Confirm password\n";
  while (c != '\n') {
    std::cin.get(c);
    if (char_count++ > 81) exit(0);
    if (c != '\n') pass2.push_back(c);
  }
  if (pass1 != pass2) {
    std::cout << "Passwords don't match, please restart and retry\n";
    exit(0);
  }
  walletPassphrase   = pass1;
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char *argv[]) {
    // FIXME: Ideally, we'd like to build the config here, but that's currently
    // not possible as the whole application has too many global state. However,
    // this is a first step.
    auto &config = const_cast<Config &>(GetConfig());
    RPCServer rpcServer;
    HTTPRPCRequestProcessor httpRPCRequestProcessor(config, rpcServer);

    bool fRet = false;
    SecureString walletPassphrase;
    //
    // Parameters
    //
    // If Qt is used, parameters/devault.conf are parsed in qt/bitcoin.cpp's
    // main()
    SetupServerArgs();
#if HAVE_DECL_DAEMON
    gArgs.AddArg("-daemon",
                 _("Run in the background as a daemon and accept commands"),
                 false, OptionsCategory::OPTIONS);
#endif
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        fprintf(stderr, "Error parsing command line arguments: %s\n",
                error.c_str());
        return false;
    }

    // Process help and version before taking care about datadir
    if (HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        std::string strUsage =
            "DeVault Core Daemon version " + FormatFullVersion() + "\n";

        if (gArgs.IsArgSet("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\nUsage:  devaultd [options]                     "
                        "Start " "DeVault Core Daemon\n";

            strUsage += "\n" + gArgs.GetHelpMessage();
        }

        fprintf(stdout, "%s", strUsage.c_str());
        return true;
    }

    try {
      
        if (!fs::is_directory(GetDataDir(false))) {
            fprintf(stderr,
                    "Error: Specified data directory \"%s\" does not exist.\n",
                    gArgs.GetArg("-datadir", "").c_str());
            return false;
        }
        if (!gArgs.ReadConfigFiles(error)) {
            fprintf(stderr, "Error reading configuration file: %s\n",
                    error.c_str());
            return false;
        }
        // Check for -testnet or -regtest parameter (Params() calls are only
        // valid after this clause)
        try {
            SelectParams(gArgs.GetChainName());
        } catch (const std::exception &e) {
            fprintf(stderr, "Error: %s\n", e.what());
            return false;
        }
      
        // Error out when loose non-argument tokens are encountered on command
        // line
        for (int i = 1; i < argc; i++) {
            if (!IsSwitchChar(argv[i][0])) {
                fprintf(stderr,
                        "Error: Command line contains unexpected token '%s', "
                        "see devaultd -h for a list of options.\n",
                        argv[i]);
                return false;
            }
        }

        // -server defaults to true for bitcoind but not for the GUI so do this
        // here
        gArgs.SoftSetBoolArg("-server", true);
        // Set this early so that parameter interactions go to console
        InitLogging();
        InitParameterInteraction();
        if (!AppInitBasicSetup()) {
            // InitError will have been called with detailed error, which ends
            // up on console
            return false;
        }
        if (!AppInitParameterInteraction(config)) {
            // InitError will have been called with detailed error, which ends
            // up on console
            return false;
        }

        if (g_wallet_init_interface.HasWalletSupport()) {
            if (!g_wallet_init_interface.CheckIfWalletExists(config.GetChainParams())) {
                if (!gArgs.GetBoolArg("-bypasspassword",false)) {
                    getPassphrase(walletPassphrase);
                } else {
                    walletPassphrase = BypassPassword;
                }
            }
        }

        if (!AppInitSanityChecks()) {
            // InitError will have been called with detailed error, which ends
            // up on console
            return false;
        }
        if (gArgs.GetBoolArg("-daemon", false)) {
#if HAVE_DECL_DAEMON
#if defined(MAC_OSX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
            fprintf(stdout, "DeVault server starting\n");

            // Daemonize
            if (daemon(1, 0)) {
                // don't chdir (1), do close FDs (0)
                fprintf(stderr, "Error: daemon() failed: %s\n",
                        strerror(errno));
                return false;
            }
#if defined(MAC_OSX)
#pragma GCC diagnostic pop
#endif
#else
            fprintf(
                stderr,
                "Error: -daemon is not supported on this operating system\n");
            return false;
#endif // HAVE_DECL_DAEMON
        }

        // Lock data directory after daemonization
        if (!AppInitLockDataDirectory()) {
            // If locking the data directory failed, exit immediately
            return false;
        }
        std::vector<std::string> words;
        bool use_bls = true;
        fRet = AppInitMain(config, rpcServer, httpRPCRequestProcessor, walletPassphrase, words, use_bls);
    } catch (const std::exception &e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

    if (!fRet) {
        Interrupt();
    } else {
        WaitForShutdown();
    }
    Shutdown();

    return fRet;
}

int main(int argc, char *argv[]) {
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();

    // Connect bitcoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
