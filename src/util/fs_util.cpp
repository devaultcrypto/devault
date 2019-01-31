// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h>
#include <config/version.h>

#include <util/fs_util.h>
#include <util/system.h>
#include <chainparamsbase.h>
#include <ui_interface.h>
#include <random.h>
#include <boost/interprocess/sync/file_lock.hpp>

#ifndef WIN32
// for posix_fallocate
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__
#include <cstring>
#include <algorithm>
#include <fcntl.h>
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
#include <shlobj.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

const char *const BITCOIN_PID_FILENAME = "devaultd.pid";

fs::path GetDebugLogPath() {
    fs::path logfile(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    if (logfile.is_absolute()) {
        return logfile;
    } else {
        return GetDataDir() / logfile;
    }
}

/** Default name for auth cookie file */
static const std::string COOKIEAUTH_FILE = ".cookie";

fs::path GetAuthCookieFile() {
    fs::path path(gArgs.GetArg("-rpccookiefile", COOKIEAUTH_FILE));
    if (!path.is_absolute()) {
        path = GetDataDir() / path;
    }
    return path;
}


fs::path GetDefaultDataDir() {
// Windows < Vista: C:\Documents and Settings\Username\Application Data\Bitcoin
// Windows >= Vista: C:\Users\Username\AppData\Roaming\Bitcoin
// Mac: ~/Library/Application Support/Bitcoin
// Unix: ~/.bitcoin
#ifdef WIN32
    // Windows
    return GetSpecialFolderPath(CSIDL_APPDATA) / "DeVault";
#else
    fs::path pathRet;
    char *pszHome = getenv("HOME");
    if (pszHome == nullptr || strlen(pszHome) == 0) {
        pathRet = fs::path("/");
    } else {
        pathRet = fs::path(pszHome);
    }
#ifdef MAC_OSX
    // Mac
    return pathRet / "Library/Application Support/DeVault";
#else
    // Unix
    return pathRet / ".devault";
#endif
#endif
}

static fs::path pathCached;
static fs::path pathCachedNetSpecific;
static CCriticalSection csPathCached;
static fs::path g_blocks_path_cached;
static fs::path g_blocks_path_cache_net_specific;

bool CheckIfWalletDatExists(bool fNetSpecific) {
  fs::path path;
  if (gArgs.IsArgSet("-datadir")) {
    fs::path p = (gArgs.GetArg("-datadir", ""));
    path = fs::absolute(p);
    if (!fs::is_directory(path)) {
      return false;
    }
  } else {
    path = GetDefaultDataDir();
  }
  
  if (fNetSpecific) {
    path /= BaseParams().DataDir();
  }
  
  path /= "wallets";
  path /= "wallet.dat";
  return fs::exists(path);
}

bool DirIsWritable(const fs::path &directory) {
    std::string hexrandom = "rand" + GetRandString(16);
    fs::path tmpFile = directory / hexrandom;

    FILE *file = fsbridge::fopen(tmpFile, "a");
    if (!file) {
        return false;
    }

    fclose(file);
    remove(tmpFile);

    return true;
}

const fs::path &GetDataDir(bool fNetSpecific) {
    LOCK(csPathCached);

    fs::path &path = fNetSpecific ? pathCachedNetSpecific : pathCached;

    // This can be called during exceptions by LogPrintf(), so we cache the
    // value so we don't have to do memory allocations after that.
    if (!path.empty()) {
        return path;
    }

    if (gArgs.IsArgSet("-datadir")) {
        fs::path p = (gArgs.GetArg("-datadir", ""));
        path = fs::absolute(p);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }

    if (fNetSpecific) {
        path /= BaseParams().DataDir();
    }

    fs::create_directories(path);
    // Make sure this wallets subdirectory exists too
    fs::create_directories(path / "wallets");

    return path;
}

const fs::path GetDataDirNoCreate() {
  // copy instead of reference
  fs::path path = pathCachedNetSpecific;
  
  // This can be called during exceptions by LogPrintf(), so we cache the
  // value so we don't have to do memory allocations after that.
  if (!path.empty()) {
    return path;
  }
  
  if (gArgs.IsArgSet("-datadir")) {
    fs::path p = (gArgs.GetArg("-datadir", ""));
    path = fs::absolute(p);
    if (!fs::is_directory(path)) {
      path = "";
      return path;
    }
  } else {
    path = GetDefaultDataDir();
  }
  return path;
}

void ClearDatadirCache() {
    LOCK(csPathCached);

    pathCached = fs::path();
    pathCachedNetSpecific = fs::path();
}

fs::path GetConfigFile(const std::string &confPath) {
    fs::path pathConfigFile(confPath);
    if (!pathConfigFile.is_absolute()) {
        pathConfigFile = GetDataDir(false) / pathConfigFile;
    }

    return pathConfigFile;
}

#ifndef WIN32
fs::path GetPidFile() {
    fs::path pathPidFile(gArgs.GetArg("-pid", BITCOIN_PID_FILENAME));
    if (!pathPidFile.is_absolute()) {
        pathPidFile = GetDataDir() / pathPidFile;
    }
    return pathPidFile;
}

void CreatePidFile(const fs::path &path, pid_t pid) {
    FILE *file = fsbridge::fopen(path, "w");
    if (file) {
        fprintf(file, "%d\n", pid);
        fclose(file);
    }
}
#endif

bool RenameOver(const fs::path& src, const fs::path& dest) {
#ifdef WIN32
    return MoveFileExA(src.string().c_str(), dest.string().c_str(),
                       MOVEFILE_REPLACE_EXISTING) != 0;
#else
    int rc = std::rename(src.string().c_str(), dest.string().c_str());
    return (rc == 0);
#endif /* WIN32 */
}

/**
 * Ignores exceptions thrown by Boost's create_directories if the requested
 * directory exists. Specifically handles case where path p exists, but it
 * wasn't possible for the user to write to the parent directory.
 */
bool TryCreateDirectories(const fs::path &p) {
    try {
        return fs::create_directories(p);
    } catch (const fs::filesystem_error &) {
        if (!fs::exists(p) || !fs::is_directory(p)) {
            throw;
        }
    }

    // create_directory didn't create the directory, it had to have existed
    // already.
    return false;
}

bool FileCommit(FILE *file) {
    // Harmless if redundantly called.
    if (fflush(file) != 0) {
        LogPrintf("%s: fflush failed: %d\n", __func__, errno);
        return false;
    }
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    if (FlushFileBuffers(hFile) == 0) {
        LogPrintf("%s: FlushFileBuffers failed: %d\n", __func__,GetLastError());
        return false;
    }
#else
#if defined(__linux__) || defined(__NetBSD__)
    // Ignore EINVAL for filesystems that don't support sync
    if (fdatasync(fileno(file)) != 0 && errno != EINVAL) {
        LogPrintf("%s: fdatasync failed: %d\n", __func__, errno);
        return false;
    }
#elif defined(__APPLE__) && defined(F_FULLFSYNC)
    // Manpage says "value other than -1" is returned on success
    if (fcntl(fileno(file), F_FULLFSYNC, 0) == -1) {
        LogPrintf("%s: fcntl F_FULLFSYNC failed: %d\n", __func__, errno);
        return false;
    }
#else
    if (fsync(fileno(file)) != 0 && errno != EINVAL) {
        LogPrintf("%s: fsync failed: %d\n", __func__, errno);
        return false;
    }
#endif
#endif
    return true;
}

bool TruncateFile(FILE *file, unsigned int length) {
#if defined(WIN32)
    return _chsize(_fileno(file), length) == 0;
#else
    return ftruncate(fileno(file), length) == 0;
#endif
}

/**
 * This function tries to make a particular range of a file allocated
 * (corresponding to disk space) it is advisory, and the range specified in the
 * arguments will never contain live data.
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
#if defined(WIN32)
    // Windows-specific version.
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    LARGE_INTEGER nFileSize;
    int64_t nEndPos = (int64_t)offset + length;
    nFileSize.u.LowPart = nEndPos & 0xFFFFFFFF;
    nFileSize.u.HighPart = nEndPos >> 32;
    SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
#elif defined(MAC_OSX)
    // OSX specific version.
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = (off_t)offset + length;
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), fst.fst_length);
#elif defined(__linux__)
    // Version using posix_fallocate.
    off_t nEndPos = (off_t)offset + length;
    posix_fallocate(fileno(file), 0, nEndPos);
#else
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    fseek(file, offset, SEEK_SET);
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now) {
            now = length;
        }
        // Allowed to fail; this function is advisory anyway.
        fwrite(buf, 1, now, file);
        length -= now;
    }
#endif
}

#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate) {
    char pszPath[MAX_PATH] = "";

    if (SHGetSpecialFolderPathA(nullptr, pszPath, nFolder, fCreate)) {
        return fs::path(pszPath);
    }

    LogPrintf(
        "SHGetSpecialFolderPathA() failed, could not obtain requested path.\n");
    return fs::path("");
}
#endif

void SetupEnvironment() {
#ifdef HAVE_MALLOPT_ARENA_MAX
    // glibc-specific: On 32-bit systems set the number of arenas to 1. By
    // default, since glibc 2.10, the C library will create up to two heap
    // arenas per core. This is known to cause excessive virtual address space
    // usage in our usage. Work around it by setting the maximum number of
    // arenas to 1.
    if (sizeof(void *) == 4) {
        mallopt(M_ARENA_MAX, 1);
    }
#endif
// On most POSIX systems (e.g. Linux, but not BSD) the environment's locale may
// be invalid, in which case the "C" locale is used as fallback.
#if !defined(WIN32) && !defined(MAC_OSX) && !defined(__FreeBSD__) &&           \
    !defined(__OpenBSD__)
    try {
        // Raises a runtime error if current locale is invalid.
        std::locale("");
    } catch (const std::runtime_error &) {
        setenv("LC_ALL", "C", 1);
    }
#endif
    // The path locale is lazy initialized and to avoid deinitialization errors
    // in multithreading environments, it is set explicitly by the main thread.
    // A dummy locale is used to extract the internal default locale, used by
    // fs::path, which is then used to explicitly imbue the path.
#ifndef NO_BOOST_FILESYSTEM    
    std::locale loc = fs::path::imbue(std::locale::classic());
#ifndef WIN32
    fs::path::imbue(loc);
#else
    fs::path::imbue(std::locale(loc, new std::codecvt_utf8_utf16<wchar_t>()));
#endif
#endif
}



/**
 * A map that contains all the currently held directory locks. After successful
 * locking, these will be held here until the global destructor cleans them up
 * and thus automatically unlocks them, or ReleaseDirectoryLocks is called.
 */
static std::map<std::string, std::unique_ptr<boost::interprocess::file_lock>>
    dir_locks;
/** Mutex to protect dir_locks. */
static std::mutex cs_dir_locks;

bool LockDirectory(const fs::path &directory, const std::string lockfile_name,
                   bool probe_only) {
    std::lock_guard<std::mutex> ulock(cs_dir_locks);
    fs::path pathLockFile = directory / lockfile_name;

    // If a lock for this directory already exists in the map, don't try to
    // re-lock it
    if (dir_locks.count(pathLockFile.string())) {
        return true;
    }

    // Create empty lock file if it doesn't exist.
    FILE *file = fsbridge::fopen(pathLockFile, "a");
    if (file) {
        fclose(file);
    }

    try {
        auto lock = std::make_unique<boost::interprocess::file_lock>(
            pathLockFile.string().c_str());
        if (!lock->try_lock()) {
            return false;
        }
        if (!probe_only) {
            // Lock successful and we're not just probing, put it into the map
            dir_locks.emplace(pathLockFile.string(), std::move(lock));
        }
    } catch (const boost::interprocess::interprocess_exception &e) {
        return error("Error while attempting to lock directory %s: %s",
                     directory.string(), e.what());
    }
    return true;
}

void ReleaseDirectoryLocks() {
    std::lock_guard<std::mutex> ulock(cs_dir_locks);
    dir_locks.clear();
}

bool LockDataDirectory(bool probeOnly) {
    std::string strDataDir = GetDataDir().string();

    // Make sure only a single process is using the data directory.
    fs::path pathLockFile = GetDataDir() / ".lock";

    if (!DirIsWritable(GetDataDir())) {
        return InitError(strprintf(
                                   _("Cannot write to data directory '%s'; check permissions."),
                                   strDataDir));
    }
 
    
    // empty lock file; created if it doesn't exist.
    FILE *file = fsbridge::fopen(pathLockFile, "a");
    if (file) {
        fclose(file);
    }

    try {
        static boost::interprocess::file_lock lock(
            pathLockFile.string().c_str());
        if (!lock.try_lock()) {
            return InitError(
                strprintf(_("Cannot obtain a lock on data directory %s. DeVault Core is probably already running."),
                          strDataDir));
        }
        if (probeOnly) {
            lock.unlock();
        }
    } catch (const boost::interprocess::interprocess_exception &e) {
        return InitError(strprintf(_("Cannot obtain a lock on data directory "
                                     "%s. DeVault Core is probably already running.") +
                                       " %s.",
                                   strDataDir, e.what()));
    }
    return true;
}


const fs::path &GetBlocksDir(bool fNetSpecific) {

    LOCK(csPathCached);

    fs::path &path =
        fNetSpecific ? g_blocks_path_cache_net_specific : g_blocks_path_cached;

    // This can be called during exceptions by LogPrintf(), so we cache the
    // value so we don't have to do memory allocations after that.
    if (!path.empty()) {
        return path;
    }

    if (gArgs.IsArgSet("-blocksdir")) {
        fs::path p = gArgs.GetArg("-blocksdir", "");
        path = fs::absolute(p);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDataDir(false);
    }
    if (fNetSpecific) {
        path /= BaseParams().DataDir();
    }
#ifdef USE_ROCKSDB
    path /= "rocksdb";
#endif
    path /= "blocks";
    fs::create_directories(path);
    return path;
}


void UnlockDirectory(const fs::path &directory,
                     const std::string &lockfile_name) {
    std::lock_guard<std::mutex> lock(cs_dir_locks);
    dir_locks.erase((directory / lockfile_name).string());
}

bool CheckDiskSpace(uint64_t nAdditionalBytes, bool blocks_dir) {

    constexpr uint64_t nMinDiskSpace = 52428800;
    // Below version was causing crashes in unit_tests, so skip using blocks_dir
    // uint64_t nFreeBytesAvailable = fs::space(blocks_dir ? GetBlocksDir() : GetDataDir()).available;

    uint64_t nFreeBytesAvailable = fs::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < nMinDiskSpace + nAdditionalBytes) {
        return false;
    }

    return true;
}

fs::path AbsPathForConfigVal(const fs::path &path, bool net_specific) {
#ifdef NO_BOOST_FILESYSTEM
    return GetDataDir(net_specific) / path;
#else
    return fs::absolute(path, GetDataDir(net_specific));
#endif     
}
