# Copyright(c) 2018 The Bitcoin developers
# Copyright(c) 2019 DeVault developers

project(devault-test)

# Process json files.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data")

find_program(PYTHON python)
function(gen_json_header NAME)
	set(HEADERS "")
	foreach(f ${ARGN})
		set(h "${CMAKE_CURRENT_BINARY_DIR}/${f}.h")

#Get the proper name for the test variable.
		get_filename_component(TEST_NAME ${f} NAME_WE)
		add_custom_command(OUTPUT ${h}
			COMMAND ${PYTHON}
			ARGS
				"${CMAKE_CURRENT_SOURCE_DIR}/data/generate_header.py"
				"${TEST_NAME}"
				"${CMAKE_CURRENT_SOURCE_DIR}/${f}" > ${h}
			MAIN_DEPENDENCY ${f}
			DEPENDS
				"data/generate_header.py"
			VERBATIM
		)
		list(APPEND HEADERS ${h})
	endforeach(f)
	set(${NAME} "${HEADERS}" PARENT_SCOPE)
endfunction()

gen_json_header(JSON_HEADERS
	data/blockfilters.json
	data/script_tests.json
	data/tx_invalid.json
	data/tx_valid.json
	data/sighash.json
)

include(TestSuite)
create_test_suite(devault)
add_dependencies(check check-devault)

add_compiler_flag(-Wno-cast-align -Wno-unused-function -Wno-unused-variable)

add_test_to_suite(devault test_devault
	activation_tests.cpp
	addrman_tests.cpp
	allocator_tests.cpp
	amount_tests.cpp
	arith_uint256_tests.cpp
#  avalanche_tests.cpp
	base32_tests.cpp
	base64_tests.cpp
	bip32_tests.cpp
	bitmanip_tests.cpp
	blockchain_tests.cpp
	blockcheck_tests.cpp
	blockencodings_tests.cpp
	blockfilter_tests.cpp
	blockindex_tests.cpp
	blockstatus_tests.cpp
  bloom_tests.cpp
	bswap_tests.cpp
	cashaddr_tests.cpp
	cashaddrenc_tests.cpp
	checkdatasig_tests.cpp
	checkpoints_tests.cpp
  checkqueue_tests.cpp
	coins_tests.cpp
  compress_tests.cpp
	config_tests.cpp
	core_io_tests.cpp
	crypto_tests.cpp
	cuckoocache_tests.cpp
	dbwrapper_tests.cpp
	denialofservice_tests.cpp
	dstencode_tests.cpp
	excessiveblock_tests.cpp
	feerate_tests.cpp
	file_tests.cpp
	finalization_tests.cpp
	getarg_tests.cpp
	hash_tests.cpp
	inv_tests.cpp
	jsonutil.cpp
	key_tests.cpp
	lcg_tests.cpp
	limitedmap_tests.cpp
	main_tests.cpp
  mempool_tests.cpp
	merkle_tests.cpp
	miner_tests.cpp
	monolith_opcodes_tests.cpp
	multisig_tests.cpp
	net_tests.cpp
	netbase_tests.cpp
	pmt_tests.cpp
	policyestimator_tests.cpp
  pow_tests.cpp
	prevector_tests.cpp
	radix_tests.cpp
	raii_event_tests.cpp
	random_tests.cpp
	rcu_tests.cpp
  reward_tests.cpp
	reverselock_tests.cpp
	rpc_tests.cpp
	rpc_server_tests.cpp
	rwcollection_tests.cpp
	sanity_tests.cpp
	scheduler_tests.cpp
	script_bitfield_tests.cpp
	script_commitment_tests.cpp
	script_P2SH_tests.cpp
	script_standard_tests.cpp
	script_tests.cpp
	scriptflags.cpp
	scriptnum_tests.cpp
  serialize_tests.cpp
  sigcache_tests.cpp
	sigencoding_tests.cpp
  sighash_tests.cpp
	sighashtype_tests.cpp
	sigopcount_tests.cpp
	sigutil.cpp
	skiplist_tests.cpp
	streams_tests.cpp
	sync_tests.cpp
	test_bitcoin.cpp
	test_bitcoin_main.cpp
	timedata_tests.cpp
  torcontrol_tests.cpp
	transaction_tests.cpp
	txindex_tests.cpp
	txvalidation_tests.cpp
	txvalidationcache_tests.cpp
	uint256_tests.cpp
  undo_tests.cpp
	univalue_tests.cpp
	util_tests.cpp
	validation_tests.cpp
	work_comparator_tests.cpp

#RPC Tests
	../rpc/test/server_tests.cpp

#Tests generated from JSON
	${JSON_HEADERS}
)

