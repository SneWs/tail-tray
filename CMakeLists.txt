cmake_minimum_required(VERSION 3.5)

project(tail-tray VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h
        src/MainWindow.ui
        resources.qrc
        src/models.h
        src/TrayMenuManager.cpp
        src/TrayMenuManager.h
        src/TailSettings.cpp
        src/TailSettings.h
        src/AccountsTabUiManager.cpp
        src/AccountsTabUiManager.h
        src/SysCommand.cpp
        src/SysCommand.h
        src/KnownValues.h
        src/TailFileReceiver.cpp
        src/TailFileReceiver.h
        src/TailRunner.h
        src/TailRunner.cpp
        src/ManageDriveWindow.h
        src/ManageDriveWindow.cpp
        src/ManageDriveWindow.ui
        src/NetworkStateMonitor.cpp
        src/NetworkStateMonitor.h
)

qt_add_executable(tail-tray
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(tail-tray PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

set_target_properties(tail-tray PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS tail-tray
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES tail-tray.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES icons/tailscale.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(tail-tray)
endif()
