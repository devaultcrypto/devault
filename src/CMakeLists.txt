# Copyright (c) 2017 The Bitcoin developers

cmake_minimum_required(VERSION 3.5)
project(DeVault)

# Default visibility is hidden on all targets.
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(FeatureSummary)

# Put all generated binaries into the top of Binary Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")


option(LINK_STATIC_LIBS "Use static linking" OFF)
option(BUILD_WALLET "Activate the wallet functionality" ON)
option(BUILD_WALLETTOOL "Build the wallet tool" OFF)
option(BUILD_ZMQ "Activate the ZeroMQ functionalities" OFF)
option(BUILD_SEEDER "Build devault-seeder" OFF)
option(BUILD_CLI "Build devault-cli" ON)
option(BUILD_TX "Build devault-tx" ON)
option(BUILD_QT "Build devault-qt" ON)
option(BUILD_BENCH "Build benchmark code" OFF)
option(BUILD_CTESTS "Build cmake unit tests" ON)
option(BUILD_ROCKSDB "Use Rocksdb instead of LevelDb" OFF)
option(ENABLE_HARDENING "Harden the executables" OFF)
option(BUILD_BUNDLE "For MacOS build MacOS Application" OFF)
option(ENABLE_REDUCE_EXPORTS "Reduce the amount of exported symbols" OFF)
option(ENABLE_QRCODE "Enable QR code display" ON)
option(BUILD_STD_FILESYSTEM "Enable trying to use std::filesystem" OFF)
option(ENABLE_UPNP "Enable UPnP support" ON)
option(START_WITH_UPNP "Make UPnP the default to map ports" OFF)

add_feature_info(WITH_WALLETTOOL BUILD_WALLETTOOL "Build bdb-check for checking wallet")
add_feature_info(WITH_WALLET BUILD_WALLET "Activate the wallet functionality")
add_feature_info(WITH_ZMQ BUILD_ZMQ "Activate the ZeroMQ functionalities")
add_feature_info(WITH_SEEDER BUILD_SEEDER "Build devault-seeder")
add_feature_info(WITH_CLI BUILD_CLI "Build devault-cli")
add_feature_info(WITH_TX BUILD_TX "Build devault-tx")
add_feature_info(WITH_QT BUILD_QT "Build devault-qt")
add_feature_info(WITH_ROCKSDB BUILD_ROCKSDB "Build with RocksDB (blockchain restart needed if switched" )
add_feature_info(WITH_HARD ENABLE_HARDENING "Harden the executables")
add_feature_info(WITH_STATIC LINK_STATIC_LIBS "Build with static linking")
add_feature_info(WITH_BENCH BUILD_BENCH "Build benchmark code")
add_feature_info(WITH_CTESTS BUILD_CTESTS "Build cmake unit tests")
add_feature_info(WITH_BUNDLE BUILD_BUNDLE "For MacOS build MacOS Application")
add_feature_info(WITH_REDUCED_EXPORTS ENABLE_REDUCE_EXPORTS "Reduce amount of exported symbols")
add_feature_info(WITH_QRCODE ENABLE_QRCODE "Enable QR code display")
add_feature_info(WITH_STD_FILESYSTEM BUILD_STD_FILESYSTEM "Enable trying to use std::filesystem")

set(CMAKE_REQUIRED_QUIET true)