target_include_directories(test_devault
	PUBLIC
#To access the generated json headers.
		${CMAKE_CURRENT_BINARY_DIR}
)

find_package(Boost 1.58 REQUIRED unit_test_framework)
#We need to detect if the BOOST_TEST_DYN_LINK flag is required.
set(CMAKE_REQUIRED_LIBRARIES Boost::unit_test_framework)
check_cxx_source_compiles("
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
" BOOST_TEST_DYN_LINK)

add_library(unit
	jsonutil.cpp
	scriptflags.cpp
	sigutil.cpp
  test_bitcoin.cpp
  ${JSON_HEADERS}
  )
target_include_directories(unit	PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../univalue/include
  )
target_link_libraries(unit rpcclient server devaultconsensus common Boost::unit_test_framework)

set(BOOST_TARGETS
#  activation - nothing currently in here
  addrman
  allocator
  arith_uint256
  amount
#  avalanche
  base32
  base64
###  blockchain
  blockcheck
#  blockencodings
  blockfilter
  blockindex
  blockstatus
  bloom
  bswap
  cashaddr
  cashaddrenc
  checkdatasig
#  checkpoints - nothing yet
  checkqueue
  coins
  compress
  config
  core_io
  crypto
  cuckoocache
  dbwrapper
  DoS
  dstencode
#  excessiveblock
  feerate
  file
  finalization
  getarg
  hash
  inv
  key
  lcg
  limitedmap
  main
  mempool
  merkle
  miner
  monolith_opcodes
  multisig
#  net -- check why 
  netbase
  pmt
  policyestimator
  pow
  prevector
  radix
  raii_event
  random
  rcu
  reward
  reverselock
#  rpc  - nothing yet
  rpc_server
  rwcollection
  sanity
  scheduler
  schnorr
  script_commitment
  script_P2SH
###  script_standard
  script
  scriptnum
  serialize
  sigcache
  sigencoding
  sighash
  sighashtype
  sigopcount
  skiplist
  streams
  sync
  timedata
  transaction
###  txindex
  txvalidationcache
  uint256
  undo
  univalue
  util
  validation
  work_comparator
#  ../rpc/test/server
)

#---------VALGRIND STUFF----------------------------------------------------
find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --track-origins=yes --show-reachable=yes --leak-check=full" )

# below functions not used/needed yet
function(add_memcheck_test name binary)
  set(memcheck_command "${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}")
  separate_arguments(memcheck_command)
  add_test(${name} ${binary} ${ARGN})
  add_test(memcheck_${name} ${memcheck_command} ./${binary} ${ARGN})
endfunction(add_memcheck_test)

function(set_memcheck_test_properties name)
  set_tests_properties(${name} ${ARGN})
  set_tests_properties(memcheck_${name} ${ARGN})
endfunction(set_memcheck_test_properties)

#---------VALGRIND STUFF----------------------------------------------------


FOREACH(tst ${BOOST_TARGETS})
  SET(SRC ${tst}_tests.cpp test_bitcoin_main.cpp)
  ADD_EXECUTABLE(tb_${tst} ${SRC})
  #To access the generated json headers.
  target_include_directories(tb_${tst}  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(tb_${tst} Boost::unit_test_framework rpcclient server devaultconsensus common unit bls relic_s)
  add_test(NAME test_${tst} COMMAND tb_${tst})
ENDFOREACH(tst ${EXTS})


if(BOOST_TEST_DYN_LINK)
FOREACH(tst ${BOOST_TARGETS})
  target_compile_definitions(tb_${tst} PRIVATE BOOST_TEST_DYN_LINK)
ENDFOREACH()
endif(BOOST_TEST_DYN_LINK)



