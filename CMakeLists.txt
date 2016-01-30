cmake_minimum_required(VERSION 3.1.0)
project(Robomongo)

# Where to look first for CMake modules, before "${CMAKE_ROOT}/Modules" is checked
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Project version
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "9")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION_BUILD "-RC3")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${PROJECT_VERSION_BUILD}")

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
find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(MongoDB REQUIRED)

if (SYSTEM_MACOSX)
    find_package(Qt5MacExtras REQUIRED)
endif()

# Projects
add_subdirectory(src/third-party/qjson)
add_subdirectory(src/third-party/qscintilla)
add_subdirectory(src/robomongo)

# Show configuration summary
include(RobomongoConfigurationSummary)

