# Copyright(c) 2018 The Bitcoin developers
# Copyright(c) 2019 DeVault developers

project(catch-tests)

# Process json files.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data")

find_program(PYTHON2 python) # Python 3 had issues on Linux
function(gen_json_header NAME)
	set(HEADERS "")
	foreach(f ${ARGN})
		set(h "${CMAKE_CURRENT_BINARY_DIR}/${f}.h")

#Get the proper name for the test variable.
		get_filename_component(TEST_NAME ${f} NAME_WE)
		add_custom_command(OUTPUT ${h}
			COMMAND ${PYTHON2}
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

#include(TestSuite)
add_compiler_flag(-Wno-cast-align -Wno-unused-function -Wno-unused-variable)

add_library(unit
  callrpc.cpp
  jsonutil.cpp
  scriptflags.cpp
  sigutil.cpp
  test_bitcoin.cpp
  test_bitcoin_main.cpp
  ${JSON_HEADERS}
  )
target_include_directories(unit	PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../univalue/include
  )
target_link_libraries(unit rpcclient server devaultconsensus common)

set(UNIT_CTESTS
#  activation - nothing currently in here
  addrman
  allocator
  arith_uint256
  amount
  avalanche
  base32
  base64
  blockchain
  blockcheck
  blockencodings
  blockfilter
  blockindex
  blockstatus
  bloom
  bswap
  cashaddr
  cashaddrenc
  checkdatasig
  checkpoints 
  checkqueue
  coins
  compress
  config
  core_io
  crypto
  cuckoocache
  dbwrapper
  denialofservice
  dstencode
  excessiveblock
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
#  net  - ok on Mac
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
  script_standard
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
  torcontrol
  transaction
  txindex
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

FOREACH(tst ${UNIT_CTESTS})
  add_executable(tb_${tst} ${tst}_tests.cpp)
  target_include_directories(tb_${tst}  PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(tb_${tst} rpcclient server devaultconsensus common unit)
  add_test(NAME test_${tst} COMMAND tb_${tst})
ENDFOREACH(tst ${UNIT_CTESTS})

