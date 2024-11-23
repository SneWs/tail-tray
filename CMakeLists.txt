cmake_minimum_required(VERSION 3.5)

project(tail-tray VERSION 0.2.6 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    add_definitions(-DWINDOWS_BUILD)
endif (WIN32)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# Translations
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

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
        src/models/Models.h
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
else ()
    qt_add_executable(tail-tray
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}

    )

endif ()

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en_US sv_SE)

qt_add_translations(tail-tray TS_FILES
    tail_tray_sv_SE.ts
    tail_tray_en_US.ts
)

target_link_libraries(tail-tray PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
endif ()

install(TARGETS tail-tray
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (UNIX AND NOT APPLE)
    install(FILES tail-tray.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )

    install(FILES tail_tray_en_US.ts tail_tray_sv_SE.ts
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/tail-tray
    )

    install(FILES icons/tailscale.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    )
endif ()

qt_finalize_executable(tail-tray)

