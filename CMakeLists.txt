cmake_minimum_required(VERSION 3.8.0)
project(Robomongo)

# Where to look first for CMake modules, before "${CMAKE_ROOT}/Modules" is checked
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Project version
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "4")
set(PROJECT_VERSION_PATCH "3")
set(PROJECT_VERSION_BUILD "")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${PROJECT_VERSION_BUILD}")

# Enable C++17 features 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Modules
include(RobomongoPrintUtils)
include(RobomongoCMakeDefaults)
include(RobomongoDefaults)
include(RobomongoCommon)
include(RobomongoTrashSymbols)
include(RobomongoTargetArch)
include(RobomongoInstallQt)
include(RobomongoPackage)

# Search for dependencies
find_package(Threading REQUIRED)    # Wrapper arround CMake "Threads" module
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(MongoDB REQUIRED)
find_package(OpenSSL REQUIRED)

if (SYSTEM_MACOSX)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
    find_package(Qt5MacExtras REQUIRED)
endif()

# Projects
set(LIBSSH2_VERSION 1.9.0)
if(SYSTEM_LINUX)
    set(LIBSSH2_VERSION 1.7.0)
endif()
set(LIBSSH2_DIR src/third-party/libssh2-${LIBSSH2_VERSION})
set(QJSON_VERSION 0.8.1)
set(QJSON_DIR src/third-party/qjson-${QJSON_VERSION})
set(QSCINTILLA_VERSION 2.8.4)
set(QSCINTILLA_DIR src/third-party/qscintilla-${QSCINTILLA_VERSION})
set(ESPRIMA_VERSION 2.7.3)
set(ESPRIMA_DIR src/third-party/esprima-${ESPRIMA_VERSION})
# todo: Use equivalent vars from googletest cmake files. Currently they are somehow not working.
set(GOOGLE_TEST_VERSION 1.8.1)
set(GOOGLE_TEST_DIR src/third-party/googletest-${GOOGLE_TEST_VERSION})
add_subdirectory(${LIBSSH2_DIR})
add_subdirectory(src/robomongo/ssh)
add_subdirectory(${QJSON_DIR})
add_subdirectory(${QSCINTILLA_DIR})
add_subdirectory(${GOOGLE_TEST_DIR})
add_subdirectory(src/robomongo)
# todo: Enable unit tests
# add_subdirectory(src/robomongo-unit-tests) # Disabled in Robo 1.4 due to too many link errors

# Show configuration summary
include(RobomongoConfigurationSummary)