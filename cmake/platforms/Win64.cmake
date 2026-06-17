# Copyright (c) 2017 The Bitcoin developers

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use given TOOLCHAIN_PREFIX if specified
if(CMAKE_TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX ${CMAKE_TOOLCHAIN_PREFIX})
else()
  set(TOOLCHAIN_PREFIX ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)
endif()

# cross compilers to use for C and C++
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_C_COMPILER_TARGET ${TOOLCHAIN_PREFIX})
set(CMAKE_CXX_COMPILER_TARGET ${TOOLCHAIN_PREFIX})

# target environment on the build host system
#   set 1st to dir with the cross compiler's C/C++ headers/libs
set(CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/${TOOLCHAIN_PREFIX};/usr/${TOOLCHAIN_PREFIX}")

# Guix reproducible build: there is no /usr/<triple> sysroot; the mingw system headers/libs
# (shlwapi.h/libshlwapi.a, ws2_32, ...) live in the cross toolchain's store path. build.sh
# exports it so find_package(SHLWAPI/...) — which uses find_path/find_library, not the compiler
# env — can resolve Windows system libraries. Guarded: a no-op for non-Guix dev cross builds.
if(DEFINED ENV{GUIX_CROSS_TOOLCHAIN_ROOT})
  list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{GUIX_CROSS_TOOLCHAIN_ROOT}")
endif()

# We also may have built dependencies for the native plateform.
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/${TOOLCHAIN_PREFIX}/native")

# modify default behavior of FIND_XXX() commands to
# search for headers/libs in the target environment and
# search for programs in the build host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Tell cmake to use the static lib otherwise it may error out
set(Boost_USE_STATIC_LIBS ON)
