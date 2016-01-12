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
if(BUILD_RELEASE OR BUILD_RELWITHDEBINFO)
    set(MongoDB_RELATIVE_BUILD_DIR build/opt)
elseif(BUILD_DEBUG)
    set(MongoDB_RELATIVE_BUILD_DIR build/debug)
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
    ${MongoDB_DIR}/src/third_party/boost-1.56.0
    ${MongoDB_DIR}/src/third_party/mozjs-38/include
    ${MongoDB_DIR}/src/third_party/pcre-8.37
    ${MongoDB_BUILD_DIR}
)

if(SYSTEM_LINUX)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-38/platform/x86_64/linux/include)
elseif(SYSTEM_WINDOWS)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-38/platform/x86_64/windows/include)
elseif(SYSTEM_MACOSX)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-38/platform/x86_64/osx/include)
elseif(SYSTEM_FREEBSD)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-38/platform/x86_64/freebsd/include)
elseif(SYSTEM_OPENBSD)
    list(APPEND MongoDB_INCLUDE_DIRS
        ${MongoDB_DIR}/src/third_party/mozjs-38/platform/x86_64/openbsd/include)
endif()


SET(MongoDB_RELATIVE_LIBS
    mongo/bson/mutable/libmutable_bson.a
    mongo/bson/util/libbson_extract.a
    mongo/client/libauthentication.a
    mongo/client/libclientdriver.a
    mongo/client/libconnection_string.a
    mongo/client/libread_preference.a
    mongo/client/libsasl_client.a
    mongo/crypto/libcrypto_tom.a
    mongo/crypto/libscramauth.a
    mongo/crypto/tom/libtomcrypt.a
    mongo/db/auth/libauthcommon.a
    mongo/db/catalog/libindex_key_validate.a
    mongo/db/commands/libtest_commands_enabled.a
    mongo/db/fts/libbase.a
    mongo/db/fts/unicode/libunicode.a
    mongo/db/geo/libgeometry.a
    mongo/db/geo/libgeoparser.a
    mongo/db/index/libexpression_params.a
    mongo/db/index/libexternal_key_generator.a
    mongo/db/index/libkey_generator.a
    mongo/db/libcommon.a
    mongo/db/libdbmessage.a
    mongo/db/libindex_names.a
    mongo/db/libmongohasher.a
    mongo/db/libnamespace_string.a
    mongo/db/libserver_options_core.a
    mongo/db/libserver_parameters.a
    mongo/db/libservice_context.a
    mongo/db/query/libcommand_request_response.a
    mongo/db/query/liblite_parsed_query.a
    mongo/db/repl/liboptime.a
    mongo/db/repl/libread_concern_args.a
    mongo/executor/libremote_command.a
    mongo/libbase.a
    mongo/liblinenoise_utf8.a
    mongo/libshell_core.a
    mongo/libshell_options.a
    mongo/platform/libplatform.a
    mongo/rpc/libcommand_reply.a
    mongo/rpc/libcommand_request.a
    mongo/rpc/libcommand_status.a
    mongo/rpc/libdocument_range.a
    mongo/rpc/liblegacy_reply.a
    mongo/rpc/liblegacy_request.a
    mongo/rpc/libmetadata.a
    mongo/rpc/libprotocol.a
    mongo/rpc/librpc.a
    mongo/scripting/libbson_template_evaluator.a
    mongo/scripting/libscripting.a
    mongo/scripting/libscripting_common.a
    mongo/shell/libmongojs.a
    mongo/util/concurrency/libspin_lock.a
    mongo/util/concurrency/libsynchronization.a
    mongo/util/concurrency/libticketholder.a
    mongo/util/libbackground_job.a
    mongo/util/libdebugger.a
    mongo/util/libdecorable.a
    mongo/util/libfail_point.a
    mongo/util/libfoundation.a
    mongo/util/libmd5.a
    mongo/util/libpassword.a
    mongo/util/libprocessinfo.a
    mongo/util/libquick_exit.a
    mongo/util/libsafe_num.a
    mongo/util/libsignal_handlers.a
    mongo/util/net/libhostandport.a
    mongo/util/net/libnetwork.a
    mongo/util/options_parser/liboptions_parser.a
    mongo/util/options_parser/liboptions_parser_init.a
    third_party/boost-1.56.0/libboost_chrono.a
    third_party/boost-1.56.0/libboost_filesystem.a
    third_party/boost-1.56.0/libboost_program_options.a
    third_party/boost-1.56.0/libboost_regex.a
    third_party/boost-1.56.0/libboost_system.a
    third_party/boost-1.56.0/libboost_thread.a
    third_party/gperftools-2.2/libtcmalloc_minimal.a
    third_party/libshim_allocator.a
    third_party/libshim_boost.a
    third_party/libshim_mozjs.a
    third_party/libshim_pcrecpp.a
    third_party/libshim_stemmer.a
    third_party/libshim_tz.a
    third_party/libshim_yaml.a
    third_party/libshim_zlib.a
    third_party/libstemmer_c/libstemmer.a
    third_party/mozjs-38/libmozjs.a
    third_party/murmurhash3/libmurmurhash3.a
    third_party/pcre-8.37/libpcrecpp.a
    third_party/s2/base/libbase.a
    third_party/s2/libs2.a
    third_party/s2/strings/libstrings.a
    third_party/s2/util/coding/libcoding.a
    third_party/s2/util/math/libmath.a
    third_party/yaml-cpp-0.5.1/libyaml.a
    third_party/zlib-1.2.8/libzlib.a
)

# Convert to absolute path
foreach(lib ${MongoDB_RELATIVE_LIBS})
  list(APPEND MongoDB_LIBS -l${MongoDB_BUILD_DIR}/${lib})
endforeach()

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
    REQUIRED_VARS MongoDB_DIR MongoDB_BUILD_DIR
    VERSION_VAR MongoDB_RECENT_TAG
    FAIL_MESSAGE "Could not find MongoDB. Make sure that CMAKE_PREFIX_PATH points to MongoDB project root.\n")

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
unset(MongoDB_RELATIVE_BUILD_DIR)
unset(MongoDB_RELATIVE_LIBS)
unset(MongoDB_RECENT_TAG)
