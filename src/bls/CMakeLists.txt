CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)

project(bls)

#------------------- ------------------- ------------------- ------------------- -------------------
# For Relic
#------------------- ------------------- ------------------- ------------------- -------------------
##SET(CMAKE_VERBOSE_MAKEFILE OFF)
set(WSIZE 64)
set(AFLAGS "-m64")
set(ALIGN 1)
set(ARITH "GMP")
set(ALLOC "AUTO")
set(STBIN "OFF" CACHE STRING "")
set(FP_PRIME 381 CACHE INTEGER "")
set(RAND "HASHD")
set(EC_CUR "PRIME")
set(QUIET 1)
set(BENCH 0)
set(TESTS 0)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions("-DWIND_VER")
ENDIF()

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DSEED "UDEV" CACHE STRING "")
  set(FP_QNRES "off" CACHE STRING "")
ELSE()
  set(DSEED "DEV" CACHE STRING "")
  set(FP_QNRES "on" CACHE STRING "")
ENDIF()

set(FPX_METHD "INTEG;INTEG;LAZYR" CACHE STRING "")

string(TOUPPER ${ARITH} ARITH)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/relic/include/relic_conf.h.in ${CMAKE_BINARY_DIR}/relic_conf.h @ONLY)
message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in")
string(TOLOWER ${ARITH} ARITH)


# THIS IS ONLY DEFINED FOR STANDALONE BUILDS
if (BUILD_BLS)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)
  
  # Add path for custom modules
  set(CMAKE_MODULE_PATH
	  ${CMAKE_MODULE_PATH}
	  ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules
    )
  
  include(AddCompilerFlags)
  check_cxx_compiler_flag(-std=c++17 HAVE_FLAG_STD_CXX17)
  if (HAVE_FLAG_STD_CXX17)
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "Compiler supports C++17")
  else()
    message(ERROR "Compiler must at least support C++17")
  endif()

  add_compiler_flags(-Wno-cpp -Wno-unused-function -Wno-nonnull-compare -Wno-implicit-fallthrough)

  add_subdirectory(config)
endif()


  
add_subdirectory(relic)

file(GLOB BLS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB OTHER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("BlsHeaders" FILES ${BLS_HEADERS} ${OTHER_HEADERS})

add_library(bls ${BLS_HEADERS} ${OTHER_HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/privatekey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/publickey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bls_functions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/util.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/elements.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schemes.cpp
  )

target_include_directories(bls
  PRIVATE
  ${include_directories}
  Boost::headers
  ${GMP_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}//relic/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_BINARY_DIR} # for relic_conf.h
  ${CMAKE_BINARY_DIR}/src # for bitcoin-config.h
  )

target_link_libraries(bls relic_s sodium)


