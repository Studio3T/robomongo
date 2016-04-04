# Put the include dirs which are in the source or build tree
# before all other include dirs, so the headers in the sources
# are prefered over the already installed ones
# Since cmake 2.4.1
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

# Use colored output
# Since cmake 2.4.0
set(CMAKE_COLOR_MAKEFILE ON)

# MongoDB compiled with this option
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This variable is used by CMake "FindThreads" module
# From documentation to FindThreads module:
#   > Please note that the compiler flag can only be
#   > used with the imported target. Use of both the imported
#   > target as well as this switch is highly recommended for new code.
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Set the default build type to release with debug info
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo
        CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo, MinSizeRel and None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)"
        FORCE)
endif()

# Do not use Qt moc, uic and rcc by default
# Since CMake 2.8.6
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)
