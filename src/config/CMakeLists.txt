# Copyright (c) 2017-2019 The Bitcoin developers

# This generates config.h which provides numerous defines
# about the state of the plateform we are building on.

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCXXSourceCompiles)

# Package informations
set(PACKAGE_NAME "DeVault Core")

# Version
set(CLIENT_VERSION_MAJOR 1)
set(CLIENT_VERSION_MINOR 0)
set(CLIENT_VERSION_REVISION 7)
set(CLIENT_VERSION_BUILD 0)

set(VERSION_MAJOR ${CLIENT_VERSION_MAJOR} PARENT_SCOPE)
set(VERSION_MINOR ${CLIENT_VERSION_MINOR} PARENT_SCOPE)
set(VERSION_REVISION ${CLIENT_VERSION_REVISION} PARENT_SCOPE)
set(VERSION_BUILD ${CLIENT_VERSION_BUILD} PARENT_SCOPE)

option(CLIENT_VERSION_IS_RELEASE "Build a release version" ON)

# Copyright
set(COPYRIGHT_YEAR 2019)
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_SUBSTITUTION Bitcoin)
string(REPLACE "%s" ${COPYRIGHT_HOLDERS_SUBSTITUTION} COPYRIGHT_HOLDERS_FINAL ${COPYRIGHT_HOLDERS})

# Process info with GIT to determine if this is a tagged release or not
# If not an annotated tags, then BUILD_SUFFIX with the git sha should be set and used
FIND_PROGRAM(GIT git)
IF(GIT)
   EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${GIT} diff --quiet 
    RESULT_VARIABLE GIT_DIRTY
    OUTPUT_VARIABLE GIT_TEMP
    )

  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${GIT} rev-parse HEAD
    OUTPUT_VARIABLE GIT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${GIT} describe --abbrev=0
    RESULT_VARIABLE GIT_HAS_NO_TAG
    OUTPUT_VARIABLE GIT_TEMP
    ERROR_QUIET
    )

  if (GIT_HAS_NO_TAG)
  else()
    EXECUTE_PROCESS(
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMAND ${GIT} describe --abbrev=0
      OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    EXECUTE_PROCESS(
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMAND ${GIT} rev-list -1 ${GIT_TAG}
      OUTPUT_VARIABLE GIT_TAG_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  endif()

  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${GIT} rev-parse --short HEAD 
    OUTPUT_VARIABLE GIT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  if (${GIT_DIRTY})
    SET(BUILD_SUFFIX_DEFINED true)
    SET(BUILD_SUFFIX "${GIT_OUT}-dirty")
    SET(BUILD_DESC "")
		message(STATUS "Neither a tagged nor checked in release ${BUILD_SUFFIX} ")
  elseif (${GIT_TAG_SHA} MATCHES ${GIT_SHA})
		message(STATUS "This is a tagged release ${GIT_TAG}")
    SET(BUILD_DESC ${GIT_TAG})
    SET(BUILD_DESC_DEFINED true)
    SET(BUILD_SUFFIX 0)
  else()
    SET(BUILD_SUFFIX_DEFINED true)
    SET(BUILD_SUFFIX "${GIT_OUT}")
    SET(BUILD_DESC "")
		message(STATUS "Not a tagged release ${GIT_OUT}")
  ENDIF()
ELSE()
		message(ERROR "Need git installed for proper tagging")
ENDIF(GIT)

# For QT
set(VERSION_DESC ${BUILD_DESC} PARENT_SCOPE)
set(VERSION_SUFFIX ${BUILD_SUFFIX} PARENT_SCOPE)


# Generate the version.h file
configure_file(version.h.cmake.in version.h ESCAPE_QUOTES)

# Endianness
check_include_files("endian.h" HAVE_ENDIAN_H)
check_include_files("sys/endian.h" HAVE_SYS_ENDIAN_H)

if(HAVE_ENDIAN_H)
	set(ENDIAN_FILE "endian.h")
elseif(HAVE_SYS_ENDIAN_H)
	set(ENDIAN_FILE "sys/endian.h")
else()
endif()

if(ENDIAN_FILE)
	check_symbol_exists(htole16 ${ENDIAN_FILE} HAVE_DECL_HTOLE16)
	check_symbol_exists(htobe16 ${ENDIAN_FILE} HAVE_DECL_HTOBE16)
	check_symbol_exists(be16toh ${ENDIAN_FILE} HAVE_DECL_BE16TOH)
	check_symbol_exists(le16toh ${ENDIAN_FILE} HAVE_DECL_LE16TOH)
	check_symbol_exists(htobe32 ${ENDIAN_FILE} HAVE_DECL_HTOBE32)
	check_symbol_exists(htole32 ${ENDIAN_FILE} HAVE_DECL_HTOLE32)
	check_symbol_exists(be32toh ${ENDIAN_FILE} HAVE_DECL_BE32TOH)
	check_symbol_exists(le32toh ${ENDIAN_FILE} HAVE_DECL_LE32TOH)
	check_symbol_exists(htobe64 ${ENDIAN_FILE} HAVE_DECL_HTOBE64)
	check_symbol_exists(htole64 ${ENDIAN_FILE} HAVE_DECL_HTOLE64)
	check_symbol_exists(be64toh ${ENDIAN_FILE} HAVE_DECL_BE64TOH)
	check_symbol_exists(le64toh ${ENDIAN_FILE} HAVE_DECL_LE64TOH)
endif()

# Byte swap
check_include_files("byteswap.h" HAVE_BYTESWAP_H)

check_symbol_exists(bswap_16 "byteswap.h" HAVE_DECL_BSWAP_16)
check_symbol_exists(bswap_32 "byteswap.h" HAVE_DECL_BSWAP_32)
check_symbol_exists(bswap_64 "byteswap.h" HAVE_DECL_BSWAP_64)

# sys/select.h and sys/prctl.h headers
check_include_files("sys/select.h" HAVE_SYS_SELECT_H)
check_include_files("sys/prctl.h" HAVE_SYS_PRCTL_H)

# Bitmanip intrinsics
function(check_builtin_exist SYMBOL VARIABLE)
	set(
		SOURCE_FILE
		"${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckBuiltinExists.c"
	)
	set(
		CMAKE_CONFIGURABLE_FILE_CONTENT
		"int main(int argc, char** argv) { (void)argv; return ${SYMBOL}(argc); }\n"
	)
	configure_file(
		"${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
		"${SOURCE_FILE}"
		@ONLY
	)
	if(NOT CMAKE_REQUIRED_QUIET)
		message(STATUS "Looking for ${SYMBOL}")
	endif()
	try_compile(${VARIABLE}
		${CMAKE_BINARY_DIR}
		${SOURCE_FILE}
		OUTPUT_VARIABLE OUTPUT
	)
	if(${VARIABLE})
		if(NOT CMAKE_REQUIRED_QUIET)
			message(STATUS "Looking for ${SYMBOL} - found")
		endif()
		set(${VARIABLE} 1 CACHE INTERNAL "Have symbol ${SYMBOL}" PARENT_SCOPE)
		file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
			"Determining if the ${SYMBOL} "
			"exist passed with the following output:\n"
			"${OUTPUT}\nFile ${SOURCEFILE}:\n"
			"${CMAKE_CONFIGURABLE_FILE_CONTENT}\n")
	else()
		if(NOT CMAKE_REQUIRED_QUIET)
			message(STATUS "Looking for ${SYMBOL} - not found")
		endif()
		set(${VARIABLE} "" CACHE INTERNAL "Have symbol ${SYMBOL}" PARENT_SCOPE)
		file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
			"Determining if the ${SYMBOL} "
			"exist failed with the following output:\n"
			"${OUTPUT}\nFile ${SOURCEFILE}:\n"
			"${CMAKE_CONFIGURABLE_FILE_CONTENT}\n")
	endif()
