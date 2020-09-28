// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Server/client environment: argument handling, config file parsing,
 * thread wrappers, startup time
 */
#ifndef BITCOIN_UTIL_SYSTEM_H
#define BITCOIN_UTIL_SYSTEM_H

#include <config/bitcoin-config.h>

#include <compat.h>
#include <compat/assumptions.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/time.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <functional>

#include <condition_variable>

// Application startup time (used for uptime calculation)
int64_t GetStartupTime();

extern const char *const BITCOIN_CONF_FILENAME;

class thread_interrupted {};

inline void interruption_point(bool interrupt) {
    if (interrupt) {
        throw thread_interrupted();
    }
}

/** Translate a message to the native language of the user. */
const extern std::function<std::string(const char *)> G_TRANSLATION_FUN;

/**
 * Translation function: return input if no translation function
 */
inline std::string _(const char *psz) {
  return G_TRANSLATION_FUN ? (G_TRANSLATION_FUN)(psz) : psz;
}

bool SetupNetworking();

template <typename... Args> bool error(const char *fmt, const Args &... args) {
    LogPrintf("ERROR: " + tfm::format(fmt, args...) + "\n");
    return false;
}

void PrintExceptionContinue(const std::exception *pex, const char *pszThread);
int RaiseFileDescriptorLimit(int nMinFD);
void runCommand(const std::string &strCommand);

bool ParseKeyValue(std::string &key, std::string &val);

inline bool IsSwitchChar(char c) {
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

enum class OptionsCategory {
    OPTIONS,
    CONNECTION,
    WALLET,
    WALLET_DEBUG_TEST,
    ZMQ,
    DEBUG_TEST,
    CHAINPARAMS,
    NODE_RELAY,
    BLOCK_CREATION,
    RPC,
    GUI,
    COMMANDS,
    REGISTER_COMMANDS,

    // Always the last option to avoid printing these in the help
    HIDDEN,
};

class ArgsManager {
protected:
    friend class ArgsManagerHelper;

    struct Arg {
        std::string m_help_param;
        std::string m_help_text;
        bool m_debug_only;

        Arg(const std::string &help_param, const std::string &help_text,
            bool debug_only)
            : m_help_param(help_param), m_help_text(help_text),
              m_debug_only(debug_only){};
    };

    mutable CCriticalSection cs_args;
    std::map<std::string, std::vector<std::string>> m_override_args;
    std::map<std::string, std::vector<std::string>> m_config_args;
    std::string m_network;
    std::set<std::string> m_network_only_args;
    std::map<OptionsCategory, std::map<std::string, Arg>> m_available_args;

    bool ReadConfigStream(std::istream &stream, std::string &error,
                          bool ignore_invalid_keys = false);

public:
    ArgsManager();

    /**
     * Select the network in use
     */
    void SelectConfigNetwork(const std::string &network);

    bool ParseParameters(int argc, const char *const argv[],
                         std::string &error);
    bool ReadConfigFiles(std::string &error, bool ignore_invalid_keys = false);

    /**
     * Log warnings for options in m_section_only_args when they are specified
     * in the default section but not overridden on the command line or in a
     * network-specific section in the config file.
     */
    void WarnForSectionOnlyArgs();

    /**
     * Return a vector of strings of the given argument
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return command-line arguments
     */
    std::vector<std::string> GetArgs(const std::string &strArg) const;

    /**
     * Return true if the given argument has been manually set.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument has been set
     */
    bool IsArgSet(const std::string &strArg) const;

    /**
     * Return true if the argument was originally passed as a negated option,
     * i.e. -nofoo.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument was passed negated
     */
    bool IsArgNegated(const std::string &strArg) const;

    /**
     * Return string argument or default value.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (e.g. "1")
     * @return command-line argument or default value
     */
    std::string GetArg(const std::string &strArg,
                       const std::string &strDefault) const;

    /**
     * Return integer argument or default value.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (e.g. 1)
     * @return command-line argument (0 if invalid number) or default value
     */
    int64_t GetArg(const std::string &strArg, int64_t nDefault) const;

    /**
     * Return boolean argument or default value.
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (true or false)
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string &strArg, bool fDefault) const;

    /**
     * Set an argument if it doesn't already have a value.
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param strValue Value (e.g. "1")
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetArg(const std::string &strArg, const std::string &strValue);

    /**
     * Set a boolean argument if it doesn't already have a value.
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param fValue Value (e.g. false)
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetBoolArg(const std::string &strArg, bool fValue);

    // Forces a arg setting, used only in testing
    void ForceSetArg(const std::string &strArg, const std::string &strValue);

    // Forces a multi arg setting, used only in testing
    void ForceSetMultiArg(const std::string &strArg,
                          const std::string &strValue);

    /**
     * Looks for -regtest, -testnet and returns the appropriate BIP70 chain
     * name.
     * @return CBaseChainParams::MAIN by default; raises runtime error if an
     * invalid combination is given.
     */
    std::string GetChainName() const;

    /**
     * Add argument
     */
    void AddArg(const std::string &name, const std::string &help,
                const bool debug_only, const OptionsCategory &cat);

    // Remove an arg setting, used only in testing
    void ClearArg(const std::string &strArg);
    
    /**
     * Clear available arguments
     */
    void ClearArgs() { m_available_args.clear(); }

    /**
     * Get the help string
     */
    std::string GetHelpMessage() const;

    /**
     * Check whether we know of this arg
     */
    bool IsArgKnown(const std::string &key) const;
};

extern ArgsManager gArgs;

/**
 * @return true if help has been requested via a command-line arg
 */
bool HelpRequested(const ArgsManager &args);

/**
 * Format a string to be used as group of options in help messages.
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string &message);

/**
 * Format a string to be used as option description in help messages.
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string &option,
                           const std::string &message);

/**
 * Return the number of physical cores available on the current system.
 * @note This does not count virtual cores, such as those provided by
 * HyperThreading when boost is newer than 1.56.
 */
int GetNumCores();

void RenameThread(const char *name);

/**
 * .. and a wrapper that just calls func once
 */
template <typename Callable> void TraceThread(const char *name, Callable func) {
    std::string s = strprintf("devault-%s", name);
    RenameThread(s.c_str());
    try {
        LogPrintf("%s thread start\n", name);
        func();
        LogPrintf("%s thread exit\n", name);
    } catch (const thread_interrupted &) {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    } catch (const std::exception &e) {
        PrintExceptionContinue(&e, name);
        throw;
    } catch (...) {
        PrintExceptionContinue(nullptr, name);
        throw;
    }
}

std::string CopyrightHolders(const std::string &strPrefix);

namespace util {

//! Simplification of std insertion
template <typename Tdst, typename Tsrc>
inline void insert(Tdst &dst, const Tsrc &src) {
    dst.insert(dst.begin(), src.begin(), src.end());
}
template <typename TsetT, typename Tsrc>
inline void insert(std::set<TsetT> &dst, const Tsrc &src) {
    dst.insert(src.begin(), src.end());
}

#ifdef WIN32
class WinCmdLineArgs {
public:
    WinCmdLineArgs();
    ~WinCmdLineArgs();
    std::pair<int, char **> get();

private:
    int argc;
    char **argv;
    std::vector<std::string> args;
};
#endif

} // namespace util

/**
 * On platforms that support it, tell the kernel the calling thread is
 * CPU-intensive and non-interactive. See SCHED_BATCH in sched(7) for details.
 *
 * @return The return value of sched_setschedule(), or 1 on systems without
 * sched_setchedule().
 */
int ScheduleBatchPriority();

#endif // BITCOIN_UTIL_SYSTEM_H
