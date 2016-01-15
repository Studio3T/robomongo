cmake_minimum_required(VERSION 3.1.0)
project(Robomongo)

# Where to look first for CMake modules, before "${CMAKE_ROOT}/Modules" is checked
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Project version
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 9)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_BUILD -dev)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${PROJECT_VERSION_BUILD}")

# Modules
include(PrintUtils)
include(DefineCMakeDefaults)
include(DefineDefaults)
include(DefineTrashSymbols)

# Search for dependencies
find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(MongoDB REQUIRED)

# Projects
add_subdirectory(src/third-party/qjson)
add_subdirectory(src/third-party/qscintilla)
add_subdirectory(src/robomongo)

# Show configuration summary
include(ConfigurationSummary)

#set(CPACK_GENERATOR ZIP)
#set(CPACK_MONOLITHIC_INSTALL ON)
#set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
#SET(CPACK_PACKAGE_EXECUTABLES "robomongo")
#unset(CPACK_INSTALL_CMAKE_PROJECTS)
#include(CPack)

