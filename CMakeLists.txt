# Copyright (c) 2017 The Bitcoin developers
# Copyright (c) 2017 The DeVault developers

cmake_minimum_required(VERSION 3.10)
project(devault
	VERSION 1.1.7
#	DESCRIPTION "DeVault is a full node implementation of the DeVault protocol."
#	HOMEPAGE_URL "https://www.devault.cc"
)

enable_testing()

# Package information
set(PACKAGE_NAME "DeVault Core")
# Copyright
set(COPYRIGHT_YEAR 2019 - 2020)

# Add path for custom modules
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

set(Boost_NO_BOOST_CMAKE 1)

# Make contrib script accessible.
set(CONTRIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/contrib)

# If ccache is available, then use it.
find_program(CCACHE ccache)
if(CCACHE)
	message(STATUS "Using ccache: ${CCACHE}")
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
endif(CCACHE)

# Default to Release configuration
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Select the configuration for the build" FORCE)
endif()

# Find the python interpreter. This is required for several targets.
# not yet .. find_package(PythonInterp 3.5 REQUIRED)

# Add the magic targets `check-*`
add_custom_target(check-all)
add_custom_target(check)
add_custom_target(check-symbols)
add_custom_target(check-security)

include(PackageHelper)
exclude_git_ignored_files_from_source_package()

# Ignore hidden files and directories (starting with a '.')
set_property(GLOBAL APPEND PROPERTY SOURCE_PACKAGE_IGNORE_FILES	"/\\\\.")

# If the build is out-of-tree, then the build directory can be ignored.
if(NOT CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
	set_property(GLOBAL APPEND PROPERTY SOURCE_PACKAGE_IGNORE_FILES
		"${CMAKE_BINARY_DIR}/"
	)
endif()

exclude_from_source_package(
	# Subdirectories
	"depends/"

	# Files
	"[^.]+[.]md$"
	"Dockerfile-doxygen"
)

add_subdirectory(src)
add_subdirectory(test)

#add_subdirectory(contrib)
#add_subdirectory(contrib/devtools)
add_subdirectory(doc)

include(PackageOptions)
get_property(CPACK_SOURCE_IGNORE_FILES GLOBAL PROPERTY SOURCE_PACKAGE_IGNORE_FILES)
include(CPack)
