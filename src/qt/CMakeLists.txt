# Copyright (c) 2018 The Bitcoin developers
# Copyright (c) 2019 DeVault developers
# Copyright (c) 2019 Jon Spock

project(devault-qt)
cmake_minimum_required(VERSION 3.5)

if (CMAKE_GENERATOR STREQUAL Xcode)
  message(STATUS "Setting DEV_DEBUG for Xcode debugging")
  add_compiler_flags(-DDEV_DEBUG)
endif()


# Project Info
SET(APP_NAME devault-qt)
SET(PROJECT_NAME_SHORT   "devault-qt")
SET(PROJECT_NAME_LONG    "CMake DeVault Qt project")
SET(PROJECT_DESCRIPTION  "CMake build system template for Qt based projects")
SET(PROJECT_COPYRIGHT    "Copyright (C) 2019 DeVault Developers")
SET(PROJECT_CONTACT      "jonspock@protonmail.com")
SET(PROJECT_VENDOR       "Jon Spock")
SET(ORG_WEBSITE          "https://devault.cc")

SET(PROJECT_VERSION_PATCH ${PATCH_OUT})

SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_REVISION "${VERSION_REVISION}")

if (VERSION_SUFFIX)
  SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_SUFFIX}")
  SET(PROJECT_VERSION_MAJOR ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION})
  SET(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION}.${CPACK_PACKAGE_VERSION_PATCH})
else()
  SET(CPACK_PACKAGE_VERSION "${VERSION_DESC}")
endif()
  
SET(PROJECT_VERSION ${CPACK_PACKAGE_VERSION})

MESSAGE(STATUS "QT Version: ${PROJECT_VERSION}")

STRING(TOLOWER ${PROJECT_NAME_SHORT} PROJECT_NAME_SHORT_LOWER)
PROJECT(${PROJECT_NAME_SHORT_LOWER}-${PROJECT_VERSION})

# System on which project is built
SITE_NAME(HOSTNAME)
SET(SYSNAME ${CMAKE_SYSTEM}-${CMAKE_SYSTEM_PROCESSOR})

# CMake Settings
SET(CMAKE_COLOR_MAKEFILE ON)

# This ensure that AUTOMOC doesn't run on generated files.
if (CMAKE_VERSION VERSION_LESS 3.10)
  message("Not setting CMP0071")
else()
  cmake_policy(SET CMP0071 OLD)
endif()

if (TOOLCHAIN_PREFIX)
  set(Qt5_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../depends/${TOOLCHAIN_PREFIX}/lib/cmake/Qt5/)
endif()

include(BrewHelper)
find_brew_prefix(QT5_PREFIX qt5)

set(QT_REQUIRED_COMPONENTS Core Widgets Network Test DBus)
find_package(Qt5 COMPONENTS ${QT_REQUIRED_COMPONENTS} REQUIRED HINTS "${QT5_PREFIX}")

# Find out more about Qt. This is similar to
# http://code.qt.io/cgit/qt/qtwebkit.git/tree/Source/cmake/OptionsQt.cmake
get_target_property(QT_CORE_TYPE Qt5::Core TYPE)
if(QT_CORE_TYPE MATCHES STATIC)
	set(QT_STATIC_BUILD ON)
endif()


find_package(Qt5DBus)
if (Qt5DBus_FOUND)
  add_compile_options("-DUSE_DBUS")
endif()

find_package(QREncode)
if (QREncode_FOUND)
  if (ENABLE_QRCODE)
    add_definitions(-DUSE_QRCODE)
    message(STATUS "Found QREncode")
    set(QR_LIB ${QRENCODE_LIBRARY})
  else()
    set(ENABLE_QRCODE false)
  endif()
else()
    message(STATUS " QREncode NOT FOUND ")
    set(ENABLE_QRCODE false)
endif()
 

# Determine the Qt libraries directory from the QT5::Core library location
get_target_property(QT_CORE_LIB_LOCATION Qt5::Core LOCATION)
get_filename_component(QT5_LIB_DIR "${QT_CORE_LIB_LOCATION}" DIRECTORY)

