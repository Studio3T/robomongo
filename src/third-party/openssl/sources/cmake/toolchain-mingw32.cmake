
# This is a CMake toolchain file so we can using Mingw to build Windows32 binaries.
# http://vtk.org/Wiki/CMake_Cross_Compiling

# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=./toolchain-mingw32.cmake ../

set( CMAKE_SYSTEM_NAME Windows )
set( CMAKE_SYSTEM_PROCESSOR i686 )

#-----<configuration>-----------------------------------------------

# configure only the lines within this <configure> block, typically

set( TC_PATH /usr/bin )
set( CROSS_COMPILE i686-w64-mingw32- )

# specify the cross compiler
set( CMAKE_C_COMPILER   ${TC_PATH}/${CROSS_COMPILE}gcc )
set( CMAKE_CXX_COMPILER ${TC_PATH}/${CROSS_COMPILE}g++ )
set( CMAKE_RC_COMPILER  ${TC_PATH}/${CROSS_COMPILE}windres )

# where is the target environment
set( CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32 )

#-----</configuration>-----------------------------------------------

# search for programs in the build host directories
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# for libraries and headers in the target directories
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

