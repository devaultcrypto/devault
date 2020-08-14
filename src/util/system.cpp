// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h>

#include <fs.h>
#include <util/system.h>
#include <util/fs_util.h>
#include <chainparamsbase.h>
#include <random.h>
#include <serialize.h>
#include <util/strencodings.h>
#include <util/time.h>

#ifndef NO_BOOST_FILESYSTEM
#include <boost/filesystem/fstream.hpp>
using fs::ifstream;
#else
#include <fstream>
using std::ifstream;
#endif

#include <cstdarg>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#ifndef WIN32
// for posix_fallocate
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__

#include <algorithm>
#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>

#else

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4804)
#pragma warning(disable : 4805)
#pragma warning(disable : 4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <codecvt>

#include <io.h> /* for _commit */
#include <shellapi.h>
#include <shlobj.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

#include <thread>

// Application startup time (used for uptime calculation)
const int64_t nStartupTime = GetTime();

const char *const BITCOIN_CONF_FILENAME = "devault.conf";

ArgsManager gArgs;

/**
 * Interpret a string argument as a boolean.
 *
 * The definition of atoi() requires that non-numeric string values like "foo",
 * return 0. This means that if a user unintentionally supplies a non-integer
 * argument here, the return value is always false. This means that -foo=false
 * does what the user probably expects, but -foo=true is well defined but does
 * not do what they probably expected.
 *
 * The return value of atoi() is undefined when given input not representable as
 * an int. On most systems this means string value between "-2147483648" and
 * "2147483647" are well defined (this method will return true). Setting
 * -txindex=2147483648 on most systems, however, is probably undefined.
 *
 * For a more extensive discussion of this topic (and a wide range of opinions
 * on the Right Way to change this code), see PR12713.
 */
static bool InterpretBool(const std::string &strValue) {
    if (strValue.empty()) {
        return true;
    }
    return (std::atoi(strValue.c_str()) != 0);
}

/** Internal helper functions for ArgsManager */
class ArgsManagerHelper {
public:
    typedef std::map<std::string, std::vector<std::string>> MapArgs;

    /** Determine whether to use config settings in the default section,
     *  See also comments around ArgsManager::ArgsManager() below. */
    static inline bool UseDefaultSection(const ArgsManager &am,
                                         const std::string &arg) {
        return (am.m_network == CBaseChainParams::MAIN ||
                am.m_network_only_args.count(arg) == 0);
    }

    /** Convert regular argument into the network-specific setting */
    static inline std::string NetworkArg(const ArgsManager &am,
                                         const std::string &arg) {
        assert(arg.length() > 1 && arg[0] == '-');
        return "-" + am.m_network + "." + arg.substr(1);
    }

    /** Find arguments in a map and add them to a vector */
    static inline void AddArgs(std::vector<std::string> &res,
                               const MapArgs &map_args,
                               const std::string &arg) {
        auto it = map_args.find(arg);
        if (it != map_args.end()) {
            res.insert(res.end(), it->second.begin(), it->second.end());
        }
    }

    /**
     * Return true/false if an argument is set in a map, and also
     * return the first (or last) of the possibly multiple values it has
     */
    static inline std::pair<bool, std::string>
    GetArgHelper(const MapArgs &map_args, const std::string &arg,
                 bool getLast = false) {
        auto it = map_args.find(arg);

        if (it == map_args.end() || it->second.empty()) {
            return std::make_pair(false, std::string());
        }

        if (getLast) {
            return std::make_pair(true, it->second.back());
        } else {
            return std::make_pair(true, it->second.front());
        }
    }

