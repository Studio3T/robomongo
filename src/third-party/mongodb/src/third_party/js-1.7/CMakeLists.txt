cmake_minimum_required(VERSION 2.8)
project(js)

#include_directories(mongodb/src/third_party/js-1.7 ../../include/js)
IF(LINUX)
ADD_DEFINITIONS(-DXP_UNIX)
ELSEIF(APPLE)
ELSEIF(WINDOWS)
ADD_DEFINITIONS(-DXP_WIN -DEXPORT_JS_API -DJSFILE -DUNICODE -D_CRT_SECURE_NO_WARNINGS -DPSAPI_VERSION=1)
ENDIF()
ADD_DEFINITIONS(-DROBOMONGO -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)
set(HEADERS

)
set(SOURCES
jsapi.c
jsarena.c
jsarray.c
jsatom.c
jsbool.c
jscntxt.c
jsdate.c
jsdbgapi.c
jsdhash.c
jsdtoa.c
jsemit.c
jsexn.c
jsfun.c
jsgc.c
jshash.c
jsiter.c
jsinterp.c
jslock.c

jslog2.c
jslong.c
jsmath.c
jsnum.c
jsobj.c
jsopcode.c
jsparse.c
jsprf.c
jsregexp.c
jsscan.c
jsscope.c
jsscript.c
jsstr.c
jsutil.c
jsxdrapi.c
jsxml.c
prmjtime.c
)
source_group(sources FILES
	${HEADERS}
	${SOURCES}
)
IF(MSVC)
SET(BUILD_DIR_JS_GEN )
ELSE()
SET(BUILD_DIR_JS_GEN ${CMAKE_CURRENT_BINARY_DIR}/)
ENDIF()
add_custom_command(
    OUTPUT jsautokw.h
    COMMAND ${CMAKE_C_COMPILER} jskwgen.c -o jswgen
    COMMAND ${BUILD_DIR_JS_GEN}jswgen > jsautokw.h
    DEPENDS jskwgen.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Regenerating header file jsautokw.h..."
)
add_custom_command(
    OUTPUT jsautocfg.h
    COMMAND ${CMAKE_C_COMPILER} jscpucfg.c -o jscpucfg
    COMMAND ${BUILD_DIR_JS_GEN}jscpucfg > jsautocfg.h
    DEPENDS jscpucfg.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Regenerating header file jsautocfg.h..."
)
add_library(js STATIC
	${HEADERS}
        ${SOURCES} jsautokw.h jsautocfg.h
)
