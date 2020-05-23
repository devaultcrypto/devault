// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <chrono>
#include <util/fs_util.h>
#include <util/system.h>
#include <util/time.h>

bool fLogIPs = DEFAULT_LOGIPS;

/**
 * NOTE: the logger instance is leaked on exit. This is ugly, but will be
 * cleaned up by the OS/libc. Defining a logger as a global object doesn't work
 * since the order of destruction of static/global objects is undefined.
 * Consider if the logger gets destroyed, and then some later destructor calls
 * LogPrintf, maybe indirectly, and you get a core dump at shutdown trying to
 * access the logger. When the shutdown sequence is fully audited and tested,
 * explicit destruction of these objects can be implemented by changing this
 * from a raw pointer to a std::unique_ptr.
 *
 * This method of initialization was originally introduced in
 * ee3374234c60aba2cc4c5cd5cac1c0aefc2d817c.
 */
BCLog::Logger &GetLogger() {
    static BCLog::Logger *const logger = new BCLog::Logger();
    return *logger;
}

static int FileWriteStr(const std::string &str, FILE *fp) {
    return fwrite(str.data(), 1, str.size(), fp);
}

bool BCLog::Logger::OpenDebugLog() {
    std::lock_guard<std::mutex> scoped_lock(m_file_mutex);

    assert(m_fileout == nullptr);
    fs::path pathDebug = GetDebugLogPath();

    m_fileout = fsbridge::fopen(pathDebug, "a");
    if (!m_fileout) {
        return false;
    }

    // Unbuffered.
    setbuf(m_fileout, nullptr);
    // Dump buffered messages from before we opened the log.
    while (!m_msgs_before_open.empty()) {
        FileWriteStr(m_msgs_before_open.front(), m_fileout);
        m_msgs_before_open.pop_front();
    }

    return true;
}

struct CLogCategoryDesc {
    BCLog::LogFlags flag;
    std::string category;
};

const CLogCategoryDesc LogCategories[] = {
    {BCLog::NONE, "0"},
    {BCLog::NONE, "none"},
    {BCLog::NET, "net"},
    {BCLog::TOR, "tor"},
    {BCLog::MEMPOOL, "mempool"},
    {BCLog::HTTP, "http"},
    {BCLog::BENCH, "bench"},
    {BCLog::ZMQ, "zmq"},
    {BCLog::DB, "db"},
    {BCLog::RPC, "rpc"},
    {BCLog::ESTIMATEFEE, "estimatefee"},
    {BCLog::ADDRMAN, "addrman"},
    {BCLog::SELECTCOINS, "selectcoins"},
    {BCLog::REINDEX, "reindex"},
    {BCLog::CMPCTBLOCK, "cmpctblock"},
    {BCLog::RANDOM, "rand"},
    {BCLog::PRUNE, "prune"},
    {BCLog::PROXY, "proxy"},
    {BCLog::MEMPOOLREJ, "mempoolrej"},
    {BCLog::LIBEVENT, "libevent"},
    {BCLog::COINDB, "coindb"},
    {BCLog::QT, "qt"},
    {BCLog::LEVELDB, "leveldb"},
    {BCLog::COLD, "coldreward"},
    {BCLog::ALL, "1"},
    {BCLog::ALL, "all"},
};

bool GetLogCategory(BCLog::LogFlags &flag, const std::string &str) {
    if (str == "") {
        flag = BCLog::ALL;
        return true;
    }
    for (const CLogCategoryDesc &category_desc : LogCategories) {
        if (category_desc.category == str) {
            flag = category_desc.flag;
            return true;
        }
    }
    return false;
}

std::string ListLogCategories() {
    std::string ret;
    int outcount = 0;
    for (const CLogCategoryDesc &category_desc : LogCategories) {
        // Omit the special cases.
        if (category_desc.flag != BCLog::NONE &&
            category_desc.flag != BCLog::ALL) {
            if (outcount != 0) {
                ret += ", ";
            }
            ret += category_desc.category;
            outcount++;
        }
    }
    return ret;
}

BCLog::Logger::~Logger() {
    if (m_fileout) {
        fclose(m_fileout);
    }
}

std::string BCLog::Logger::LogTimestampStr(const std::string &str) {
    std::string strStamped;

    if (!m_log_timestamps) {
        return str;
    }

    if (m_started_new_line) {
        int64_t nTimeMicros = GetTimeMicros();
        strStamped = FormatISO8601DateTime(nTimeMicros / 1000000);
        if (m_log_time_micros) {
            strStamped.pop_back();
            strStamped += strprintf(".%06dZ", nTimeMicros % 1000000);
        }
        int64_t mocktime = GetMockTime();
        if (mocktime) {
            strStamped +=
                " (mocktime: " + FormatISO8601DateTime(mocktime) + ")";
        }
        strStamped += ' ' + str;
    } else {
        strStamped = str;
    }

    if (!str.empty() && str[str.size() - 1] == '\n') {
        m_started_new_line = true;
    } else {
        m_started_new_line = false;
    }

    return strStamped;
}

