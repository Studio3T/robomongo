# (in progress...)
#
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
    ${base_dir}/src
    ${base_dir}/src/third_party/boost-1.56.0
    ${base_dir}/src/third_party/mozjs-38/include
    ${base_dir}/src/third_party/pcre-8.37
    ${base_dir}/${build_dir}
)

if(UNIX)
    list(APPEND include_dirs ${base_dir}/src/third_party/mozjs-38/platform/x86_64/linux/include)
elseif(WIN32)
    # todo
elseif(APPLE)
    # todo
endif()

# Get tag
execute_process(
    COMMAND git describe --abbrev=0 --tags
    WORKING_DIRECTORY ${base_dir}
    OUTPUT_VARIABLE git_describe
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

SET(relative_static_libs
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

foreach(lib ${relative_static_libs})
  list(APPEND static_libs -l${base_dir}/${build_dir}/${lib})
endforeach()

# handle the QUIETLY and REQUIRED arguments and set ALSA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MongoDB
    REQUIRED_VARS include_dirs base_dir
    VERSION_VAR git_describe
    FAIL_MESSAGE "Could not find MongoDB. Make sure that CMAKE_PREFIX_PATH points to MongoDB project root.\n")

set(start -Wl,--whole-archive -Wl,--start-group)
set(end -Wl,--end-group -Wl,--no-whole-archive)

set(definitions
    DPCRE_STATIC
    BOOST_THREAD_VERSION=4
    BOOST_THREAD_DONT_PROVIDE_VARIADIC_THREAD
    BOOST_THREAD_NO_DEPRECATED
    BOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS
    BOOST_THREAD_HAS_BUG
    MONGO_CONFIG_HAVE_HEADER_UNISTD_H
)

if(MongoDB_FOUND)
    set(MongoDB_VERSION ${git_describe})
    set(MongoDB_DIR ${base_dir})
    set(MongoDB_LIBRARIES ${start} ${static_libs} ${end} dl) # Original MongoDB link command has the following in the end: m rt dl
    set(MongoDB_INCLUDE_DIRS ${include_dirs})
    set(MongoDB_COMPILE_DEFINITIONS ${definitions})

    # Add imported target
    add_library(mongodb INTERFACE IMPORTED)

    # Specify INTERFACE properties for this target
    set_target_properties(mongodb PROPERTIES
        INTERFACE_LINK_LIBRARIES      "${MongoDB_LIBRARIES}"
        INTERFACE_COMPILE_DEFINITIONS "${MongoDB_COMPILE_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${MongoDB_INCLUDE_DIRS}"
    )

endif()

# Cleanup
unset(base_dir)
unset(build_dir)
unset(git_describe)
unset(src_dir)
unset(static_libs)
unset(start)
unset(end)
unset(definitions)