set(STATIC_DEPENDENCIES_CMAKE_FILE "${CMAKE_BINARY_DIR}/QtStaticDependencies.cmake")
if(EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
	file(REMOVE ${STATIC_DEPENDENCIES_CMAKE_FILE})
endif()

set(CONVERT_PRL_PATH "${CONTRIB_PATH}/qt/convert-prl-libs-to-cmake.pl")
macro(CONVERT_PRL_LIBS_TO_CMAKE _qt_component)
	if(TARGET Qt5::${_qt_component})
		get_target_property(_lib_location Qt5::${_qt_component} LOCATION)
		execute_process(COMMAND ${PERL_EXECUTABLE} "${CONVERT_PRL_PATH}"
			--lib "${_lib_location}"
			--qt_lib_install_dir "${QT5_LIB_DIR}"
			--out "${STATIC_DEPENDENCIES_CMAKE_FILE}"
			--component "${_qt_component}"
			--compiler "${CMAKE_CXX_COMPILER_ID}"
		)
	endif()
endmacro()

# UI elements
qt5_wrap_ui(UI_GENERATED_HEADERS
	forms/addressbookpage.ui
	forms/askpassphrasedialog.ui
	forms/setpassphrasedialog.ui
	forms/coincontroldialog.ui
	forms/editaddressdialog.ui
	forms/helpmessagedialog.ui
	forms/intro.ui
	forms/modaloverlay.ui
	forms/openuridialog.ui
	forms/optionsdialog.ui
	forms/overviewpage.ui
	forms/receivecoinsdialog.ui
	forms/receiverequestdialog.ui
	forms/debugwindow.ui
	forms/sendcoinsdialog.ui
	forms/sendcoinsentry.ui
	forms/rewardcontroldialog.ui
	forms/rewardsdialog.ui
	forms/rewardsentry.ui
	forms/signverifymessagedialog.ui
	forms/transactiondescdialog.ui
	forms/startoptionsdialog.ui
	forms/startoptions.ui
	forms/startoptionsrestore.ui
	forms/startoptionssort.ui
	forms/startoptionsrevealed.ui
	forms/startoptionsmain.ui
	forms/revealphrase.ui
	forms/sweep.ui
	forms/sweepbls.ui
)

# Qt MOC
set(CMAKE_AUTOMOC ON)

# Handle qrc resources
qt5_add_resources(QRC_BITCOIN_CPP bitcoin.qrc)

add_library(bitcoin-qt-base
  ${QT_BASE_HEADERS}
	bantablemodel.cpp
	bitcoinaddressvalidator.cpp
	bitcoinamountfield.cpp
	bitcoingui.cpp
	bitcoinunits.cpp
	clientmodel.cpp
	checkcoins.cpp
	csvmodelwriter.cpp
    dvtui.cpp
	guiutil.cpp
	intro.cpp
	modaloverlay.cpp
	networkstyle.cpp
	notificator.cpp
	optionsdialog.cpp
	optionsmodel.cpp
	peertablemodel.cpp
	platformstyle.cpp
	qvalidatedlineedit.cpp
	qvaluecombobox.cpp
	rpcconsole.cpp
	splashscreen.cpp
	trafficgraphwidget.cpp
	utilitydialog.cpp
	setpassphrasedialog.cpp
	dvtui.cpp
	revealphrase.cpp
	sweep.cpp
	sweepbls.cpp

	startoptionsdialog.cpp
	startoptions.cpp
	startoptionsrestore.cpp
	startoptionssort.cpp
	startoptionsrevealed.cpp
	startoptionsmain.cpp

	# Handle ui files
	${UI_GENERATED_HEADERS}

	# Translations
	${BITCOIN_QM_FILES}

	# Handle qrc files
	${QRC_BITCOIN_CPP}
	qrc_bitcoin_locale.cpp

  )

# Linux support
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	list(APPEND QT_PLUGIN_COMPONENTS QXcbIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_XCB=1)
endif()

# Windows support
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND QT_PLUGIN_COMPONENTS QWindowsIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_WINDOWS=1)

	target_sources(bitcoin-qt-base PRIVATE winshutdownmonitor.cpp)
endif()

