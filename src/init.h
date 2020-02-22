// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <util/system.h>

#include <memory>
#include <string>
#include <support/allocators/secure.h>

class Config;
class CScheduler;
class CWallet;
class HTTPRPCRequestProcessor;
class RPCServer;

class WalletInitInterface;
extern const WalletInitInterface &g_wallet_init_interface;


/** Interrupt threads */
void Interrupt();
/** Interrupt all script checking threads once they're out of work */
void InterruptThreadScriptCheck();
void Shutdown();
//! Initialize the logging infrastructure
void InitLogging();
//! Parameter interaction: change current parameters depending on various rules
void InitParameterInteraction();

/**
 * Initialize bitcoin: Basic context setup.
 * @note This can be done before daemonization.
 * Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read.
 */
bool AppInitBasicSetup();
/**
 * Initialization: parameter interaction.
 * @note This can be done before daemonization.
 * Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read,
 * AppInitBasicSetup should have been called.
 */
bool AppInitParameterInteraction(Config &config);
/**
 * Initialization sanity checks: ecc init, sanity checks, dir lock.
 * @note This can be done before daemonization.
 * Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read,
 * AppInitParameterInteraction should have been called.
 */
bool AppInitSanityChecks();
/**
 * Lock bitcoin data directory.
 * @note This should only be done after daemonization.
 * Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read,
 * AppInitSanityChecks should have been called.
 */
bool AppInitLockDataDirectory();
/**
 * Bitcoin main initialization.
 * @note This should only be done after daemonization.
 * @pre Parameters should be parsed and config file should be read,
 * AppInitLockDataDirectory should have been called.
 */
bool AppInitMain(Config &config,
                 RPCServer &rpcServer,
                 HTTPRPCRequestProcessor &httpRPCRequestProcessor,
                 const SecureString& walletPassphrase,
                 const std::vector<std::string>& words, bool use_bls);

/**
 * Setup the arguments for gArgs.
 */
void SetupServerArgs();

/** Returns licensing information (for -version) */
std::string LicenseInfo();

// This is a convenience for miners who don't want to use wallet passwords explicitly
// For devaultd add the option -bypasspassword for this to be used
const SecureString BypassPassword("");
