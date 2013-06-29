cmake_minimum_required(VERSION 2.8)
project(js)

#include_directories(mongodb/src/third_party/js-1.7 ../../include/js)
IF(LINUX)
ADD_DEFINITIONS(-DXP_UNIX)
ELSEIF(MAC_OS)
ELSEIF(WINDOWS)
ADD_DEFINITIONS(-DXP_WIN -DEXPORT_JS_API -DJSFILE -DUNICODE -DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -DPSAPI_VERSION=1)
ENDIF()
ADD_DEFINITIONS(-DROBOMONGO -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)
IF(NOT BOOST_ENABLED)
INCLUDE_DIRECTORIES(mongodb/mongodb/src/third_party/boost)
SET(BOOST_SOURCES mongodb/src/third_party/boost/libs/program_options/src/cmdline.cpp
mongodb/src/third_party/boost/libs/program_options/src/config_file.cpp
mongodb/src/third_party/boost/libs/program_options/src/convert.cpp
mongodb/src/third_party/boost/libs/program_options/src/options_description.cpp
mongodb/src/third_party/boost/libs/program_options/src/parsers.cpp
mongodb/src/third_party/boost/libs/program_options/src/positional_options.cpp
mongodb/src/third_party/boost/libs/program_options/src/split.cpp
mongodb/src/third_party/boost/libs/program_options/src/utf8_codecvt_facet.cpp
mongodb/src/third_party/boost/libs/program_options/src/value_semantic.cpp
mongodb/src/third_party/boost/libs/program_options/src/variables_map.cpp
mongodb/src/third_party/boost/libs/program_options/src/winmain.cpp
mongodb/src/third_party/boost/libs/filesystem/v2/src/v2_operations.cpp
mongodb/src/third_party/boost/libs/filesystem/v2/src/v2_path.cpp
mongodb/src/third_party/boost/libs/filesystem/v2/src/v2_portability.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/codecvt_error_category.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/operations.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/path.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/path_traits.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/portability.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/unique_path.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/utf8_codecvt_facet.cpp
mongodb/src/third_party/boost/libs/filesystem/v3/src/windows_file_codecvt.cpp
mongodb/src/third_party/boost/libs/thread/src/pthread/once.cpp
mongodb/src/third_party/boost/libs/thread/src/pthread/thread.cpp
mongodb/src/third_party/boost/libs/system/src/error_code.cpp)
ENDIF()
set(HEADERS

)
set(SOURCES
mongodb/src/third_party/js-1.7/jsapi.c
mongodb/src/third_party/js-1.7/jsarena.c
mongodb/src/third_party/js-1.7/jsarray.c
mongodb/src/third_party/js-1.7/jsatom.c
mongodb/src/third_party/js-1.7/jsbool.c
mongodb/src/third_party/js-1.7/jscntxt.c
mongodb/src/third_party/js-1.7/jsdate.c
mongodb/src/third_party/js-1.7/jsdbgapi.c
mongodb/src/third_party/js-1.7/jsdhash.c
mongodb/src/third_party/js-1.7/jsdtoa.c
mongodb/src/third_party/js-1.7/jsemit.c
mongodb/src/third_party/js-1.7/jsexn.c
mongodb/src/third_party/js-1.7/jsfun.c
mongodb/src/third_party/js-1.7/jsgc.c
mongodb/src/third_party/js-1.7/jshash.c
mongodb/src/third_party/js-1.7/jsiter.c
mongodb/src/third_party/js-1.7/jsinterp.c
mongodb/src/third_party/js-1.7/jslock.c

mongodb/src/third_party/js-1.7/jslog2.c
mongodb/src/third_party/js-1.7/jslong.c
mongodb/src/third_party/js-1.7/jsmath.c
mongodb/src/third_party/js-1.7/jsnum.c
mongodb/src/third_party/js-1.7/jsobj.c
mongodb/src/third_party/js-1.7/jsopcode.c
mongodb/src/third_party/js-1.7/jsparse.c
mongodb/src/third_party/js-1.7/jsprf.c
mongodb/src/third_party/js-1.7/jsregexp.c
mongodb/src/third_party/js-1.7/jsscan.c
mongodb/src/third_party/js-1.7/jsscope.c
mongodb/src/third_party/js-1.7/jsscript.c
mongodb/src/third_party/js-1.7/jsstr.c
mongodb/src/third_party/js-1.7/jsutil.c
mongodb/src/third_party/js-1.7/jsxdrapi.c
mongodb/src/third_party/js-1.7/jsxml.c
mongodb/src/third_party/js-1.7/prmjtime.c


)

source_group(sources FILES
	${HEADERS}
        ${BOOST_SOURCES}
	${SOURCES}
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
add_custom_command(
    OUTPUT jsautokw.h
    COMMAND gcc ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jskwgen.c -o jswgen
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/jswgen > jsautokw.h
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jskwgen.c
    COMMENT "Regenerating header file jsautokw.h..."
)
add_custom_command(
    OUTPUT jsautocfg.h
    COMMAND gcc ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jscpucfg.c -o jscpucfg
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/jscpucfg > jsautocfg.h
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jscpucfg.c
    COMMENT "Regenerating header file jsautocfg.h..."
)
ELSE()
add_custom_command(
    OUTPUT jsautokw.h
    COMMAND cl ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jskwgen.c -o jswgen
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/jswgen > jsautokw.h
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jskwgen.c
    COMMENT "Regenerating header file jsautokw.h..."
)
add_custom_command(
    OUTPUT jsautocfg.h
    COMMAND cl ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jscpucfg.c -o jscpucfg
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/jscpucfg > jsautocfg.h
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/mongodb/src/third_party/js-1.7/jscpucfg.c
    COMMENT "Regenerating header file jsautocfg.h..."
)
ENDIF()
add_library(js STATIC
	${HEADERS}
        ${SOURCES} ${BOOST_SOURCES} jsautokw.h jsautocfg.h
)