void BCLog::Logger::LogPrintStr(const std::string &str) {
    std::string strTimestamped = LogTimestampStr(str);

    if (m_print_to_console) {
        // Print to console.
        fwrite(strTimestamped.data(), 1, strTimestamped.size(), stdout);
        fflush(stdout);
    } else if (m_print_to_file) {
        std::lock_guard<std::mutex> scoped_lock(m_file_mutex);

        // Buffer if we haven't opened the log yet.
        if (m_fileout == nullptr) {
            m_msgs_before_open.push_back(strTimestamped);
        } else {
            // Reopen the log file, if requested.
            if (m_reopen_file) {
                m_reopen_file = false;
                fs::path pathDebug = GetDebugLogPath();
                FILE *new_fileout = fsbridge::fopen(pathDebug, "a");
                if (new_fileout) {
                    // unbuffered.
                    setbuf(m_fileout, nullptr);
                    fclose(m_fileout);
                    m_fileout = new_fileout;
                }
            }
            FileWriteStr(strTimestamped, m_fileout);
        }
    }
}

void BCLog::Logger::ShrinkDebugFile() {
    // Amount of debug.log to save at end when shrinking (must fit in memory)
    constexpr size_t RECENT_DEBUG_HISTORY_SIZE = 10 * 1000000;
    // Scroll debug.log if it's getting too big.
    fs::path pathLog = GetDebugLogPath();
    FILE *file = fsbridge::fopen(pathLog, "r");
    // If debug.log file is more than 10% bigger the RECENT_DEBUG_HISTORY_SIZE
    // trim it down by saving only the last RECENT_DEBUG_HISTORY_SIZE bytes.
    if (file &&
        fs::file_size(pathLog) > 11 * (RECENT_DEBUG_HISTORY_SIZE / 10)) {
        // Restart the file with some of the end.
        std::vector<char> vch(RECENT_DEBUG_HISTORY_SIZE, 0);
        if (fseek(file, -((long)vch.size()), SEEK_END)) {
            LogPrintf("Failed to shrink debug log file: fseek(...) failed\n");
            fclose(file);
            return;
        }
        int nBytes = fread(vch.data(), 1, vch.size(), file);
        fclose(file);

        file = fsbridge::fopen(pathLog, "w");
        if (file) {
            fwrite(vch.data(), 1, nBytes, file);
            fclose(file);
        }
    } else if (file != nullptr) {
        fclose(file);
    }
}
std::string BCLog::Logger::RenameLastDebugFile(){
  fs::path pathLog = GetDataDir();
  pathLog /= DEFAULT_DEBUGLOGFILE;
  fs::path oldLog = GetDataDir();
  if (fs::exists(pathLog)) {
    auto last_write_time = fs::last_write_time(pathLog);
#ifdef NO_BOOST_FILESYSTEM
    // This may be implementation dependent but need to divide for Mac OS Catalina's use of filesystem
    auto last_write_int = last_write_time.time_since_epoch().count()/1000000000;
#else
    auto last_write_int = last_write_time; // no change
#endif
    std::string s = "debug-"+FormatDebugLogDateTime(last_write_int)+".log";
    oldLog /= s;
    fs::rename(pathLog, oldLog);
    return oldLog.string();
  }
  return "";
}

//! Remove debug.log files older than 1 day unless "keeplogfiles" is specified
//  then keep up to days specified by argument with a minimum of 1
void BCLog::Logger::RemoveOlderDebugFiles() {
    int days_to_keep = gArgs.GetArg("-keeplogfiles", DEFAULT_DEBUGLOGFILE_KEEPDAYS);
    days_to_keep = std::max(1, days_to_keep);
    fs::directory_iterator dir_it(GetDataDir());
    for(const auto& it : dir_it) {
        if (!fs::is_regular_file(it.status())) continue;
        std::string filename = it.path().filename().string();
        std::size_t found_log = filename.find(".log");
        std::size_t found_debug = filename.find("debug");
        if (found_log !=std::string::npos && found_debug != std::string::npos) {
            auto last_write_time = fs::last_write_time(it.path());
            std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef NO_BOOST_FILESYSTEM
          auto last_write_int = last_write_time.time_since_epoch().count();
#else
          auto last_write_int = last_write_time;
#endif
            if ((cftime - last_write_int) > (60*60*24*days_to_keep)) {
                LogPrintf("Removing %s since older than %d day\n",it.path().filename().string(),days_to_keep);
                fs::remove(it.path());
            }
        }
    }
} 

void BCLog::Logger::EnableCategory(LogFlags category) {
    m_categories |= category;
}

bool BCLog::Logger::EnableCategory(const std::string &str) {
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) {
        return false;
    }
    EnableCategory(flag);
    return true;
}

void BCLog::Logger::DisableCategory(LogFlags category) {
    m_categories &= ~category;
}

bool BCLog::Logger::DisableCategory(const std::string &str) {
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) {
        return false;
    }
    DisableCategory(flag);
    return true;
}

bool BCLog::Logger::WillLogCategory(LogFlags category) const {
    // ALL is not meant to be used as a logging category, but only as a mask
    // representing all categories.
    if (category == BCLog::NONE || category == BCLog::ALL) {
        LogPrintf("Error trying to log using a category mask instead of an "
                  "explicit category.\n");
        return true;
    }

    return (m_categories.load(std::memory_order_relaxed) & category) != 0;
}

bool BCLog::Logger::DefaultShrinkDebugFile() const {
    return m_categories != BCLog::NONE;
}