endfunction()

check_builtin_exist(__builtin_clz HAVE_DECL___BUILTIN_CLZ)
check_builtin_exist(__builtin_clzl HAVE_DECL___BUILTIN_CLZL)
check_builtin_exist(__builtin_clzll HAVE_DECL___BUILTIN_CLZLL)
check_builtin_exist(__builtin_popcount HAVE_DECL___BUILTIN_POPCOUNT)

# Memory management capabilities
check_symbol_exists(M_ARENA_MAX "malloc.h" HAVE_MALLOPT_ARENA_MAX)
check_symbol_exists(malloc_info "malloc.h" HAVE_MALLOC_INFO)

# Various system libraries
check_symbol_exists(strnlen "string.h" HAVE_DECL_STRNLEN)
check_symbol_exists(daemon "unistd.h" HAVE_DECL_DAEMON)

# Check for ways to obtain entropy
check_symbol_exists(getentropy "unistd.h" HAVE_GETENTROPY)
check_symbol_exists(getentropy "sys/random.h" HAVE_GETENTROPY_RAND)

check_cxx_source_compiles("
	#include <unistd.h>  /* for syscall */
	#include <sys/syscall.h>  /* for SYS_getrandom */
	int main() {
		syscall(SYS_getrandom, nullptr, 0, 0);
		return 0;
	}
" HAVE_SYS_GETRANDOM)

check_cxx_source_compiles("
	#include <sys/types.h>
	#include <sys/sysctl.h>
	int main() {
		static const int name[2] = {CTL_KERN, KERN_ARND};
		sysctl(name, 2, nullptr, nullptr, nullptr, 0);
		return 0;
	}
" HAVE_SYSCTL_ARND)

# OpenSSL functionality
set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_CRYPTO_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})
check_symbol_exists(EVP_MD_CTX_new "openssl/evp.h" HAVE_DECL_EVP_MD_CTX_NEW)

# Activate wallet
set(ENABLE_WALLET ${BUILD_WALLET})

# Activate ZeroMQ
set(ENABLE_ZMQ ${BUILD_ZMQ})

# Try to find libqrencode
# Only used in the wallet GUI
if(ENABLE_QRCODE AND BUILD_BITCOIN_WALLET AND BUILD_BITCOIN_QT)
	find_package(QREncode REQUIRED)
	set(USE_QRCODE 1 CACHE INTERNAL "QR code is enabled")
endif()

# Try to find miniupnpc
if(ENABLE_UPNP)
	find_package(Miniupnpc REQUIRED)

	# The expected behavior is as follow:
	#  - If UPnP is enabled USE_UPNP must be defined
	#  - If UPnP should be the default port map method, USE_UPNP should be
	#    defined to 1, otherwise it should be defined to 0.
	set(USE_UPNP ${START_WITH_UPNP} CACHE INTERNAL "UPnP is enabled")
endif()

# Generate the config
configure_file(bitcoin-config.h.cmake.in bitcoin-config.h ESCAPE_QUOTES)
