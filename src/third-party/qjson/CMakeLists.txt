# add_custom_command (OUTPUT ${qjson_SOURCE_DIR}/lib/json_parser.cc
#                    PRE_BUILD
#                    COMMAND bison -t -o json_parser.cc -d json_parser.yy
#                    DEPENDS json_parser.yy
#                    WORKING_DIRECTORY ${qjson_SOURCE_DIR}/lib/
#                    )

# To regenerate json_scanner.cc use:
# flex json_scanner.yy

SET(qjson_MOC_HDRS
  parserrunnable.h
  serializerrunnable.h
)

IF (Qt5Core_FOUND)
    QT5_WRAP_CPP(qjson_MOC_SRCS ${qjson_MOC_HDRS})
ELSE()
    QT4_WRAP_CPP(qjson_MOC_SRCS ${qjson_MOC_HDRS})
ENDIF()

SET (qjson_SRCS parser.cpp qobjecthelper.cpp json_scanner.cpp json_parser.cc parserrunnable.cpp serializer.cpp serializerrunnable.cpp)
SET (qjson_HEADERS parser.h parserrunnable.h qobjecthelper.h serializer.h serializerrunnable.h qjson_export.h)

# Required to use the intree copy of FlexLexer.h
INCLUDE_DIRECTORIES(.)
IF(WIN32)
    ADD_DEFINITIONS(-DQJSON_MAKEDLL)
    ADD_LIBRARY (qjson SHARED ${qjson_SRCS} ${qjson_MOC_SRCS} ${qjson_HEADERS})
ELSE()
    ADD_LIBRARY (qjson STATIC ${qjson_SRCS} ${qjson_MOC_SRCS} ${qjson_HEADERS})
ENDIF()


IF (Qt5Core_FOUND)
  TARGET_LINK_LIBRARIES( qjson ${Qt5Core_LIBRARIES})
ELSE()
  TARGET_LINK_LIBRARIES( qjson ${QT_LIBRARIES})
ENDIF()

#if(NOT android)
        #set_target_properties(qjson PROPERTIES
                                #VERSION ${QJSON_LIB_MAJOR_VERSION}.${QJSON_LIB_MINOR_VERSION}.${QJSON_LIB_PATCH_VERSION}
                                #SOVERSION ${QJSON_LIB_MAJOR_VERSION})
#endif()
#IF(APPLE)
#    set_target_properties(qjson PROPERTIES
#                          DEFINE_SYMBOL QJSON_MAKEDLL
#                          PUBLIC_HEADER "${qjson_HEADERS}"
#                          FRAMEWORK ${OSX_FRAMEWORK})
#ENDIF()

IF(WIN32)
INSTALL(TARGETS qjson RUNTIME DESTINATION . COMPONENT QTLIBS)
#INSTALL(TARGETS qjson EXPORT qjson-export
#   LIBRARY DESTINATION ${LIB_INSTALL_DIR}
#   RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
#   ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
#   FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
#   PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}/qjson
#)
ENDIF()
