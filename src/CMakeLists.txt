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
option(BUILD_ZMQ "Activate the ZeroMQ functionalities" ON)
option(BUILD_SEEDER "Build devault-seeder" OFF)
option(BUILD_CLI "Build devault-cli" ON)
option(BUILD_TX "Build devault-tx" ON)
option(BUILD_QT "Build devault-qt" ON)
option(BUILD_TEST_BENCH "Build benchmark and test code" OFF)
option(BUILD_ROCKSDB "Use Rocksdb instead of LevelDb" ON)
option(ENABLE_HARDENING "Harden the executables" OFF)

add_feature_info(WITH_WALLET BUILD_WALLET "Activate the wallet functionality")
add_feature_info(WITH_ZMQ BUILD_ZMQ "Activate the ZeroMQ functionalities")
add_feature_info(WITH_SEEDER BUILD_SEEDER "Build devault-seeder")
add_feature_info(WITH_CLI BUILD_CLI "Build devault-cli")
add_feature_info(WITH_TX BUILD_TX "Build devault-tx")
add_feature_info(WITH_QT BUILD_QT "Build devault-qt")
add_feature_info(WITH_ROCKSDB BUILD_ROCKSDB "Build with RocksDB (blockchain restart needed if switched" )
add_feature_info(WITH_HARD ENABLE_HARDENING "Harden the executables")
add_feature_info(WITH_STATIC LINK_STATIC_LIBS "Build with static linking")
add_feature_info(WITH_BENCH BUILD_TEST_BENCH "Build benchmark and test code")

set(CMAKE_REQUIRED_QUIET true)

