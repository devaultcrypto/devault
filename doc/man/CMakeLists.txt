# Copyright (c) 2019 The Bitcoin developers
# Copyright (c) 2019 The DeVault developers

include(GNUInstallDirs)
set(MAN_DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

install(FILES devault.1 DESTINATION "${MAN_DESTINATION}")

if(BUILD_CLI)
	install(FILES devault-cli.1 DESTINATION "${MAN_DESTINATION}")
endif()

if(BUILD_TX)
	install(FILES devault-tx.1 DESTINATION "${MAN_DESTINATION}")
endif()

if(BUILD_QT)
	install(FILES devault-qt.1 DESTINATION "${MAN_DESTINATION}")
endif()
