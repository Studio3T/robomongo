#
# Find the MongoDB libraries
#
# This module defines the following variables:
#      MongoDB_FOUND
#      MongoDB_LIBS
#      MongoDB_INCLUDE_DIRS
#      MongoDB_DEFINITIONS
#
# Imported target "mongodb" is created
#
# We assume, that at least "mongo" target was build by the following command:
#    $ scons mongo
#
#

# Try to find MongoDB directory (uses CMAKE_PREFIX_PATH locations)
find_path(
    MongoDB_DIR src/mongo/config.h.in
    DOC "Path to MongoDB (github.com/robomongo-shell) root directory"
)

# Find relative path to build directory
if(BUILD_RELEASE OR BUILD_RELWITHDEBINFO OR BUILD_MINSIZEREL)
    set(MongoDB_RELATIVE_BUILD_DIR build/opt)
    set(MongoDB_OBJECT_LIST_BUILD_TYPE_PART release)
elseif(BUILD_DEBUG)
    set(MongoDB_RELATIVE_BUILD_DIR build/debug)
    set(MongoDB_OBJECT_LIST_BUILD_TYPE_PART debug)
endif()

# Set absolute path to build directory
set(MongoDB_BUILD_DIR ${MongoDB_DIR}/${MongoDB_RELATIVE_BUILD_DIR})

# Set commong compiler definitons
set(MongoDB_DEFINITIONS
    PCRE_STATIC
    BOOST_THREAD_VERSION=4
    BOOST_THREAD_DONT_PROVIDE_VARIADIC_THREAD
    BOOST_THREAD_NO_DEPRECATED
    BOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS
    BOOST_THREAD_HAS_BUG
    MONGO_CONFIG_HAVE_HEADER_UNISTD_H
)

# Set common compiler include directories
set(MongoDB_INCLUDE_DIRS
    ${MongoDB_DIR}/src
    ${MongoDB_DIR}/src/third_party/boost-1.60.0
    ${MongoDB_DIR}/src/third_party/mozjs-45/include
    ${MongoDB_DIR}/src/third_party/mozjs-45/mongo_sources
    ${MongoDB_DIR}/src/third_party/pcre-8.39
    ${MongoDB_BUILD_DIR}
)

if(SYSTEM_LINUX)
    set(MongoDB_OBJECT_LIST_PLATFORM_PART linux)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-45/platform/x86_64/linux/include)
elseif(SYSTEM_WINDOWS)
    set(MongoDB_OBJECT_LIST_PLATFORM_PART windows)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-45/platform/x86_64/windows/include)
elseif(SYSTEM_MACOSX)
    set(MongoDB_OBJECT_LIST_PLATFORM_PART macosx)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-45/platform/x86_64/osx/include)
elseif(SYSTEM_FREEBSD)
    set(MongoDB_OBJECT_LIST_PLATFORM_PART freebsd)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-45/platform/x86_64/freebsd/include)
elseif(SYSTEM_OPENBSD)
    set(MongoDB_OBJECT_LIST_PLATFORM_PART openbsd)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-45/platform/x86_64/openbsd/include)
endif()

# Read list of object files
# See "mongodb/README.md" file (relative to the current folder) for more information.
file(READ
    ${CMAKE_CURRENT_LIST_DIR}/mongodb/${MongoDB_OBJECT_LIST_PLATFORM_PART}-${MongoDB_OBJECT_LIST_BUILD_TYPE_PART}.objects
    MongoDB_RELATIVE_LIBS)

string(STRIP "${MongoDB_RELATIVE_LIBS}" MongoDB_RELATIVE_LIBS)

# Convert string to list
string(REPLACE " " ";" MongoDB_RELATIVE_LIBS ${MongoDB_RELATIVE_LIBS})

# Convert to absolute path
foreach(lib ${MongoDB_RELATIVE_LIBS})
  list(APPEND MongoDB_LIBS ${MongoDB_DIR}/${lib})
endforeach()

if(SYSTEM_WINDOWS)
  list(APPEND MongoDB_LIBS $ENV{WindowsSdkDir}/Lib/winv6.3/um/x64/Crypt32.Lib)
endif()

# Get MongoDB repository recent tag
execute_process(
    COMMAND git describe --abbrev=0 --tags
    WORKING_DIRECTORY ${MongoDB_DIR}
    OUTPUT_VARIABLE MongoDB_RECENT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Handle the QUIETLY and REQUIRED arguments and set ALSA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MongoDB
    FOUND_VAR MongoDB_FOUND     # When using CMake 3.0 MONGODB_FOUND variable will be created.
                                # Make it explicit that variable name is MongoDB_FOUND.
    REQUIRED_VARS MongoDB_DIR MongoDB_BUILD_DIR
    VERSION_VAR MongoDB_RECENT_TAG
    FAIL_MESSAGE "Could not find Robomongo Shell (MongoDB fork). Make sure that CMAKE_PREFIX_PATH points to Robomongo Shell project root.\n")

if(MongoDB_FOUND)
    set(MongoDB_VERSION ${MongoDB_RECENT_TAG})

    # Original MongoDB link command has the following in the end: m rt dl
    set(MongoDB_LIBS
        ${LINK_WHOLE_ARCHIVE_START}   # Linux: -Wl,--whole-archive
        ${LINK_LIBGROUP_START}        # Linux: -Wl,--start-group
        ${MongoDB_LIBS}
        ${LINK_LIBGROUP_END}          # Linux: -Wl,--end-group
        ${LINK_WHOLE_ARCHIVE_END}     # Linux: -Wl,--no-whole-archive
        ${CMAKE_DL_LIBS}              # Linux: dl
    )

    # Add imported target
    add_library(mongodb INTERFACE IMPORTED)

    # Specify INTERFACE properties for this target
    set_target_properties(mongodb PROPERTIES
        INTERFACE_LINK_LIBRARIES      "${MongoDB_LIBS}"
        INTERFACE_COMPILE_DEFINITIONS "${MongoDB_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${MongoDB_INCLUDE_DIRS}"
    )

endif()

# Cleanup
unset(MongoDB_OBJECT_LIST_BUILD_TYPE_PART)
unset(MongoDB_OBJECT_LIST_PLATFORM_PART)
unset(MongoDB_RELATIVE_BUILD_DIR)
unset(MongoDB_RELATIVE_LIBS)
unset(MongoDB_RECENT_TAG)
