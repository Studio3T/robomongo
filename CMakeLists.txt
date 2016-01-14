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
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(MongoDB REQUIRED)

# Projects
add_subdirectory(src/third-party/qjson)
add_subdirectory(src/third-party/qscintilla)
add_subdirectory(src/robomongo)


# Show configuration summary and build tip
message("")
message(" _______________[ Robomongo configuration summary ]________________")
message("| ")
message("|-- Dependencies:")
message("| ")
message("|   Qt5 Core:           ${Qt5Core_VERSION}     ${Qt5Core_DIR}")
message("|   Qt5 Gui:            ${Qt5Gui_VERSION}     ${Qt5Gui_DIR}")
message("|   Qt5 Widgets:        ${Qt5Widgets_VERSION}     ${Qt5Widgets_DIR}")
message("|   Qt5 PrintSupport:   ${Qt5Widgets_VERSION}     ${Qt5PrintSupport_DIR}")
message("|   MongoDB:            ${MongoDB_VERSION}     ${MongoDB_BUILD_DIR}")
message("|")
message("|-- Summary:")
message("|")
message("|   System:           ${CMAKE_SYSTEM_NAME}")
message("|   Build type:       ${CMAKE_BUILD_TYPE}")
message("|   Install path:     ${CMAKE_INSTALL_PREFIX}")
message("|")
message("|-- Building and installing:")
message("| ")
message("|   Now you can build Robomongo with the following command:")
message("|     $ cmake --build .")
message("|")
message("|   And install it to ${CMAKE_INSTALL_PREFIX}:")
message("|     $ cmake --build . --target install")
message("|__________________________________________________________________")
message("")
