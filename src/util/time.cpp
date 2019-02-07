// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h>

#include <cassert>
#include <util/time.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include <tinyformat.h>

#include <atomic>
#include <sstream>
#include <iomanip> // for put_time
#include <chrono>
#include <thread>
#include <tinyformat.h>

//!< For unit testing
static std::atomic<int64_t> nMockTime(0);

int64_t GetTime() {
    int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
    if (mocktime) {
        return mocktime;
    }

    time_t now = time(nullptr);
    assert(now > 0);
    return now;
}

void SetMockTime(int64_t nMockTimeIn) {
    nMockTime.store(nMockTimeIn, std::memory_order_relaxed);
}

int64_t GetMockTime() {
    return nMockTime.load(std::memory_order_relaxed);
}

int64_t GetTimeMillis() {
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return millis;
}

int64_t GetTimeMicros() {
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  return micros;
}

int64_t GetSystemTimeInSeconds() {
    return GetTimeMicros() / 1000000;
}

void MilliSleep(int64_t n) { std::this_thread::sleep_for(std::chrono::milliseconds(n)); }

std::string DateTimeStrFormat(const char *pszFormat, int64_t nTime) {
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(nTime);
  std::time_t ttp = std::chrono::system_clock::to_time_t(tp);
  static std::locale classic(std::locale::classic());
  // std::locale takes ownership of the pointer
  // std::locale loc(classic, new boost::posix_time::time_facet(pszFormat));
  std::stringstream ss;
  ss.imbue(classic);  // was loc.
  ss << std::put_time(std::gmtime(&ttp), pszFormat);
  return ss.str();
}

// Custom format since Windows doesn't like ":" in filenames
// Specifically for renaming debug.log fiels
std::string FormatDebugLogDateTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H-%M-%SZ", nTime);
}


std::string FormatISO8601DateTime(int64_t nTime) {
    struct tm ts;
    time_t time_val = nTime;
#ifdef _MSC_VER
    gmtime_s(&ts, &time_val);
#else
    gmtime_r(&time_val, &ts);
#endif
    return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900,
                     ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min,
                     ts.tm_sec);
}

std::string FormatISO8601Date(int64_t nTime) {
    struct tm ts;
    time_t time_val = nTime;
#ifdef _MSC_VER
    gmtime_s(&ts, &time_val);
#else
    gmtime_r(&time_val, &ts);
#endif
    return strprintf("%04i-%02i-%02i", ts.tm_year + 1900, ts.tm_mon + 1,
                     ts.tm_mday);
}
