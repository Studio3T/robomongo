CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(js)

#include_directories(mongodb/src/third_party/js-1.7 ../../include/js)
IF(UNIX)
    ADD_DEFINITIONS(-DXP_UNIX -DHAVE_VA_COPY -DVA_COPY=va_copy)
ELSEIF(WIN32)
    ADD_DEFINITIONS(-DXP_WIN)
ENDIF()
ADD_DEFINITIONS(-DROBOMONGO -DJSFILE -DEXPORT_JS_API -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)

SET(HEADERS)

SET(SOURCES
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

SOURCE_GROUP(sources FILES ${HEADERS} ${SOURCES})

ADD_CUSTOM_COMMAND(
    OUTPUT jsautokw.h
    COMMAND ${CMAKE_C_COMPILER} jskwgen.c -o jswgen
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/jswgen > jsautokw.h
    DEPENDS jskwgen.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Regenerating header file jsautokw.h..."
)
ADD_CUSTOM_COMMAND(
    OUTPUT jsautocfg.h
    COMMAND ${CMAKE_C_COMPILER} jscpucfg.c -o jscpucfg
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/jscpucfg > jsautocfg.h
    DEPENDS jscpucfg.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Regenerating header file jsautocfg.h..."
)
ADD_LIBRARY(js STATIC
	${HEADERS}
        ${SOURCES} jsautokw.h jsautocfg.h
)
