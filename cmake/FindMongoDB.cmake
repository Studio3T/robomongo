# Find MongoDB
#
# Find the MongoDB libraries
#
#   This module defines the following variables:
#      MongoDB_FOUND         - True if MONGODB_INCLUDE_DIR & MONGODB_LIBRARY are found
#      MongoDB_LIBRARIES     - Set when MONGODB_LIBRARY is found
#      MongoDB_INCLUDE_DIRS  - Set when MONGODB_INCLUDE_DIR is found
#

find_path(base_dir src/mongo/config.h.in)

if("${CMAKE_BUILD_TYPE}" STREQUAL "" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(build_dir build/debug)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(build_dir build/opt)
endif()

set(include_dirs
    src
    src/third_party/boost-1.56.0
    src/third_party/mozjs-38/include
    src/third_party/mozjs-38/platform/x86_64/linux/include
    src/third_party/pcre-8.37
    ${build_dir}
)

if(UNIX)
    list(APPEND include_dirs src/third_party/mozjs-38/platform/x86_64/linux/include)
elseif(WIN32)
elseif(APPLE)
endif()

# Get tag
execute_process(
    COMMAND git describe --abbrev=0 --tags
    WORKING_DIRECTORY ${base_dir}
    OUTPUT_VARIABLE git_describe
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# handle the QUIETLY and REQUIRED arguments and set ALSA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MongoDB
    REQUIRED_VARS include_dirs base_dir
    VERSION_VAR git_describe
    FAIL_MESSAGE "Could not find MongoDB. Make sure that CMAKE_PREFIX_PATH points to MongoDB project root.\n")

if(MongoDB_FOUND)
    set(MongoDB_VERSION ${git_describe})
    set(MongoDB_DIR ${base_dir})
endif()

#mark_as_advanced()

# Cleanup
unset(base_dir)
unset(build_dir)
unset(git_describe)
unset(src_dir)
