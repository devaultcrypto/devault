cmake_minimum_required(VERSION 3.1)
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.1")
	cmake_policy(SET CMP0054 NEW)
endif()

project(RELIC C CXX)

set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "4")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(VERSION ${PROJECT_VERSION})

set(INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/low)
include_directories(${include_directories} ${CMAKE_BINARY_DIR} ${INCLUDE})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

message("-- Configuring ${PROJECT_NAME} ${PROJECT_VERSION}...")
  
option(DEBUG "Build with debugging support" off)
option(PROFL "Build with profiling support" off)
option(TRACE "Build with tracing support" off)
option(OVERH "Build with overhead estimation" off)
option(DOCUM "Build documentation" off)
option(STRIP "Build only the selected algorithms" off)
option(QUIET "Build with printing disabled" off)
option(COLOR "Build with colored output" on)
option(BIGED "Build with big-endian support" off)
option(STLIB "Build static library" on)
option(STBIN "Build static binaries" on)
option(AMALG "Build amalgamation" off)

if (SHOW_MESSAGES) 
  message(STATUS "Available switches (default = VERBS):\n")
  
  message("   DEBUG=[off|on] Build with debugging support.")
  message("   PROFL=[off|on] Build with profiling support.")
  message("   TRACE=[off|on] Build with tracing support.")
  message("   OVERH=[off|on] Build with overhead estimation.")
  message("   DOCUM=[off|on] Build documentation.")
  message("   STRIP=[off|on] Build only selected algorithms.")
  message("   QUIET=[off|on] Build with printing disabled.")
  message("   COLOR=[off|on] Build with colored output.")
  message("   BIGED=[off|on] Build with big-endian support.")
  message("   STLIB=[off|on] Build static library.")
  message("   STBIN=[off|on] Build static binaries.")
  message("   AMALG=[off|on] Build amalgamation for better performance.")

  message(STATUS "Number of times each test or benchmark is ran (default = 50, 1000):")
  
  message("   TESTS=n        If n > 0, build automated tests and run them n times.")
  message("   BENCH=n        If n > 0, build automated benchmarks and run them n * n times.")
  
  message(STATUS "Number of available processor cores (default = 1):")
  
  message("   CORES=n        If n > 1, please enable multithreading support.")
  
  message(STATUS "Available modules (default = ALL)")
  
  message("   WITH=BN       Multiple precision arithmetic.")
  message("   WITH=DV       Temporary double-precision digit vectors.")
  message("   WITH=FP       Prime field arithmetic.")
  message("   WITH=FPX      Prime extension field arithmetic.")
  message("   WITH=FB       Binary field arithmetic.")
  message("   WITH=EP       Elliptic curves over prime fields.")
  message("   WITH=EPX      Elliptic curves over quadratic extensions of prime fields.")
  message("   WITH=EB       Elliptic curves over binary fields.")
  message("   WITH=ED       Elliptic Edwards curves over prime fields.")
  message("   WTTH=EC       Elliptic curve cryptography.")
  message("   WITH=PB       Pairings over binary elliptic curves.")
  message("   WITH=PP       Pairings over prime elliptic curves.")
  message("   WTTH=PC       Pairing-based cryptography.")
  message("   WITH=BC       Block ciphers (symmetric encryption).")
  message("   WITH=MD       Message digests (hash functions).")
  message("   WITH=CP       Cryptographic protocols.")
  message("   WITH=ALL      All of the above.")
  message("   Note: the programmer is responsible for not using unselected modules.")
  
  message(STATUS "Available arithmetic backends (default = easy):")

  message("   ARITH=easy     Easy-to-understand implementation.")
  message("   ARITH=gmp      GNU Multiple Precision library.")
  
  message(STATUS "Available memory-allocation policies (default = AUTO):")
  
  message("   ALLOC=AUTO     All memory is automatically allocated.")
  message("   ALLOC=STATIC   All memory is allocated statically once.")
  message("   ALLOC=DYNAMIC  All memory is allocated dynamically on demand.")
  message("   ALLOC=STACK    All memory is allocated from the stack.")
  
  message(STATUS "Supported operating systems (default = LINUX):")
  
  message("   OPSYS=         Undefined/No specific operating system.")
  message("   OPSYS=LINUX    GNU/Linux operating system.")
  message("   OPSYS=FREEBSD  FreeBSD operating system.")
  message("   OPSYS=MACOSX   Mac OS X operating system.")
  message("   OPSYS=WINDOWS  Windows operating system.")
  message("   OPSYS=DROID    Android operating system.")
  message("   OPSYS=DUINO    Arduino platform.")
  
  message(STATUS "Supported multithreading APIs (default = UNDEF):")
  
  message("   MULTI=         No multithreading support.")
  message("   MULTI=OPENMP   Open Multi-Processing.")
  message("   MULTI=PTHREAD  POSIX threads.")

  message(STATUS "Supported timers (default = HPROC):")
  
  message(STATUS "Prefix to identify this build of the library (default = \"\"):")
  
  message("   LABEL=relic")
endif()


include(cmake/bn.cmake)
include(cmake/fb.cmake)
include(cmake/fp.cmake)
include(cmake/fpx.cmake)
include(cmake/ep.cmake)
include(cmake/pp.cmake)
include(cmake/md.cmake)
include(cmake/rand.cmake)

# Check if multiple precision integer arithmetic is required.
set(WITH_BN 1)

# Check if temporary vectors are required.
set(WITH_DV 1)

# Check if prime field arithmetic is required.
set(WITH_FP 1)

# Check if prime extension field arithmetic is required.
set(WITH_FPX 1)

# Check if prime elliptic curve support is required.
set(WITH_EP 1)

#Check if support for elliptic curves defined over prime field extensions is required.
set(WITH_EPX 1)

# Check if support for pairings over prime curves is required.
set(WITH_PP 1)

# Check if support for pairings over binary curves is required.
set(WITH_PB 1)

# Check if support for hash functions is required.
set(WITH_MD 1)

# Compiler flags.
# set(COMP "-O3 -Wno-unused-function -funroll-loops -fomit-frame-pointer -march=native -mtune=native")

# Discover the operating system.
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(OPSYS "LINUX" CACHE STRING "Operating system")
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)
  
if(OPSYS STREQUAL LINUX)
  add_definitions(-D_GNU_SOURCE)
endif(OPSYS STREQUAL LINUX)

message(STATUS "Configured operating system: ${OPSYS}")

include(cmake/gmp.cmake)
if(GMP_FOUND)
	include_directories(${include_directories} ${GMP_INCLUDE_DIR})
	set(ARITH_LIBS ${GMP_LIBRARIES})
endif(GMP_FOUND)


message(STATUS "Compiler flags: ${CMAKE_C_FLAGS}")

string(TOUPPER ${ARITH} ARITH)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in ${CMAKE_BINARY_DIR}/relic_conf.h @ONLY)
message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR}/include/relic_conf.h.in")
string(TOLOWER ${ARITH} ARITH)

set(RELIC "relic")
set(RELIC_S "relic_s")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

if(DOCUM)
	include(cmake/doxygen.cmake)
endif(DOCUM)

