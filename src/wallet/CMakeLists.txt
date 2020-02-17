# Copyright (c) 2017 The Bitcoin developers

project(wallet)

# Add Berkeley DB dependency.
find_package(BerkeleyDB REQUIRED)

# Add event dependency. This is only required for evhttp_uridecode
# in rpcwallet.cpp so it may be worth considering using an alternative.
find_package(Event REQUIRED)

file(GLOB WALLET_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h )
source_group("WalletHeaders" FILES ${WALLET_HEADERS})

add_library(wallet
  ${WALLET_HEADERS}
  ../interfaces/wallet.cpp
  ../script/ismine.cpp
  ../rpc/util.cpp
  analyzecoins.cpp
  bls_tx.cpp
  crypter.cpp
  db.cpp
  extkey.cpp
  fees.cpp
  finaltx.cpp
  init.cpp
  rpcdump.cpp
  rpcwallet.cpp
  wallet.cpp
  walletdb.cpp
  walletutil.cpp
  dictionary.cpp
  mnemonic.cpp
  hdchain.cpp
  misc.cpp
  wallettool.cpp
)

target_link_libraries(wallet util univalue Event ${BDBXX_LIBRARY})

if (RocksDB_FOUND)
  target_include_directories(wallet  PUBLIC ${BDBXX_INCLUDE_DIR})
else()
  target_include_directories(wallet
    PUBLIC ${BDBXX_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/include
    )
endif()


# bdb-check
if(BUILD_WALLETTOOL)
	add_executable(bdb-check bdb-check.cpp)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_sources(bdb-check PRIVATE bdb-check-res.rc)
	endif()

	target_link_libraries(bdb-check devaultconsensus wallet server bls relic_s)
endif()


# check if using tests here...
if (BUILD_CTESTS)
  add_subdirectory(test)
endif()