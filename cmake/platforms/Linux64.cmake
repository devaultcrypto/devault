# Copyright (c) 2019 The Bitcoin developers

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use given TOOLCHAIN_PREFIX if specified
if(CMAKE_TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX ${CMAKE_TOOLCHAIN_PREFIX})
else()
  set(TOOLCHAIN_PREFIX ${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)
endif()

# Cross compilers to use for C and C++.
# Prefer the TOOLCHAIN_PREFIX-prefixed compiler when present (e.g. the Guix old-glibc
# x86_64-linux-gnu cross toolchain used for portable release builds); otherwise fall back to
# the plain native gcc/g++ (dev builds that pass this file without a cross toolchain present).
find_program(_TOOLCHAIN_PREFIXED_CC ${TOOLCHAIN_PREFIX}-gcc)
if(_TOOLCHAIN_PREFIXED_CC)
  set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
else()
  set(CMAKE_C_COMPILER gcc)
  set(CMAKE_CXX_COMPILER g++)
endif()

set(CMAKE_C_COMPILER_TARGET ${TOOLCHAIN_PREFIX})
set(CMAKE_CXX_COMPILER_TARGET ${TOOLCHAIN_PREFIX})

# Target environment on the build host system
# Set 1st to directory with the cross compiler's C/C++ headers/libs
set(CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/${TOOLCHAIN_PREFIX}")

# We also may have built dependencies for the native platform.
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/${TOOLCHAIN_PREFIX}/native")

# Modify default behavior of FIND_XXX() commands to:
#  - search for headers in the target environment,
#  - search the libraries in the target environment first then the host (to find
#    the compiler supplied libraries),
#  - search for programs in the build host environment.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

string(APPEND CMAKE_C_FLAGS_INIT " -m64")
string(APPEND CMAKE_CXX_FLAGS_INIT " -m64")

# Tell cmake to use the static lib otherwise it may error out
set(Boost_USE_STATIC_LIBS ON)