if (LINK_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
  add_linker_flag(-static-libstdc++)
endif()

# Allow usage of sanitizers by setting ECM_ENABLE_SANITIZERS
if(ENABLE_SANITIZERS)
	set(ECM_ENABLE_SANITIZERS ${ENABLE_SANITIZERS})
	find_package(ECM NO_MODULE)
	if(ECM_MODULE_PATH)
		list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
		include(ECMEnableSanitizers)
	else()
		message(FATAL_ERROR
			"ECM is required to enable the sanitizers (https://api.kde.org/ecm/index.html)"
		)
	endif()
endif()

# Cmake uses the CMAKE_BUILD_TYPE variable to select the build configuration.
# By default it supports more configurations that needed for DeVault, and
# all the releases types set NDEBUG which is unwanted as it disables the assert
# completely.
# Remove the -DNDEBUG flag from the CFLAGS/CXXFLAGS in all the configurations
include(AddCompilerFlags)
remove_compiler_flags(-DNDEBUG)

# Overrides the flags for the Debug build type
# This mimics the autotools behavior by setting the CFLAGS to '-g -O2`, which
# are not well suited for debugging.
# FIXME: update CFLAGS with better debug oriented optimization flags
#set(CMAKE_C_FLAGS_DEBUG "-g -O2")

# Prefer -g3, defaults to -g if unavailable
#dd_cxx_compiler_flag_with_fallback(CMAKE_CXX_FLAGS_DEBUG -g3 -g)

# Prefer -Og, defaults to -O0 if unavailable
#add_cxx_compiler_flag_with_fallback(CMAKE_CXX_FLAGS_DEBUG -Og -O0)

# Define the debugging symbols DEBUG and DEBUG_LOCKORDER when the Debug build
# type is selected.
#string(APPEND CMAKE_CXX_FLAGS_DEBUG " -DDEBUG -DDEBUG_LOCKORDER")

# Ensure that WINDRES_PREPROC is enabled when using windres.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# Ensure that WINDRES_PREPROC is enabled when using windres.
	list(APPEND CMAKE_RC_FLAGS "-DWINDRES_PREPROC")

	# Build all static so there is no dll file to distribute.
	add_compiler_flags(-static)
endif()

if(ENABLE_REDUCE_EXPORTS)
	# Default visibility is set by CMAKE_<LANG>_VISIBILITY_PRESET, but this
	# doesn't tell if the visibility set is effective.
	# Check if the flag -fvisibility=hidden is supported, as using the hidden
	# visibility is a requirement to reduce exports.
	check_compiler_flag(HAS_CXX_FVISIBILITY CXX -fvisibility=hidden)
	if(NOT HAS_CXX_FVISIBILITY)
		message(FATAL_ERROR "Cannot set default symbol visibility. Use -DENABLE_REDUCE_EXPORTS=OFF.")
	endif()

	# Also hide symbols from static libraries
	add_linker_flags(-Wl,--exclude-libs,ALL)
endif()

# Enable statically linking libstdc++
if(ENABLE_STATIC_LIBSTDCXX)
	add_linker_flags(-static-libstdc++)
endif()

# All windows code is PIC, forcing it on just adds useless compile warnings
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for all targets
endif()
	
if(ENABLE_HARDENING)
	# Enable stack protection
	add_cxx_compiler_flags(-fstack-protector-all -Wstack-protector)

	# Enable some buffer overflow checking
	add_compiler_flags(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2)

	# Enable ASLR (these flags are primarily targeting MinGw)
	add_linker_flags(-Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va)

	# Make the relocated sections read-only
	add_linker_flags(-Wl,-z,relro -Wl,-z,now)

	#add_compiler_flags(-fPIE)
	#add_linker_flags(-pie)

	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		# MinGw provides its own libssp for stack smashing protection
		link_libraries(ssp)
	endif()
endif()

# Enable warning
add_c_compiler_flags(-Wnested-externs -Wstrict-prototypes)
add_compiler_flags(
	-Wall
	-Wextra
	-Wformat
	-Wvla
	-Wformat-security
	-Wcast-align
	-Wunused-parameter
	-Wmissing-braces
	-Wthread-safety-analysis
	-Wshadow
	-Wrange-loop-analysis
	-Wredundant-decls
	-Wredundant-move
)

option(EXTRA_WARNINGS "Enable extra warnings" OFF)
if(EXTRA_WARNINGS)
	add_cxx_compiler_flags(-Wsuggest-override)
else()
	add_compiler_flags(-Wno-unused-parameter)
	add_compiler_flags(-Wno-implicit-fallthrough)
endif()

add_feature_info(WITH_WARNINGS EXTRA_WARNINGS "Enable extra warnings")


check_cxx_compiler_flag(-std=c++17 HAVE_FLAG_STD_CXX17)
if (HAVE_FLAG_STD_CXX17)
  set(CMAKE_CXX_STANDARD 17)
  message(STATUS "Compiler supports C++17")
  add_definitions("-D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR") # still needed?
else()
  message(ERROR "Compiler must at least support C++17")
endif()

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_compiler_flags(-Wrange-loop-analysis)
  # For 17 we need BOOST_NO_AUTO_PTR defined for boost
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\1" DARWIN_MAJOR_VERSION "${CMAKE_SYSTEM_VERSION}")
  message(STATUS "Darwin Version : ${CMAKE_SYSTEM_VERSION} found, major version ${DARWIN_MAJOR_VERSION} ")
  # HAVE_VARIANT means support for std::variant and std::optional
  if (DARWIN_MAJOR_VERSION GREATER 18) # 18 is Mojave
    message(STATUS "Will use std c++ filesystem since > Mojave")
    set(BUILD_STD_FILESYSTEM ON)
  endif()
  if (DARWIN_MAJOR_VERSION GREATER 17) # 17 is High Sierra
    add_definitions("-DOS_MACOSX -DBOOST_NO_AUTO_PTR -DHAVE_STD_SHARED_MUTEX -DHAVE_VARIANT")
  else()
    add_definitions("-DOS_MACOSX -DBOOST_NO_AUTO_PTR")
  endif()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions("-DBOOST_NO_AUTO_PTR -DHAVE_STD_SHARED_MUTEX -DHAVE_VARIANT")
  add_compiler_flags(-Wno-cpp -Wno-unused-function -Wno-nonnull-compare -Wno-implicit-fallthrough)
else()
  # secp256k1 Linux gcc
  add_compiler_flags(-Wno-cpp -Wno-unused-function -Wno-nonnull-compare -Wno-implicit-fallthrough)
endif()


if (EXISTS	"${CMAKE_CURRENT_SOURCE_DIR}/config/bitcoin-config.h")
  message(FATAL_ERROR ": Please delete ${CMAKE_CURRENT_SOURCE_DIR}/config/bitcoin-config.h before continuing")
endif()

if (EXISTS	"${CMAKE_CURRENT_SOURCE_DIR}/config/version.h")
  message(FATAL_ERROR ": Please delete ${CMAKE_CURRENT_SOURCE_DIR}/config/version.h before continuing")
endif()


include(BrewHelper)

find_package(sodium REQUIRED)
if (SODIUM_FOUND)
  message(STATUS "Found libsodium")
  #message(STATUS "Sodium include dir = ${sodium_INCLUDE_DIR}")
endif()


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# libtool style configure
add_subdirectory(config)

# libraries
add_subdirectory(crypto)

# Boost packages required (Later check for and replace with std::filesystem
# but currently not working with any compilers)
if (BUILD_STD_FILESYSTEM)
  find_package(Filesystem)
  if (Filesystem_FOUND)
    message(STATUS "Found std::filesystem , adding define NO_BOOST_FILESYSTEM")
    add_definitions("-DNO_BOOST_FILESYSTEM")
  else()
    set(BOOST_PACKAGES_REQUIRED filesystem system)
  endif()
else()
  set(BOOST_PACKAGES_REQUIRED filesystem system) # system needed if boost < 1.69
endif()
  
# Can change to LevelDB if needed
if (BUILD_ROCKSDB)
  find_package(RocksDB) # REQUIRED)
  if (RocksDB_FOUND)
    message(STATUS "Found RocksDB")
    #message(STATUS "Libs : ${ROCKSDB_LIBRARIES}")
    add_definitions("-DUSE_ROCKSDB")
    set(DB_LIBRARIES ${ROCKSDB_LIBRARIES} ${BZIP2_LIBRARIES})
  else()
    message(STATUS "FAILED to FIND RocksDB")
    add_subdirectory(leveldb)
    set(DB_LIBRARIES leveldb memenv)
    include_directories(${include_directories} leveldb/include)
  endif()
else()
    add_subdirectory(leveldb)
    set(DB_LIBRARIES leveldb memenv)
    include_directories(${include_directories} leveldb/include)
endif()  

add_subdirectory(secp256k1)
add_subdirectory(univalue)

#--------------------------------------------------------------------------------
# DeVault
#--------------------------------------------------------------------------------
add_subdirectory(devault)
#
#--------------------------------------------------------------------------------


file(GLOB PRIMITIVE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/primitives/*.h)
source_group("PrimitiveHeaders" FILES ${PRIMITIVE_HEADERS})

file(GLOB RPC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/rpc/*.h )
source_group("RpcHeaders" FILES ${RPC_HEADERS})

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("SrcHeaders" FILES ${HEADERS})

file(GLOB CON_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/consensus/*.h)
source_group("ConsenusHeaders" FILES ${CON_HEADERS})

file(GLOB POLICY_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/policy/*.h)
source_group("PolicyHeaders" FILES ${POLICY_HEADERS})

file(GLOB SCRIPT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/script/*.h)
source_group("ScriptHeaders" FILES ${SCRIPT_HEADERS})

file(GLOB SUPPORT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/support/*.h ${CMAKE_CURRENT_SOURCE_DIR}/support/allocators/*.h)
source_group("SupportHeaders" FILES ${SUPPORT_HEADERS})

# Because the DeVault source code is disorganised, we
# end up with a bunch of libraries without any aparent
# cohesive structure. This is inherited from Bitcoin Core
# and reflecting this.
# TODO: Improve the structure once cmake is rocking.

# Various completely unrelated features shared by all executables.
add_library(util
  ${SUPPORT_HEADERS}
	chainparamsbase.cpp
	clientversion.cpp
	compat/glibc_sanity.cpp
	compat/glibcxx_sanity.cpp
	fs.cpp
	logging.cpp
	random.cpp # needs libsodium
	rcu.cpp
	rpc/protocol.cpp
	rpc/util.cpp
	rpc/misc.cpp
	support/cleanse.cpp
	support/lockedpool.cpp
	sync.cpp
	threadinterrupt.cpp
	uint256.cpp
	util.cpp # needs libsodium
	fs_util.cpp
	utilmoneystr.cpp
	utilstrencodings.cpp
	utiltime.cpp
    ui_interface.cpp # for InitError in fs_util
	util/bytevectorhash.cpp
)

target_compile_definitions(util PUBLIC HAVE_CONFIG_H)
target_include_directories(util
	PUBLIC
		.
		# To access the config.
		${CMAKE_CURRENT_BINARY_DIR}
    ${sodium_INCLUDE_DIR}
)

# Target specific configs
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_THREADAPI win32)

	find_package(SHLWAPI REQUIRED)
	target_link_libraries(util ${SHLWAPI_LIBRARY})
	target_include_directories(util PUBLIC ${SHLWAPI_INCLUDE_DIR})

	find_library(WS2_32_LIBRARY NAMES ws2_32)
	target_link_libraries(util ${WS2_32_LIBRARY})
	
	target_compile_definitions(util PUBLIC BOOST_THREAD_USE_LIB)
endif()

function(prepend var prefix)
	set(listVar "")
	foreach(f ${ARGN})
		list(APPEND listVar "${prefix}${f}")
	endforeach(f)
	set(${var} "${listVar}" PARENT_SCOPE)
endfunction(prepend)

if (Filesystem_FOUND)
  # Still need Boost header files, but no linkage
  find_package(Boost 1.69) # bump up requirement to 1.69 so boost::system is not needed
  target_link_libraries(util univalue crypto sodium std::filesystem)
  # can probably be more selective than this, but needed for now
  include_directories($include_directories "${Boost_INCLUDE_DIR}")
else()
  prepend(BOOST_LIBRARIES "Boost::" ${BOOST_PACKAGES_REQUIRED})
  find_package(Boost 1.65 REQUIRED COMPONENTS ${BOOST_PACKAGES_REQUIRED})
  target_link_libraries(util univalue crypto sodium ${BOOST_LIBRARIES})
endif()


# Make sure boost uses std::atomic (it doesn't before 1.63)
target_compile_definitions(util PUBLIC BOOST_SP_USE_STD_ATOMIC BOOST_AC_USE_STD_ATOMIC)

# More completely unrelated features shared by all executables.
# Because nothing says this is different from util than "common"
add_library(common
  ${HEADERS}
	cashaddr.cpp
	cashaddrenc.cpp
	chainparams.cpp
	config.cpp
	consensus/merkle.cpp
	coins.cpp
	compressor.cpp
	dstencode.cpp
	feerate.cpp
	globals.cpp
	core_read.cpp
	core_write.cpp
	key.cpp
	keystore.cpp
	netaddress.cpp
	netbase.cpp
	primitives/block.cpp
	protocol.cpp
	scheduler.cpp
	script/sign.cpp
	script/standard.cpp
	warnings.cpp
)

target_link_libraries(common util secp256k1)

# libdevaultconsensus
add_library(devaultconsensus
  ${SCRIPT_HEADERS}
  ${PRIMITIVE_HEADERS}
	arith_uint256.cpp
	hash.cpp
	primitives/transaction.cpp
	pubkey.cpp
	script/devaultconsensus.cpp
	script/bitfield.cpp
	script/interpreter.cpp
	script/script.cpp
	script/script_error.cpp
	script/sigencoding.cpp
	uint256.cpp
	utilstrencodings.cpp
)

target_link_libraries(devaultconsensus common devault)

# Bitcoin server facilities
add_library(server
  ${RPC_HEADERS}
  ${CONSENSUS_HEADERS}
  ${POLICY_HEADERS}
	addrman.cpp
	addrdb.cpp
	avalanche.cpp
	bloom.cpp
	blockencodings.cpp
	blockfilter.cpp
	chain.cpp
	checkpoints.cpp
	config.cpp
	consensus/activation.cpp
	consensus/tx_verify.cpp
	globals.cpp
	httprpc.cpp
	httpserver.cpp
	index/base.cpp
	index/txindex.cpp
	init.cpp
	interfaces/handler.cpp
	interfaces/node.cpp
	dbwrapper.cpp
	merkleblock.cpp
	miner.cpp
	net.cpp
	net_processing.cpp
	noui.cpp
	policy/fees.cpp
	policy/policy.cpp
	pow.cpp
	rest.cpp
	rpc/abc.cpp
	rpc/blockchain.cpp
	rpc/command.cpp
	rpc/jsonrpcrequest.cpp
	rpc/mining.cpp
	rpc/net.cpp
	rpc/rawtransaction.cpp
	rpc/server.cpp
	script/scriptcache.cpp
	script/sigcache.cpp
	script/ismine.cpp
	timedata.cpp
	torcontrol.cpp
	txdb.cpp
	txmempool.cpp
    upgrade_check.cpp
    utxo_functions.cpp
	validation.cpp
	validationinterface.cpp
)

# This require libevent
find_package(Event REQUIRED)
find_package(Miniupnpc REQUIRED)

if (NOT RocksDB_FOUND)
  target_include_directories(server PRIVATE leveldb/helpers/memenv)
endif()

target_link_libraries(server
	${EVENT_LIBRARY}
	${EVENT_PTHREAD_LIBRARY}
	devaultconsensus
  ${DB_LIBRARIES}
  ${MINIUPNP_LIBRARY}
	Threads::Threads
)

if(ENABLE_UPNP)
	target_include_directories(server PUBLIC ${MINIUPNPC_INCLUDE_DIR})
	target_link_libraries(server ${MINIUPNPC_LIBRARY})

	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		find_library(IPHLPAPI_LIBRARY NAMES iphlpapi)
		if(NOT IPHLPAPI_LIBRARY)
			message(FATAL_ERROR "Lib iphlpapi is missing")
		endif()
		target_link_libraries(server ${IPHLPAPI_LIBRARY})

		target_compile_definitions(server
			PUBLIC -DSTATICLIB
			PUBLIC -DMINIUPNP_STATICLIB
		)
	endif()
endif()

# Benchmark suite.
if (BUILD_BENCH)
  add_subdirectory(bench)
endif()

if (CMAKE_GENERATOR STREQUAL Ninja)
  message(STATUS "Disabling BUILD_CTESTS since using Ninja")
  SET(BUILD_CTESTS OFF)
endif()


# Wallet
if(BUILD_WALLET)
	add_subdirectory(wallet)
	target_link_libraries(server wallet)
else()
  add_library(nowallet dummywallet.cpp)
  target_include_directories(nowallet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}	${CMAKE_CURRENT_BINARY_DIR})
	target_link_libraries(server nowallet)
endif()

# ZeroMQ
if (BUILD_ZMQ)
  find_package(ZeroMQ)
  if (ZeroMQ_FOUND) 
	  add_subdirectory(zmq)
	  target_link_libraries(server zmq)
  endif()
endif()

# RPC client support
add_library(rpcclient rpc/client.cpp)
target_link_libraries(rpcclient univalue util)

# devault-seeder
if(BUILD_SEEDER)
	add_subdirectory(seeder)
endif()

# bitcoin-cli
if(BUILD_CLI)
	add_executable(devault-cli bitcoin-cli.cpp)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_sources(devault-cli PRIVATE bitcoin-cli-res.rc)
	endif()

	target_link_libraries(devault-cli common rpcclient ${EVENT_LIBRARY})
endif()

# bitcoin-tx
if(BUILD_TX)
	add_executable(devault-tx bitcoin-tx.cpp)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_sources(devault-tx PRIVATE bitcoin-tx-res.rc)
	endif()

	target_link_libraries(devault-tx devaultconsensus)
endif()

# bitcoind
add_executable(devaultd bitcoind.cpp)
target_link_libraries(devaultd server)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_sources(devaultd PRIVATE bitcoind-res.rc)
endif()

# DeVault-Core, only build if both BUILD_WALLET and BUILD_QT are set
if(BUILD_WALLET)
  if(BUILD_QT)
	  add_subdirectory(qt)
  endif()
endif()


if (BUILD_CTESTS)
  add_subdirectory(catch_tests)
endif()


message(STATUS "-----------------------------------------")
if (CMAKE_BUILD_TYPE)
  message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")
  message(STATUS "-----------------------------------------")
endif()

#########################################################################
# print features summary
#########################################################################
message(STATUS "######################################################")
message(STATUS "Enabled features")
feature_summary(WHAT ENABLED_FEATURES)
message(STATUS "######################################################")
message(STATUS "Disabled features")
feature_summary(WHAT DISABLED_FEATURES)
message(STATUS "######################################################")