    /**
     * Get the string value of an argument, returning a pair of a boolean
     * indicating the argument was found, and the value for the argument
     * if it was found (or the empty string if not found).
     */
    static inline std::pair<bool, std::string> GetArg(const ArgsManager &am,
                                                      const std::string &arg) {
        LOCK(am.cs_args);
        std::pair<bool, std::string> found_result(false, std::string());

        // We pass "true" to GetArgHelper in order to return the last
        // argument value seen from the command line (so "bitcoind -foo=bar
        // -foo=baz" gives GetArg(am,"foo")=={true,"baz"}
        found_result = GetArgHelper(am.m_override_args, arg, true);
        if (found_result.first) {
            return found_result;
        }

        // But in contrast we return the first argument seen in a config file,
        // so "foo=bar \n foo=baz" in the config file gives
        // GetArg(am,"foo")={true,"bar"}
        if (!am.m_network.empty()) {
            found_result = GetArgHelper(am.m_config_args, NetworkArg(am, arg));
            if (found_result.first) {
                return found_result;
            }
        }

        if (UseDefaultSection(am, arg)) {
            found_result = GetArgHelper(am.m_config_args, arg);
            if (found_result.first) {
                return found_result;
            }
        }

        return found_result;
    }

    /* Special test for -testnet and -regtest args, because we don't want to be
     * confused by craziness like "[regtest] testnet=1"
     */
    static inline bool GetNetBoolArg(const ArgsManager &am,
                                     const std::string &net_arg) {
        std::pair<bool, std::string> found_result(false, std::string());
        found_result = GetArgHelper(am.m_override_args, net_arg, true);
        if (!found_result.first) {
            found_result = GetArgHelper(am.m_config_args, net_arg, true);
            if (!found_result.first) {
                // not set
                return false;
            }
        }
        // is set, so evaluate
        return InterpretBool(found_result.second);
    }
};

/**
 * Interpret -nofoo as if the user supplied -foo=0.
 *
 * This method also tracks when the -no form was supplied, and if so, checks
 * whether there was a double-negative (-nofoo=0 -> -foo=1).
 *
 * If there was not a double negative, it removes the "no" from the key, and
 * returns true, indicating the caller should clear the args vector to indicate
 * a negated option.
 *
 * If there was a double negative, it removes "no" from the key, sets the value
 * to "1" and returns false.
 *
 * If there was no "no", it leaves key and value untouched and returns false.
 *
 * Where an option was negated can be later checked using the IsArgNegated()
 * method. One use case for this is to have a way to disable options that are
 * not normally boolean (e.g. using -nodebuglogfile to request that debug log
 * output is not sent to any file at all).
 */
static bool InterpretNegatedOption(std::string &key, std::string &val) {
    assert(key[0] == '-');

    size_t option_index = key.find('.');
    if (option_index == std::string::npos) {
        option_index = 1;
    } else {
        ++option_index;
    }
    if (key.substr(option_index, 2) == "no") {
        bool bool_val = InterpretBool(val);
        key.erase(option_index, 2);
        if (!bool_val) {
            // Double negatives like -nofoo=0 are supported (but discouraged)
            LogPrintf(
                "Warning: parsed potentially confusing double-negative %s=%s\n",
                key, val);
            val = "1";
        } else {
            return true;
        }
    }
    return false;
}

ArgsManager::ArgsManager()
    : /* These options would cause cross-contamination if values for mainnet
       * were used while running on regtest/testnet (or vice-versa).
       * Setting them as section_only_args ensures that sharing a config file
       * between mainnet and regtest/testnet won't cause problems due to these
       * parameters by accident. */
      m_network_only_args{
          "-addnode", "-connect", "-port",   "-bind",
          "-rpcport", "-rpcbind", "-wallet",
      } {
    // nothing to do
}

void ArgsManager::WarnForSectionOnlyArgs() {
    // if there's no section selected, don't worry
    if (m_network.empty()) {
        return;
    }

    // if it's okay to use the default section for this network, don't worry
    if (m_network == CBaseChainParams::MAIN) {
        return;
    }

    for (const auto &arg : m_network_only_args) {
        std::pair<bool, std::string> found_result;

        // if this option is overridden it's fine
        found_result = ArgsManagerHelper::GetArgHelper(m_override_args, arg);
        if (found_result.first) {
            continue;
        }

        // if there's a network-specific value for this option, it's fine
        found_result = ArgsManagerHelper::GetArgHelper(
            m_config_args, ArgsManagerHelper::NetworkArg(*this, arg));
        if (found_result.first) {
            continue;
        }

        // if there isn't a default value for this option, it's fine
        found_result = ArgsManagerHelper::GetArgHelper(m_config_args, arg);
        if (!found_result.first) {
            continue;
        }

        // otherwise, issue a warning
        LogPrintf("Warning: Config setting for %s only applied on %s network "
                  "when in [%s] section.\n",
                  arg, m_network, m_network);
    }
}

