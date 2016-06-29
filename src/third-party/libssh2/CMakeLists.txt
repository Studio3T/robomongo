# Copyright (c) 2014 Alexander Lamaison <alexander.lamaison@gmail.com>
#
# Redistribution and use in source and binary forms,
# with or without modification, are permitted provided
# that the following conditions are met:
#
#   Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
#   Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the following
#   disclaimer in the documentation and/or other materials
#   provided with the distribution.
#
#   Neither the name of the copyright holder nor the names
#   of any other contributors may be used to endorse or
#   promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sources/cmake)

project(libssh2 C)

include(FeatureSummary)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckFunctionExistsMayNeedLibrary)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckNonblockingSocketSupport)
include(SocketLibraries)

set(HAVE_EVP_AES_128_CTR YES)

## Library definition
set(SOURCES
  openssl.c
  openssl.h
  agent.c
  channel.c
  channel.h
  comp.c
  comp.h
  crypt.c
  crypto.h
  global.c
  hostkey.c
  keepalive.c
  kex.c
  knownhost.c
  libssh2_priv.h
  mac.c
  mac.h
  misc.c
  misc.h
  packet.c
  packet.h
  pem.c
  publickey.c
  scp.c
  session.c
  session.h
  sftp.c
  sftp.h
  transport.c
  transport.h
  userauth.c
  userauth.h
  version.c)

if(WIN32)
  list(APPEND SOURCES ../win32/libssh2.rc)
endif()

foreach (source ${SOURCES})
  list (APPEND sources sources/src/${source})
endforeach ()

add_library(libssh2 ${sources})
# we want it to be called libssh2 on all platforms
set_target_properties(libssh2 PROPERTIES PREFIX "")

target_compile_definitions(libssh2
    PRIVATE
        LIBSSH2_OPENSSL)

target_link_libraries(libssh2
    PUBLIC
       ssl crypto)

target_include_directories(libssh2
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sources/include>
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>)

## Options

option(ENABLE_ZLIB_COMPRESSION "Use zlib for compression")
add_feature_info(Compression ENABLE_ZLIB_COMPRESSION
  "using zlib for compression")
if(ENABLE_ZLIB_COMPRESSION)
  find_package(ZLIB REQUIRED)

  target_include_directories(libssh2 PRIVATE ${ZLIB_INCLUDE_DIRS})
  list(APPEND LIBRARIES ${ZLIB_LIBRARIES})
  list(APPEND PC_REQUIRES_PRIVATE zlib)
  if(ZLIB_FOUND)
    target_compile_definitions(libssh2 PRIVATE LIBSSH2_HAVE_ZLIB=1)
  endif()
endif()

# Enable debugging logging by default if the user configured a debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_LOGGING_DEFAULT ON)
else()
  set(DEBUG_LOGGING_DEFAULT OFF)
endif()

option(ENABLE_DEBUG_LOGGING "log execution with debug trace"
  ${DEBUG_LOGGING_DEFAULT})
add_feature_info(Logging ENABLE_DEBUG_LOGGING
   "Logging of execution with debug trace")
if(ENABLE_DEBUG_LOGGING)
  target_compile_definitions(libssh2 PRIVATE LIBSSH2DEBUG)
endif()

## Platform checks
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)

check_include_files(sys/uio.h HAVE_SYS_UIO_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/un.h HAVE_SYS_UN_H)
check_include_files(windows.h HAVE_WINDOWS_H)
check_include_files(ws2tcpip.h HAVE_WS2TCPIP_H)
check_include_files(winsock2.h HAVE_WINSOCK2_H)

check_type_size("long long" LONGLONG)

if(HAVE_SYS_TIME_H)
  check_symbol_exists(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
else()
  check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
endif()
if(HAVE_STDLIB_H)
  check_symbol_exists(strtoll stdlib.h HAVE_STRTOLL)
else()
  check_function_exists(strtoll HAVE_STRTOLL)
endif()
if (NOT HAVE_STRTOLL)
  # Try _strtoi64 if strtoll isn't available
  check_symbol_exists(_strtoi64 stdlib.h HAVE_STRTOI64)
endif()
check_symbol_exists(snprintf stdio.h HAVE_SNPRINTF)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR
   ${CMAKE_SYSTEM_NAME} STREQUAL "Interix")
  # poll() does not work on these platforms
  #
  # Interix: "does provide poll(), but the implementing developer must
  # have been in a bad mood, because poll() only works on the /proc
  # filesystem here"
  #
  # Mac OS X's poll has funny behaviors, like:
  # not being able to do poll on no fildescriptors (10.3?)
  # not being able to poll on some files (like anything in /dev)
  # not having reliable timeout support
  # inconsistent return of POLLHUP where other implementations give POLLIN
  message("poll use is disabled on this platform")
else()
  check_function_exists(poll HAVE_POLL)
endif()

append_needed_socket_libraries(LIBRARIES)

# Non-blocking socket support tests.  Must be after after library tests to
# link correctly
set(SAVE_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${LIBRARIES})
check_nonblocking_socket_support()
set(CMAKE_REQUIRED_LIBRARIES ${SAVE_CMAKE_REQUIRED_LIBRARIES})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/src/libssh2_config_cmake.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/libssh2_config.h)
# to find generated header
target_include_directories(libssh2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Check for the OS.
# Daniel's note: this should not be necessary and we need to work to
# get this removed.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_compile_definitions(libssh2 PRIVATE LIBSSH2_WIN32)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  target_compile_definitions(libssh2 PRIVATE LIBSSH2_DARWIN)
endif()

if(CMAKE_VERSION VERSION_LESS "2.8.12")
  # Fall back to over-linking dependencies
  target_link_libraries(libssh2 ${LIBRARIES})
else()
  target_link_libraries(libssh2 PRIVATE ${LIBRARIES})
endif()

# Ignore single compiler warning for this third-party project
if(COMPILER_MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4133")
endif()
