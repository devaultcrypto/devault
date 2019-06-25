# Copyright (c) 2019 The DeVault developers
# Copyright (c) 2019 Jon Spock
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(wallet_tests)

find_package(Boost 1.58 REQUIRED unit_test_framework)
#We need to detect if the BOOST_TEST_DYN_LINK flag is required.
set(CMAKE_REQUIRED_LIBRARIES Boost::unit_test_framework)
check_cxx_source_compiles("
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
" BOOST_TEST_DYN_LINK)

target_include_directories(devault
	PRIVATE
    ${Boost_INCLUDE_DIRS}
		..
	PUBLIC
		# To access the config.
		${CMAKE_CURRENT_BINARY_DIR}/..
    )

SET(WALLET_CTESTS  
	accounting_tests
	bip39_test
	walletdb_tests
	wallet_tests
	)


FOREACH(tst ${WALLET_CTESTS})
	SET(SRC ${tst}.cpp wallet_test_fixture.cpp)
	ADD_EXECUTABLE(tb_${tst} ${SRC})
  #To access the generated json headers.
  target_include_directories(tb_${tst}	PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(tb_${tst} Boost::unit_test_framework rpcclient server unit)
  add_test(NAME test_${tst} COMMAND tb_${tst})
  #	set_property(TARGET tb_${tst} PROPERTY CXX_STANDARD 11)
  #	set_property(TARGET tb_${tst} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  #  set_target_properties(tb_${tst} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/..")
ENDFOREACH(tst ${EXTS})

#--

if(BOOST_TEST_DYN_LINK)
FOREACH(tst ${WALLET_CTESTS})
	target_compile_definitions(tb_${tst} PRIVATE BOOST_TEST_DYN_LINK)
ENDFOREACH()
endif(BOOST_TEST_DYN_LINK)


#target_link_libraries(bip39_test devaultconsensus common util crypto )