void ArgsManager::SelectConfigNetwork(const std::string &network) {
    m_network = network;
}

bool ArgsManager::ParseParameters(int argc, const char *const argv[],
                                  std::string &error) {
    LOCK(cs_args);
    m_override_args.clear();

    for (int i = 1; i < argc; i++) {
        std::string key(argv[i]);
        std::string val;
        size_t is_index = key.find('=');
        if (is_index != std::string::npos) {
            val = key.substr(is_index + 1);
            key.erase(is_index);
        }
#ifdef WIN32
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (key[0] == '/') {
            key[0] = '-';
        }
#endif

        if (key[0] != '-') {
            break;
        }

        // Transform --foo to -foo
        if (key.length() > 1 && key[1] == '-') {
            key.erase(0, 1);
        }

        // Check for -nofoo
        if (InterpretNegatedOption(key, val)) {
            m_override_args[key].clear();
        } else {
            m_override_args[key].push_back(val);
        }

        // Check that the arg is known
        if (!(IsSwitchChar(key[0]) && key.size() == 1)) {
            if (!IsArgKnown(key)) {
                error = strprintf("Invalid parameter %s", key.c_str());
                return false;
            }
        }
    }
    return true;
}

bool ArgsManager::IsArgKnown(const std::string &key) const {
    size_t option_index = key.find('.');
    std::string arg_no_net;
    if (option_index == std::string::npos) {
        arg_no_net = key;
    } else {
        arg_no_net =
            std::string("-") + key.substr(option_index + 1, std::string::npos);
    }

    for (const auto &arg_map : m_available_args) {
        if (arg_map.second.count(arg_no_net)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> ArgsManager::GetArgs(const std::string &strArg) const {
    std::vector<std::string> result = {};
    // special case
    if (IsArgNegated(strArg)) {
        return result;
    }

    LOCK(cs_args);

    ArgsManagerHelper::AddArgs(result, m_override_args, strArg);
    if (!m_network.empty()) {
        ArgsManagerHelper::AddArgs(
            result, m_config_args,
            ArgsManagerHelper::NetworkArg(*this, strArg));
    }

    if (ArgsManagerHelper::UseDefaultSection(*this, strArg)) {
        ArgsManagerHelper::AddArgs(result, m_config_args, strArg);
    }

    return result;
}

bool ArgsManager::IsArgSet(const std::string &strArg) const {
    // special case
    if (IsArgNegated(strArg)) {
        return true;
    }
    return ArgsManagerHelper::GetArg(*this, strArg).first;
}

bool ArgsManager::IsArgNegated(const std::string &strArg) const {
    LOCK(cs_args);

    const auto &ov = m_override_args.find(strArg);
    if (ov != m_override_args.end()) {
        return ov->second.empty();
    }

    if (!m_network.empty()) {
        const auto &cfs =
            m_config_args.find(ArgsManagerHelper::NetworkArg(*this, strArg));
        if (cfs != m_config_args.end()) {
            return cfs->second.empty();
        }
    }

    const auto &cf = m_config_args.find(strArg);
    if (cf != m_config_args.end()) {
        return cf->second.empty();
    }

    return false;
}

std::string ArgsManager::GetArg(const std::string &strArg,
                                const std::string &strDefault) const {
    if (IsArgNegated(strArg)) {
        return "0";
    }
    std::pair<bool, std::string> found_res =
        ArgsManagerHelper::GetArg(*this, strArg);
    if (found_res.first) {
        return found_res.second;
    }
    return strDefault;
}

int64_t ArgsManager::GetArg(const std::string &strArg, int64_t nDefault) const {
    if (IsArgNegated(strArg)) {
        return 0;
    }
    std::pair<bool, std::string> found_res =
        ArgsManagerHelper::GetArg(*this, strArg);
    if (found_res.first) {
      return std::atoll(found_res.second.c_str());
    }
    return nDefault;
}

bool ArgsManager::GetBoolArg(const std::string &strArg, bool fDefault) const {
    if (IsArgNegated(strArg)) {
        return false;
    }
    std::pair<bool, std::string> found_res =
        ArgsManagerHelper::GetArg(*this, strArg);
    if (found_res.first) {
        return InterpretBool(found_res.second);
    }
    return fDefault;
}

bool ArgsManager::SoftSetArg(const std::string &strArg,
                             const std::string &strValue) {
    LOCK(cs_args);
    if (IsArgSet(strArg)) {
        return false;
    }
    ForceSetArg(strArg, strValue);
    return true;
}

bool ArgsManager::SoftSetBoolArg(const std::string &strArg, bool fValue) {
    if (fValue) {
        return SoftSetArg(strArg, std::string("1"));
    } else {
        return SoftSetArg(strArg, std::string("0"));
    }
}

void ArgsManager::ForceSetArg(const std::string &strArg,
                              const std::string &strValue) {
    LOCK(cs_args);
    m_override_args[strArg] = {strValue};
}

/**
 * This function is only used for testing purpose so
 * so we should not worry about element uniqueness and
 * integrity of mapMultiArgs data structure
 */
void ArgsManager::ForceSetMultiArg(const std::string &strArg,
                                   const std::string &strValue) {
    LOCK(cs_args);
    m_override_args[strArg].push_back(strValue);
}

void ArgsManager::AddArg(const std::string &name, const std::string &help,
                         const bool debug_only, const OptionsCategory &cat) {
    // Split arg name from its help param
    size_t eq_index = name.find('=');
    if (eq_index == std::string::npos) {
        eq_index = name.size();
    }

    std::map<std::string, Arg> &arg_map = m_available_args[cat];
    auto ret = arg_map.emplace(
        name.substr(0, eq_index),
        Arg(name.substr(eq_index, name.size() - eq_index), help, debug_only));
    // Make sure an insertion actually happened.
    assert(ret.second);
}

void ArgsManager::ClearArg(const std::string &strArg) {
    LOCK(cs_args);
    m_override_args.erase(strArg);
    m_config_args.erase(strArg);
}

std::string ArgsManager::GetHelpMessage() const {
    const bool show_debug = gArgs.GetBoolArg("-help-debug", false);

    std::string usage = "";
    for (const auto &arg_map : m_available_args) {
        switch (arg_map.first) {
            case OptionsCategory::OPTIONS:
                usage += HelpMessageGroup("Options:");
                break;
            case OptionsCategory::CONNECTION:
                usage += HelpMessageGroup("Connection options:");
                break;
            case OptionsCategory::ZMQ:
                usage += HelpMessageGroup("ZeroMQ notification options:");
                break;
            case OptionsCategory::DEBUG_TEST:
                usage += HelpMessageGroup("Debugging/Testing options:");
                break;
            case OptionsCategory::NODE_RELAY:
                usage += HelpMessageGroup("Node relay options:");
                break;
            case OptionsCategory::BLOCK_CREATION:
                usage += HelpMessageGroup("Block creation options:");
                break;
            case OptionsCategory::RPC:
                usage += HelpMessageGroup("RPC server options:");
                break;
            case OptionsCategory::WALLET:
                usage += HelpMessageGroup("Wallet options:");
                break;
            case OptionsCategory::WALLET_DEBUG_TEST:
                if (show_debug) {
                    usage +=
                        HelpMessageGroup("Wallet debugging/testing options:");
                }
                break;
            case OptionsCategory::CHAINPARAMS:
                usage += HelpMessageGroup("Chain selection options:");
                break;
            case OptionsCategory::GUI:
                usage += HelpMessageGroup("UI Options:");
                break;
            case OptionsCategory::COMMANDS:
                usage += HelpMessageGroup("Commands:");
                break;
            case OptionsCategory::REGISTER_COMMANDS:
                usage += HelpMessageGroup("Register Commands:");
                break;
            default:
                break;
        }

        // When we get to the hidden options, stop
        if (arg_map.first == OptionsCategory::HIDDEN) {
            break;
        }

        for (const auto &arg : arg_map.second) {
            if (show_debug || !arg.second.m_debug_only) {
                std::string name;
                if (arg.second.m_help_param.empty()) {
                    name = arg.first;
                } else {
                    name = arg.first + arg.second.m_help_param;
                }
                usage += HelpMessageOpt(name, arg.second.m_help_text);
            }
        }
    }
    return usage;
}

bool HelpRequested(const ArgsManager &args) {
    return args.IsArgSet("-?") || args.IsArgSet("-h") || args.IsArgSet("-help");
}

static const int screenWidth = 79;
static const int optIndent = 2;
static const int msgIndent = 7;

std::string HelpMessageGroup(const std::string &message) {
    return std::string(message) + std::string("\n\n");
}

std::string HelpMessageOpt(const std::string &option,
                           const std::string &message) {
    return std::string(optIndent, ' ') + std::string(option) +
           std::string("\n") + std::string(msgIndent, ' ') +
           FormatParagraph(message, screenWidth - msgIndent, msgIndent) +
           std::string("\n\n");
}

static std::string FormatException(const std::exception *pex,
                                   const char *pszThread) {
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(nullptr, pszModule, sizeof(pszModule));
#else
    const char *pszModule = "devault";
#endif
    if (pex) {
        return strprintf("EXCEPTION: %s       \n%s       \n%s in %s       \n",
                         typeid(*pex).name(), pex->what(), pszModule,
                         pszThread);
    } else {
        return strprintf("UNKNOWN EXCEPTION       \n%s in %s       \n",
                         pszModule, pszThread);
    }
}

void PrintExceptionContinue(const std::exception *pex, const char *pszThread) {
    std::string message = FormatException(pex, pszThread);
    LogPrintf("\n\n************************\n%s\n", message);
    fprintf(stderr, "\n\n************************\n%s\n", message.c_str());
}

static std::string TrimString(const std::string& str, const std::string& pattern)
{
  std::string::size_type front = str.find_first_not_of(pattern);
  if (front == std::string::npos) {
    return std::string();
  }
  std::string::size_type end = str.find_last_not_of(pattern);
  return str.substr(front, end - front + 1);
}

static std::vector<std::pair<std::string, std::string>> GetConfigOptions(std::istream& stream)
{
  std::vector<std::pair<std::string, std::string>> options;
  std::string str, prefix;
  std::string::size_type pos;
  while (std::getline(stream, str)) {
    if ((pos = str.find('#')) != std::string::npos) {
      str = str.substr(0, pos);
    }
    const static std::string pattern = " \t\r\n";
    str = TrimString(str, pattern);
    if (!str.empty()) {
      if (*str.begin() == '[' && *str.rbegin() == ']') {
        prefix = str.substr(1, str.size() - 2) + '.';
      } else if ((pos = str.find('=')) != std::string::npos) {
        std::string name = prefix + TrimString(str.substr(0, pos), pattern);
        std::string value = TrimString(str.substr(pos + 1), pattern);
        options.emplace_back(name, value);
      }
    }
  }
  return options;
}

bool ArgsManager::ReadConfigStream(std::istream &stream, std::string &error,
                                   bool ignore_invalid_keys) {
    LOCK(cs_args);

    for (const std::pair<std::string, std::string>& option : GetConfigOptions(stream)) {
      std::string strKey = std::string("-") + option.first;
      std::string strValue = option.second;
      if (InterpretNegatedOption(strKey, strValue)) {
        m_config_args[strKey].clear();
      } else {
        m_config_args[strKey].push_back(strValue);
      }

      // Check that the arg is known
      if (!IsArgKnown(strKey) && !ignore_invalid_keys) {
          error = strprintf("Invalid configuration value %s", strKey.c_str());
          return false;
      }
    }
    return true;
}

bool ArgsManager::ReadConfigFiles(std::string &error,
                                  bool ignore_invalid_keys) {
    {
        LOCK(cs_args);
        m_config_args.clear();
    }
    const std::string confPath = GetArg("-conf", BITCOIN_CONF_FILENAME);
    ifstream stream(GetConfigFile(confPath));
    // ok to not have a config file
    if (stream.good()) {
        if (!ReadConfigStream(stream, error, ignore_invalid_keys)) {
            return false;
        }
    }

    // If datadir is changed in .conf file:
    ClearDatadirCache();
    if (!fs::is_directory(GetDataDir(false))) {
        error = strprintf("specified data directory \"%s\" does not exist.",
                          gArgs.GetArg("-datadir", "").c_str());
        return false;
    }
    return true;
}

std::string ArgsManager::GetChainName() const {
    bool fRegTest = ArgsManagerHelper::GetNetBoolArg(*this, "-regtest");
    bool fTestNet = ArgsManagerHelper::GetNetBoolArg(*this, "-testnet");

    if (fTestNet && fRegTest) {
        throw std::runtime_error(
            "Invalid combination of -regtest and -testnet.");
    }
    if (fRegTest) {
        return CBaseChainParams::REGTEST;
    }
    if (fTestNet) {
        return CBaseChainParams::TESTNET;
    }
    return CBaseChainParams::TESTNET;
}

/**
 * This function tries to raise the file descriptor limit to the requested
 * number. It returns the actual file descriptor limit (which may be more or
 * less than nMinFD)
 */
int RaiseFileDescriptorLimit(int nMinFD) {
#if defined(WIN32)
    return 2048;
#else
    struct rlimit limitFD;
    if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
        if (limitFD.rlim_cur < (rlim_t)nMinFD) {
            limitFD.rlim_cur = nMinFD;
            if (limitFD.rlim_cur > limitFD.rlim_max) {
                limitFD.rlim_cur = limitFD.rlim_max;
            }
            setrlimit(RLIMIT_NOFILE, &limitFD);
            getrlimit(RLIMIT_NOFILE, &limitFD);
        }
        return limitFD.rlim_cur;
    }
    // getrlimit failed, assume it's fine.
    return nMinFD;
#endif
}

void runCommand(const std::string &strCommand) {
    if (strCommand.empty()) {
        return;
    }
#ifndef WIN32
    int nErr = ::system(strCommand.c_str());
#else
    int nErr = ::_wsystem(
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>()
            .from_bytes(strCommand)
            .c_str());
#endif
    if (nErr) {
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand,
                  nErr);
    }
}

void RenameThread(const char *name) {
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), name);

