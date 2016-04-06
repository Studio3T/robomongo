# This files defines:
#
# 1) Platform checks
#
#    SYSTEM_MACOSX
#    SYSTEM_WINDOWS
#    SYSTEM_LINUX
#    SYSTEM_FREEBSD
#    SYSTEM_OPENBSD
#    SYSTEM_NETBSD
#
# 2) Build types checks
#
#    BUILD_DEBUG
#    BUILD_RELEASE
#    BUILD_RELWITHDEBINFO
#    BUILD_MINSIZEREL
#
# 3) Link "lib groups" and "whole-archive" options
#
#    LINK_LIBGROUP_START
#    LINK_LIBGROUP_END
#    LINK_WHOLE_ARCHIVE_START
#    LINK_WHOLE_ARCHIVE_END

# Platform checks
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(SYSTEM_MACOSX TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(SYSTEM_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SYSTEM_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(SYSTEM_FREEBSD TRUE)
    set(SYSTEM_BSD TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    set(SYSTEM_OPENBSD TRUE)
    set(SYSTEM_BSD TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    set(SYSTEM_NETBSD TRUE)
    set(SYSTEM_BSD TRUE)
endif()

# Build types
if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(BUILD_DEBUG TRUE)
elseif("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    set(BUILD_RELEASE TRUE)
elseif("${CMAKE_BUILD_TYPE}" MATCHES "RelWithDebInfo")
    set(BUILD_RELWITHDEBINFO TRUE)
elseif("${CMAKE_BUILD_TYPE}" MATCHES "MinSizeRel")
    set(BUILD_MINSIZEREL TRUE)
endif()

# Compiler checks
# We check Clang using MATCH instead of strict equality, because as of CMake 3.0.0 the
# CMAKE_<LANG>_COMPILER_ID value for Apple-provided Clang is AppleClang. To test
# for both the Apple-provided Clang and the regular Clang we use MATCH.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(COMPILER_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^GNU$")
    set(COMPILER_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^MSVC$")
    set(COMPILER_MSVC TRUE)
endif()

# Link "groups" and "whole-archive" options
if(SYSTEM_LINUX OR SYSTEM_FREEBSD OR SYSTEM_OPENBSD)
    set(LINK_LIBGROUP_START        -Wl,--start-group)
    set(LINK_LIBGROUP_END          -Wl,--end-group)
    set(LINK_WHOLE_ARCHIVE_START   -Wl,--whole-archive)
    set(LINK_WHOLE_ARCHIVE_END     -Wl,--no-whole-archive)
elseif(SYSTEM_MACOSX)
    set(LINK_LIBGROUP_START        "")
    set(LINK_LIBGROUP_END          "")
    set(LINK_WHOLE_ARCHIVE_START   -Wl,-all_load)
    set(LINK_WHOLE_ARCHIVE_END     -Wl,-noall_load)
endif()
