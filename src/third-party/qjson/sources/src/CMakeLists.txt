# add_custom_command (OUTPUT ${qjson_SOURCE_DIR}/lib/json_parser.cc
#                    PRE_BUILD
#                    COMMAND bison -t -o json_parser.cc -d json_parser.yy
#                    DEPENDS json_parser.yy
#                    WORKING_DIRECTORY ${qjson_SOURCE_DIR}/lib/
#                    )

# To regenerate json_scanner.cc use:
# flex json_scanner.yy

set(qjson_MOC_HDRS
  parserrunnable.h
  serializerrunnable.h
)

IF (NOT Qt5Core_FOUND)
  qt4_wrap_cpp(qjson_MOC_SRCS ${qjson_MOC_HDRS})
ENDIF()

set (qjson_SRCS parser.cpp qobjecthelper.cpp json_scanner.cpp json_parser.cc parserrunnable.cpp serializer.cpp serializerrunnable.cpp)
set (qjson_HEADERS parser.h parserrunnable.h qobjecthelper.h serializer.h serializerrunnable.h qjson_export.h)

# Required to use the intree copy of FlexLexer.h
INCLUDE_DIRECTORIES(.)

# Special hack to fix scanner test compilation on Windows
# JsonScanner class is not for export. So need to make static
# lib what can be compiled against scanner test.
if(WIN32 AND QJSON_BUILD_TESTS AND BUILD_SHARED_LIBS)
  add_library(qjson_scanner STATIC json_scanner.cpp)
endif()

add_library (qjson${QJSON_SUFFIX} ${qjson_SRCS} ${qjson_MOC_SRCS} ${qjson_HEADERS})
IF (Qt5Core_FOUND)
  target_link_libraries( qjson${QJSON_SUFFIX} ${Qt5Core_LIBRARIES})
ELSE()
  target_link_libraries( qjson${QJSON_SUFFIX} ${QT_LIBRARIES})
ENDIF()

if(NOT ANDROID)
	set_target_properties(qjson${QJSON_SUFFIX} PROPERTIES
	                      VERSION ${QJSON_LIB_MAJOR_VERSION}.${QJSON_LIB_MINOR_VERSION}.${QJSON_LIB_PATCH_VERSION}
                              SOVERSION ${QJSON_LIB_MAJOR_VERSION}
	                      )
endif()

if(NOT BUILD_SHARED_LIBS)
	set_target_properties( qjson${QJSON_SUFFIX} PROPERTIES COMPILE_DEFINITIONS "QJSON_STATIC")
endif()

set_target_properties(qjson${QJSON_SUFFIX} PROPERTIES
                      DEFINE_SYMBOL QJSON_MAKEDLL
                      PUBLIC_HEADER "${qjson_HEADERS}"
                      FRAMEWORK ${OSX_FRAMEWORK}
                      )

INSTALL(TARGETS qjson${QJSON_SUFFIX} EXPORT qjson-export
   LIBRARY DESTINATION ${LIB_INSTALL_DIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
   ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
   FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
   PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}/qjson${QJSON_SUFFIX}
)

if(MSVC)
  get_target_property(LOCATION qjson LOCATION_DEBUG)
  string(REGEX REPLACE "\\.[^.]*$" ".pdb" LOCATION "${LOCATION}")
  install(FILES ${LOCATION} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin CONFIGURATIONS Debug)

  get_target_property(LOCATION qjson LOCATION_RELWITHDEBINFO)
  string(REGEX REPLACE "\\.[^.]*$" ".pdb" LOCATION "${LOCATION}")
  install(FILES ${LOCATION} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin CONFIGURATIONS RelWithDebInfo)
endif(MSVC)