#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

bool SetupNetworking() {
#ifdef WIN32
    // Initialize Windows Sockets.
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion) != 2 ||
        HIBYTE(wsadata.wVersion) != 2) {
        return false;
    }
#endif
    return true;
}

int GetNumCores() {
    return std::thread::hardware_concurrency();
}

std::string CopyrightHolders(const std::string &strPrefix) {
    return strPrefix + "The Bitcoin developers";
}

// Obtain the application startup time (used for uptime calculation)
int64_t GetStartupTime() {
    return nStartupTime;
}

int ScheduleBatchPriority() {
#ifdef SCHED_BATCH
    const static sched_param param{};
    if (int ret = pthread_setschedparam(pthread_self(), SCHED_BATCH, &param)) {
        LogPrintf("Failed to pthread_setschedparam: %s\n", strerror(errno));
        return ret;
    }
    return 0;
#else
    return 1;
#endif
}

namespace util {
#ifdef WIN32
WinCmdLineArgs::WinCmdLineArgs() {
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8_cvt;
    argv = new char *[argc];
    args.resize(argc);
    for (int i = 0; i < argc; i++) {
        args[i] = utf8_cvt.to_bytes(wargv[i]);
        argv[i] = &*args[i].begin();
    }
    LocalFree(wargv);
}

WinCmdLineArgs::~WinCmdLineArgs() {
    delete[] argv;
}

std::pair<int, char **> WinCmdLineArgs::get() {
    return std::make_pair(argc, argv);
}
#endif
} // namespace util
