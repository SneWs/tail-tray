cmake_minimum_required(VERSION 3.21)

project(tail-tray-tests VERSION 0.2.23 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network LinguistTools Test)

set(PROJECT_SOURCES
    ../src/models/TailAccountInfo.h
    ../src/models/TailDeviceInfo.h
    ../src/models/TailUser.h
    ../src/models/TailNetInfo.h
    ../src/models/TailDriveInfo.h
    ../src/models/TailStatus.h
    ../src/models/TailState.h
    ../src/models/TailDnsStatus.h
    ../src/models/CurrentTailPrefs.h
    ../src/models/JsonHelpers.h
    ../src/models/Models.h
    ../src/models/IpnEvents.h 
    ../src/models/TailPrefsConfig.h 
)

macro(BasicTest TestName)
    string(TOLOWER ${TestName} TestSourceFileName)

    set(targetName "test-${TestName}")
    set(testProjectName "${TestName}")

    add_executable(${targetName} 
        ${PROJECT_SOURCES}
        "src/test-${TestSourceFileName}.cpp"
    )

    target_include_directories(${targetName} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    )


    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data/"
            "$<TARGET_FILE_DIR:${targetName}>/data/"
    )

    target_link_libraries(${targetName} PRIVATE 
        Qt${QT_VERSION_MAJOR}::Widgets 
        Qt${QT_VERSION_MAJOR}::Network 
        Qt${QT_VERSION_MAJOR}::Test
    )

    set_target_properties(${targetName} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

    add_test(NAME ${testProjectName} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" COMMAND $<TARGET_FILE:${targetName}>)
endmacro()

BasicTest(device-info)

BasicTest(tail-status)