if (LINK_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
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
	add_compiler_flag(-static)
endif()

# All windows code is PIC, forcing it on just adds useless compile warnings
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for all targets
	#add_compiler_flag(-fPIC)
endif()
	
if(ENABLE_HARDENING)
	# Enable stack protection
	add_cxx_compiler_flag(-fstack-protector-all -Wstack-protector)
	
	# Enable some buffer overflow checking
	add_compiler_flag(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2)
	
	# Enable ASLR (these flags are primarily targeting MinGw)
	add_linker_flag(-Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va)
	
	# Make the relocated sections read-only
	add_linker_flag(-Wl,-z,relro -Wl,-z,now)

	# CMake provides the POSITION_INDEPENDENT_CODE property to set PIC/PIE.
	# Unfortunately setting the -pie linker flag this way require CMake >= 3.14,
	# which is not widely distributed at the time of writing.
	# FIXME: use the POSITION_INDEPENDENT_CODE property instead
	if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for all targets
	else()
		# MinGw provides its own libssp for stack smashing protection
		link_libraries(ssp)
	endif()
endif()

# Enable warning
add_c_compiler_flag(-Wnested-externs -Wstrict-prototypes)
add_compiler_flag(
	-Wall
	-Wextra
	-Wformat
	-Wvla
	-Wformat-security
	-Wcast-align
	-Wunused-parameter
	-Wmissing-braces
	# FIXME: Activating this flag cause cmake to fail on leveldb.
	# -Wthread-safety-analysis
	-Wshadow
)

option(EXTRA_WARNINGS "Enable extra warnings" OFF)
if(EXTRA_WARNINGS)
	add_cxx_compiler_flag(-Wsuggest-override)
else()
	add_compiler_flag(-Wno-unused-parameter)
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
  add_compiler_flag(-Wrange-loop-analysis)
  # For 17 we need BOOST_NO_AUTO_PTR defined for boost
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\1" DARWIN_MAJOR_VERSION "${CMAKE_SYSTEM_VERSION}")
  message(STATUS "Darwin Version : ${CMAKE_SYSTEM_VERSION} found, major version ${DARWIN_MAJOR_VERSION} ")
  # HAVE_VARIANT means support for std::variant and std::optional
  if (DARWIN_MAJOR_VERSION GREATER 17) # 17 is High Sierra
    add_definitions("-DOS_MACOSX -DBOOST_NO_AUTO_PTR -DHAVE_STD_SHARED_MUTEX -DHAVE_VARIANT")
  else()
    add_definitions("-DOS_MACOSX -DBOOST_NO_AUTO_PTR")
  endif()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions("-DBOOST_NO_AUTO_PTR -DHAVE_STD_SHARED_MUTEX -DHAVE_VARIANT")
  add_compiler_flag(-Wno-cpp -Wno-unused-function -Wno-nonnull-compare -Wno-implicit-fallthrough)
else()
  # secp256k1 Linux gcc
  add_compiler_flag(-Wno-cpp -Wno-unused-function -Wno-nonnull-compare -Wno-implicit-fallthrough)
endif()


if (EXISTS	"${CMAKE_CURRENT_SOURCE_DIR}/config/bitcoin-config.h")
  message(FATAL_ERROR ": Please delete ${CMAKE_CURRENT_SOURCE_DIR}/config/bitcoin-config.h before continuing")
endif()


# Create a target for OpenSSL
include(BrewHelper)
find_brew_prefix(OPENSSL_ROOT_DIR openssl)
find_package(OpenSSL REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# libtool style configure
add_subdirectory(config)

# libraries
add_subdirectory(crypto)

#find_package(bzip2)
#find_package(lz4)

# Can change to LevelDB if needed
if (BUILD_ROCKSDB)
  find_package(RocksDB) # REQUIRED)
  if (RocksDB_FOUND)
    message(STATUS "Found RocksDB")
    message(STATUS "Libs : ${ROCKSDB_LIBRARIES}")
    add_definitions("-DUSE_ROCKSDB")
    set(DB_LIBRARIES ${ROCKSDB_LIBRARIES} ${BZIP2_LIBRARIES})
  else()
    message(STATUS "FAILED to FIND RocksDB")
    add_subdirectory(leveldb)
    set(DB_LIBRARIES leveldb memenv)
  endif()
else()
    add_subdirectory(leveldb)
    set(DB_LIBRARIES leveldb memenv)
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

# Because the Bitcoin ABc source code is disorganised, we
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
	random.cpp # needs openssl
	rcu.cpp
	rpc/protocol.cpp
	support/cleanse.cpp
	support/lockedpool.cpp
	sync.cpp
	threadinterrupt.cpp
	uint256.cpp
	util.cpp # needs openssl
	fs_util.cpp
	utilmoneystr.cpp
	utilstrencodings.cpp
	utiltime.cpp
)

target_compile_definitions(util PUBLIC HAVE_CONFIG_H)
target_include_directories(util
	PUBLIC
		.
		# To access the config.
		${CMAKE_CURRENT_BINARY_DIR}
    ${OPENSSL_INCLUDE_DIR}
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

# Boost packages required (Later check for and replace with std::filesystem
# but currently not working with any compilers)
set(BOOST_PACKAGES_REQUIRED filesystem)

function(prepend var prefix)
	set(listVar "")
	foreach(f ${ARGN})
		list(APPEND listVar "${prefix}${f}")
	endforeach(f)
	set(${var} "${listVar}" PARENT_SCOPE)
endfunction(prepend)

prepend(BOOST_LIBRARIES "Boost::" ${BOOST_PACKAGES_REQUIRED})
find_package(Boost 1.58 REQUIRED COMPONENTS ${BOOST_PACKAGES_REQUIRED})
target_link_libraries(util univalue crypto OpenSSL::Crypto OpenSSL::SSL ${BOOST_LIBRARIES})

# Make sure boost uses std::atomic (it doesn't before 1.63)
target_compile_definitions(util PUBLIC BOOST_SP_USE_STD_ATOMIC BOOST_AC_USE_STD_ATOMIC)

# More completely unrelated features shared by all executables.
# Because nothing says this is different from util than "common"
add_library(common
  ${HEADERS}
	amount.cpp
	base58.cpp
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
	init.cpp
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
	rpc/misc.cpp
	rpc/net.cpp
	rpc/rawtransaction.cpp
	rpc/safemode.cpp
	rpc/server.cpp
	script/scriptcache.cpp
	script/sigcache.cpp
	script/ismine.cpp
	timedata.cpp
	torcontrol.cpp
	txdb.cpp
	txmempool.cpp
	ui_interface.cpp
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

if (BUILD_TEST_BENCH)
  # Test suite.
  add_subdirectory(test)
  
  # Benchmark suite.
  add_subdirectory(bench)
endif()

# Wallet
if(BUILD_WALLET)
	add_subdirectory(wallet)
	target_link_libraries(server wallet)
endif()

# ZeroMQ
if (BUILD_ZMQ)
	add_subdirectory(zmq)
	target_link_libraries(server zmq)
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

# Bitcoin-qt
if(BUILD_QT)
	add_subdirectory(qt)
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