# OSX support
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	list(APPEND QT_PLUGIN_COMPONENTS QCocoaIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_COCOA=1)

	target_sources(bitcoin-qt-base PRIVATE
		macdockiconhandler.mm
		macnotificationhandler.mm
	)

	set_property(TARGET bitcoin-qt-base PROPERTY AUTOMOC_MOC_OPTIONS "-DQ_OS_MAC")

	target_link_libraries(bitcoin-qt-base
		"-framework Foundation"
		"-framework ApplicationServices"
		"-framework AppKit"
	)
endif()

if(QT_STATIC_BUILD)
	list(APPEND QT_REQUIRED_COMPONENTS ${QT_PLUGIN_COMPONENTS})
	foreach(qt_module ${QT_REQUIRED_COMPONENTS})
		CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
	endforeach()
	# HACK: We must explicitly add LIB path of the Qt installation
	# to correctly find qtpcre
	link_directories("${QT5_LIB_DIR}")

	# Now that we generated the dependencies, import them.
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CONVERT_PRL_PATH}")
	if(NOT EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
		message(FATAL_ERROR "Unable to find ${STATIC_DEPENDENCIES_CMAKE_FILE}")
	endif()
	include(${STATIC_DEPENDENCIES_CMAKE_FILE})
	list(REMOVE_DUPLICATES STATIC_LIB_DEPENDENCIES)

	# According to Qt documentation (https://doc.qt.io/qt-5/plugins-howto.html):
	# "Plugins can be linked statically into your application.
	# If you build the static version of Qt, this is the only option for
	# including Qt's predefined plugins."
	# So if the Qt build is static, the plugins should also be static and we
	# need to define QT_STATICPLUGIN to tell the code to import <QTPlugin>.
	target_compile_definitions(bitcoin-qt-base PUBLIC -DQT_STATICPLUGIN=1)

	# Add the platform plugin definition if required
	# Setting this definition tells the code what is the target for Q_IMPORT_PLUGIN().
	foreach(qt_platform_definition ${QT_PLUGIN_PLATFORM_DEFINITIONS})
		target_compile_definitions(bitcoin-qt-base PUBLIC "${qt_platform_definition}")
	endforeach()

	# Link the required plugins
	foreach(qt_plugin ${QT_PLUGIN_COMPONENTS})
		target_link_libraries(bitcoin-qt-base Qt5::${qt_plugin})
	endforeach()
endif()

# Localisation
add_subdirectory(locale)

add_custom_command(OUTPUT temp_bitcoin_locale.qrc
	COMMAND cmake
	ARGS
		-E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/bitcoin_locale.qrc"
		temp_bitcoin_locale.qrc
	MAIN_DEPENDENCY bitcoin_locale.qrc
	VERBATIM
)

add_custom_command(OUTPUT qrc_bitcoin_locale.cpp
	COMMAND Qt5::rcc
	ARGS
		temp_bitcoin_locale.qrc
		-name bitcoin_locale
		-o qrc_bitcoin_locale.cpp
	MAIN_DEPENDENCY temp_bitcoin_locale.qrc
	DEPENDS locales
	VERBATIM
)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
if(BUILD_WALLET)
  file(GLOB QT_BASE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
  source_group("BaseHeaders" FILES ${QT_BASE_HEADERS})
endif()
endif()



target_compile_definitions(bitcoin-qt-base PUBLIC OBJC_OLD_DISPATCH_PROTOTYPES)


# Windows support
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_sources(bitcoin-qt-base PRIVATE winshutdownmonitor.cpp)
endif()

target_include_directories(bitcoin-qt-base
	PUBLIC
		.
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_BINARY_DIR}/forms
)

# OSX support
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_sources(bitcoin-qt-base PRIVATE 
		macdockiconhandler.mm
		macnotificationhandler.mm
	)
endif()

target_link_libraries(bitcoin-qt-base
  server
  rpcclient
  Qt5::Widgets
  Qt5::Network
  ${QR_LIB}
  bls
  relic_s
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set_property(TARGET bitcoin-qt-base PROPERTY AUTOMOC_MOC_OPTIONS "-DQ_OS_MAC")
endif()

# Wallet
if(BUILD_WALLET)
	# Automoc option.
	set(AUTOMOC_MOC_OPTIONS -DENABLE_WALLET=1)

	# Add wallet functionality to bitcoin-qt
	target_sources(bitcoin-qt-base
		PRIVATE
			addressbookpage.cpp
			addresstablemodel.cpp
			askpassphrasedialog.cpp
			coincontroldialog.cpp
			coincontroltreewidget.cpp
			editaddressdialog.cpp
			openuridialog.cpp
			overviewpage.cpp
			receivecoinsdialog.cpp
			receiverequestdialog.cpp
			recentrequeststablemodel.cpp
			sendcoinsdialog.cpp
			sendcoinsentry.cpp
			rewardcontroldialog.cpp
			rewardcontroltreewidget.cpp
			rewardsdialog.cpp
			rewardsentry.cpp
			signverifymessagedialog.cpp
			transactiondesc.cpp
			transactiondescdialog.cpp
			transactionfilterproxy.cpp
			transactionrecord.cpp
			transactiontablemodel.cpp
			transactionview.cpp
			walletcontroller.cpp
			walletframe.cpp
			walletmodel.cpp
			walletmodeltransaction.cpp
			walletview.cpp
	)

	target_link_libraries(bitcoin-qt-base wallet)

	if(ENABLE_QRCODE)
		target_include_directories(bitcoin-qt-base PUBLIC ${QRENCODE_INCLUDE_DIR})
		target_link_libraries(bitcoin-qt-base ${QR_LIB})
	endif()
endif()

# The executable
if (DO_PACKAGING)
  add_executable(${APP_NAME} MACOSX_BUNDLE bitcoin.cpp)
else()
  add_executable(${APP_NAME} WIN32 bitcoin.cpp)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${APP_NAME} bitcoin-qt-base "-framework Foundation" "-framework CoreServices" "-framework AppKit")
else()
  target_link_libraries(${APP_NAME} bitcoin-qt-base)
endif()

QT5_USE_Modules(${APP_NAME} Gui)
QT5_USE_Modules(${APP_NAME} Core)
QT5_USE_Modules(${APP_NAME} Widgets)
QT5_USE_Modules(${APP_NAME} Test)
QT5_USE_Modules(${APP_NAME} PrintSupport)
QT5_USE_Modules(${APP_NAME} Network)
if (Qt5DBus_FOUND)
  QT5_USE_Modules(${APP_NAME} DBus)
endif()

set_target_properties(${APP_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../..")


if (BUILD_BUNDLE)
  #############################################################
  # Enable packaging
  INCLUDE(InstallRequiredSystemLibraries)
  
  SET_PROPERTY(TARGET ${APP_NAME} PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../../share/qt/Info.plist.in")
  SET_PROPERTY(TARGET ${APP_NAME} PROPERTY MACOSX_BUNDLE TRUE)
  
  set(APPS "${APP_NAME}.app")
  
  
  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
  SET(CPACK_PACKAGE_NAME ${PROJECT_NAME_SHORT})
  SET(CPACK_PACKAGE_DESCRIPTION "${PROJECT_NAME_LONG}")
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME_SHORT} - ${PROJECT_DESCRIPTION}")
  SET(CPACK_PACKAGE_VENDOR ${PROJECT_VENDOR})
  SET(CPACK_PACKAGE_CONTACT ${PROJECT_CONTACT})
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
  SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  
  SET(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${CPACK_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
  

  add_custom_command(TARGET ${APP_NAME}
    POST_BUILD
    COMMAND macdeployqt ${APPS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  
  
  SET(CPACK_GENERATOR "DragNDrop")
  # Libraries are bundled directly
  SET(CPACK_COMPONENT_LIBRARIES_HIDDEN TRUE)
  SET(MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
  SET(MACOSX_BUNDLE_INFO_STRING "devault-qt")
  SET(MACOSX_BUNDLE_GUI_IDENTIFIER "com.apple.devault-qt")
  SET(MACOSX_BUNDLE_COPYRIGHT "(c) DeVault 2019")
  SET(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
  SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
  SET(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${PROJECT_VERSION}")
  # Bundle Properties
  
  SET(CPACK_PACKAGE_EXECUTABLES "devault-qt" "devault-qt")
  INCLUDE(CPack)
  INSTALL(TARGETS ${APP_NAME} DESTINATION .)
endif()

# Test tests
# add_subdirectory(test)
