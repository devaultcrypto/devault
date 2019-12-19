# Copyright (c) 2019 The DeVault developers
# Copyright (c) 2019 Jon Spock
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(devault)

file(GLOB DEVAULT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("DevaultHeaders" FILES ${DEVAULT_HEADERS})

# The library
add_library(devault
  ${DEVAULT_HEADERS}
  rewards.cpp
  rewards_calculation.cpp
  rewardsview.cpp
  budget.cpp
)

target_include_directories(devault
	PRIVATE
    ${Boost_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/include
	..
	PUBLIC
	# To access the config.
	${CMAKE_CURRENT_BINARY_DIR}/..
)

