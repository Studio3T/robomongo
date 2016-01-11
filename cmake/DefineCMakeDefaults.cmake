# Always include srcdir and builddir in include path
# This saves typing ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY} in
# about every subdir
# Since cmake 2.4.0
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Put the include dirs which are in the source or build tree
# before all other include dirs, so the headers in the sources
# are prefered over the already installed ones
# Since cmake 2.4.1
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

# Use colored output
# Since cmake 2.4.0
set(CMAKE_COLOR_MAKEFILE ON)

# Set the default build type to release with debug info
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE STRING
          "Choose the type of build, options are: Debug, Release, RelWithDebInfo, MinSizeRel and None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)"
      FORCE
  )
endif()

# Automatically generate code for Qt moc, uic and rcc files
# Since CMake 2.8.6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
