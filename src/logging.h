// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <tinyformat.h>

#include <atomic>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;

// For debug.log log files, used in init and logging.cpp
static const std::string DEFAULT_DEBUGLOGFILE = "debug.log";
static const uint32_t DEFAULT_DEBUGLOGFILE_KEEPDAYS = 7;

extern bool fLogIPs;

struct CLogCategoryActive {
    std::string category;
    bool active;
};

namespace BCLog {

enum LogFlags : uint32_t {
    NONE = 0,
    NET = (1 << 0),
    TOR = (1 << 1),
    MEMPOOL = (1 << 2),
    HTTP = (1 << 3),
    BENCHM = (1 << 4),
    ZMQ = (1 << 5),
    DB = (1 << 6),
    RPC = (1 << 7),
    ESTIMATEFEE = (1 << 8),
    ADDRMAN = (1 << 9),
    SELECTCOINS = (1 << 10),
    REINDEX = (1 << 11),
    CMPCTBLOCK = (1 << 12),
    RANDOM = (1 << 13),
    PRUNE = (1 << 14),
    PROXY = (1 << 15),
    MEMPOOLREJ = (1 << 16),
    LIBEVENT = (1 << 17),
    COINDB = (1 << 18),
    QT = (1 << 19),
    LEVELDB = (1 << 20),
    COLD = (1 << 21),
    ALL = ~uint32_t(0),
};

class Logger {
private:
    FILE *m_fileout = nullptr;
    std::mutex m_file_mutex;
    std::list<std::string> m_msgs_before_open;

    /**
     * m_started_new_line is a state variable that will suppress printing of the
     * timestamp when multiple calls are made that don't end in a newline.
     */
    std::atomic_bool m_started_new_line{true};

    /**
     * Log categories bitfield.
     */
    std::atomic<uint32_t> m_categories{0};

    std::string LogTimestampStr(const std::string &str);

public:
    bool m_print_to_console = false;
    bool m_print_to_file = true;

    bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
    bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;

    std::atomic<bool> m_reopen_file{false};

    ~Logger();

    /** Send a string to the log output */
    void LogPrintStr(const std::string &str);

    bool OpenDebugLog();
    void ShrinkDebugFile();
    std::string RenameLastDebugFile();
    void RemoveOlderDebugFiles();


    uint32_t GetCategoryMask() const { return m_categories.load(); }

    void EnableCategory(LogFlags category);
    bool EnableCategory(const std::string &str);
    void DisableCategory(LogFlags category);
    bool DisableCategory(const std::string &str);

    /** Return true if log accepts specified category */
    bool WillLogCategory(LogFlags category) const;

    /** Default for whether ShrinkDebugFile should be run */
    bool DefaultShrinkDebugFile() const;
};

} // namespace BCLog

BCLog::Logger &GetLogger();

/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(BCLog::LogFlags category) {
    return GetLogger().WillLogCategory(category);
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags &flag, const std::string &str);

// Be conservative when using LogPrintf/error or other things which
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a users disk with debug.log entries.

#define LogPrint(category, ...)                                                \
    do {                                                                       \
        if (LogAcceptCategory((category))) {                                   \
            GetLogger().LogPrintStr(tfm::format(__VA_ARGS__));                 \
        }                                                                      \
    } while (0)

#define LogPrintf(...)                                                         \
    do {                                                                       \
        GetLogger().LogPrintStr(tfm::format(__VA_ARGS__));                     \
    } while (0)


#define LogPrintfToBeContinued LogPrintf
#define LogPrintToBeContinued LogPrint

#endif // BITCOIN_LOGGING_H
