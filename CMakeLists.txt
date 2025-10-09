cmake_minimum_required(VERSION 3.21)

project(tail-tray VERSION 0.2.23 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Davfs or not
option(DAVFS_ENABLED "Enable DAVFS" ON)
if (DAVFS_ENABLED)
    message("DAVFS Has been enabled!")
    add_definitions(-DDAVFS_ENABLED)
endif (DAVFS_ENABLED)

if (WIN32)
    add_definitions(-DWINDOWS_BUILD)
endif (WIN32)

get_filename_component(FULL_LIBDIR_PATH
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/tail-tray" ABSOLUTE)

add_definitions(-DTRANSLATIONS_DIR="${FULL_LIBDIR_PATH}")

message(STATUS "Full path of translations install path: ${FULL_LIBDIR_PATH}")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network LinguistTools)

option(BUILD_TESTS "Build tests" ON)

# Use KNotifications
option(KNOTIFICATIONS_ENABLED "Enable KNotifications" OFF)
if (KNOTIFICATIONS_ENABLED)
    message("KNotifications has been enabled! This requires Plasma 6")

    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(KDEInstallDirs)
    include(KDECMakeSettings)

    add_definitions(-DKNOTIFICATIONS_ENABLED)
    find_package(KF6Notifications)
else ()
    message("KNotifications has not been enabled!")
endif (KNOTIFICATIONS_ENABLED)

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow.ui
    resources.qrc
    src/models/TailAccountInfo.h
    src/models/TailDeviceInfo.h
    src/models/TailUser.h
    src/models/TailNetInfo.h
    src/models/TailDriveInfo.h
    src/models/TailStatus.h
    src/models/TailState.h
    src/models/TailDnsStatus.h
    src/models/CurrentTailPrefs.h
    src/models/JsonHelpers.h
    src/models/Models.h
    src/models/IpnEvents.h 
    src/models/TailPrefsConfig.h 
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
    src/TailDriveUiManager.h
    src/TailDriveUiManager.cpp
    src/ManageDriveWindow.h
    src/ManageDriveWindow.cpp
    src/ManageDriveWindow.ui
    src/NetworkStateMonitor.cpp
    src/NetworkStateMonitor.h
    src/AdvertiseRoutesDlg.h
    src/AdvertiseRoutesDlg.cpp
    src/AdvertiseRoutesDlg.ui
    src/DnsSettingsDlg.h
    src/DnsSettingsDlg.cpp
    src/DnsSettingsDlg.ui
    src/IpnWatcher.cpp
    src/IpnWatcher.h
    src/SingleApplicationImpl.h
    src/PleaseWaitDlg.h
    src/PleaseWaitDlg.cpp
    src/PleaseWaitDlg.ui
    src/NotificationsManager.h
    src/NotificationsManager.cpp
    src/ScriptManager.h
    src/ScriptManager.cpp
    src/Scriptfolderwatcher.h
    src/Scriptfolderwatcher.cpp
)

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE tailscale.icns)

    # And the following tells CMake where to find and install the file itself.
    set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/icons/tailscale.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")

    qt_add_executable(tail-tray
            MACOSX_BUNDLE ${app_icon_macos}
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
    )

    set_target_properties(tail-tray PROPERTIES
            ${BUNDLE_ID_OPTION}
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            MACOSX_BUNDLE TRUE)
elseif (WIN32)
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/tail-tray-windows.rc")
    set(WIN_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/tailscale-windows.ico")

    qt_add_executable(tail-tray
            WIN32
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            ${app_icon_resource_windows}
            src/scriptfolderwatcher.h src/scriptfolderwatcher.cpp
    )
else ()
    qt_add_executable(tail-tray
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
    )

endif ()

qt_add_translations(tail-tray TS_FILES
        translations/tail_tray_en_US.ts
        translations/tail_tray_sv_SE.ts
        translations/tail_tray_de_DE.ts
        translations/tail_tray_fr_FR.ts
)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en_US sv_SE de_DE fr_FR)

target_link_libraries(
    tail-tray PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Network 
)

if (KNOTIFICATIONS_ENABLED)
    target_link_libraries(
        tail-tray PRIVATE 
        KF6::Notifications
    )
endif (KNOTIFICATIONS_ENABLED)

configure_file(src/Paths.h.in ${CMAKE_SOURCE_DIR}/src/Paths.h @ONLY)

if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
endif ()

install(TARGETS tail-tray
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (UNIX AND NOT APPLE)
    configure_file(tail-tray.desktop.in
        ${CMAKE_SOURCE_DIR}/tail-tray.desktop
        @ONLY
    )

    install(FILES tail-tray.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )

if (KNOTIFICATIONS_ENABLED)
    install(FILES "tail-tray.notifyrc" DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
endif (KNOTIFICATIONS_ENABLED)

    install(FILES "${CMAKE_BINARY_DIR}/tail_tray_en_US.qm"
            "${CMAKE_BINARY_DIR}/tail_tray_sv_SE.qm"
            "${CMAKE_BINARY_DIR}/tail_tray_fr_FR.qm"
            "${CMAKE_BINARY_DIR}/tail_tray_de_DE.qm"
            DESTINATION "${FULL_LIBDIR_PATH}")

    install(FILES icons/tailscale.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    )
endif ()

qt_finalize_executable(tail-tray)

if (BUILD_TESTS)
    # Add the tests subdirectory so it is built with the main project
    add_subdirectory(tests)
endif ()
