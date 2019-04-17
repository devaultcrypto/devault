# Copyright (c) 2019 The DeVault developers
# Copyright (c) 2019 Jon Spock
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(wallet_tests)

find_package(Boost 1.58 REQUIRED ${BOOST_PACKAGES_REQUIRED})

target_include_directories(devault
	PRIVATE
    ${Boost_INCLUDE_DIRS}
		..
	PUBLIC
		# To access the config.
		${CMAKE_CURRENT_BINARY_DIR}/..
)

add_executable(bip39_test bip39_test.cpp)
target_include_directories(bip39_test	PRIVATE  ${Boost_INCLUDE_DIRS})
target_link_libraries(bip39_test devaultconsensus common util crypto )