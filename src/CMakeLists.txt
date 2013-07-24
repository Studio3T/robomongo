IF(WIN32 AND MINGW)
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
    #ADD_DEFINITIONS(-D_WIN32_WINNT=0x0601 -DCCH_RM_MAX_APP_NAME=256 -DCCH_RM_MAX_SVC_NAME=256)
ENDIF(WIN32 AND MINGW)
IF(BOOST_ENABLED)
    INTEGRATE_BOOST(thread system filesystem)
ENDIF()
ADD_SUBDIRECTORY(third-party/qjson)
ADD_SUBDIRECTORY(third-party/qscintilla/Qt4Qt5)
ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/js-1.7)
IF(BUILD_PCRECPP_LIBRARY)
    SET(PCRE_BUILD_TESTS OFF CACHE BOOL "Build the tests pcre tests.")
    SET(PCRE_BUILD_PCREGREP OFF CACHE BOOL "Build pcregrep")	
    ADD_SUBDIRECTORY(../include/pcre pcrecpp)
    #FIND_LIBRARY(PCRECPP_LIB pcrecpp)
ENDIF(BUILD_PCRECPP_LIBRARY)

SET(HEADERS_GUI_TO_MOC
    robomongo/gui/ExampleWindow.h
    robomongo/gui/dialogs/ConnectionsDialog.h
    robomongo/gui/dialogs/ConnectionDialog.h
    robomongo/gui/widgets/LogWidget.h
    robomongo/gui/widgets/explorer/ExplorerWidget.h
    robomongo/gui/widgets/explorer/ExplorerTreeWidget.h
    robomongo/gui/MainWindow.h
    robomongo/gui/widgets/explorer/ExplorerTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h
    robomongo/gui/widgets/explorer/EditIndexDialog.h
    robomongo/gui/widgets/workarea/WorkAreaWidget.h
    robomongo/gui/widgets/workarea/QueryWidget.h
    robomongo/gui/widgets/workarea/WorkAreaTabWidget.h
    robomongo/gui/widgets/workarea/WorkAreaTabBar.h
    robomongo/gui/widgets/workarea/BsonWidget.h
    robomongo/gui/widgets/workarea/BsonTreeWidget.h
    robomongo/gui/widgets/workarea/BsonTreeItem.h
    robomongo/gui/editors/jsedit.h
    robomongo/gui/widgets/workarea/OutputWidget.h
    robomongo/gui/widgets/workarea/OutputItemContentWidget.h
    robomongo/gui/examples/CredentialModel.h
    robomongo/gui/dialogs/ConnectionAuthTab.h
    robomongo/gui/dialogs/ConnectionBasicTab.h
    robomongo/gui/dialogs/ConnectionAdvancedTab.h
    robomongo/gui/dialogs/ConnectionDiagnosticDialog.h
    robomongo/gui/widgets/workarea/ScriptWidget.h
    robomongo/gui/widgets/workarea/PagingWidget.h
    robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h
    robomongo/gui/widgets/workarea/OutputItemWidget.h
    robomongo/gui/widgets/workarea/IndicatorLabel.h
    robomongo/gui/widgets/workarea/ProgressBarPopup.h
    robomongo/gui/dialogs/DocumentTextEditor.h
    robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h
    robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h
    robomongo/gui/dialogs/CreateDatabaseDialog.h
    robomongo/gui/dialogs/CreateUserDialog.h
    robomongo/gui/dialogs/FunctionTextEditor.h
    robomongo/gui/dialogs/AboutDialog.h
    robomongo/gui/editors/FindFrame.h)
SET(HEADERS_GUI
    robomongo/gui/utils/DialogUtils.h
    robomongo/gui/GuiRegistry.h
    robomongo/gui/editors/PlainJavaScriptEditor.h
    robomongo/gui/editors/JSLexer.h
    robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h
    robomongo/gui/ViewMode.h)
SET(SOURCES_GUI
    robomongo/gui/utils/DialogUtils.cpp
    robomongo/gui/ExampleWindow.cpp
    robomongo/gui/dialogs/ConnectionsDialog.cpp
    robomongo/gui/dialogs/ConnectionDialog.cpp
    robomongo/gui/widgets/LogWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerTreeWidget.cpp
    robomongo/gui/MainWindow.cpp
    robomongo/gui/GuiRegistry.cpp
    robomongo/gui/widgets/explorer/ExplorerTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.cpp
    robomongo/gui/widgets/explorer/EditIndexDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.cpp
    robomongo/gui/widgets/workarea/WorkAreaWidget.cpp
    robomongo/gui/widgets/workarea/QueryWidget.cpp
    robomongo/gui/widgets/workarea/WorkAreaTabWidget.cpp
    robomongo/gui/widgets/workarea/WorkAreaTabBar.cpp
    robomongo/gui/widgets/workarea/BsonWidget.cpp
    robomongo/gui/widgets/workarea/BsonTreeWidget.cpp
    robomongo/gui/widgets/workarea/BsonTreeItem.cpp
    robomongo/gui/editors/jsedit.cpp
    robomongo/gui/editors/PlainJavaScriptEditor.cpp
    robomongo/gui/editors/JSLexer.cpp
    robomongo/gui/editors/FindFrame.cpp
    robomongo/gui/widgets/workarea/OutputWidget.cpp
    robomongo/gui/widgets/workarea/OutputItemContentWidget.cpp
    robomongo/gui/examples/CredentialModel.cpp
    robomongo/gui/dialogs/ConnectionAuthTab.cpp
    robomongo/gui/dialogs/ConnectionBasicTab.cpp
    robomongo/gui/dialogs/ConnectionAdvancedTab.cpp
    robomongo/gui/dialogs/ConnectionDiagnosticDialog.cpp
    robomongo/gui/widgets/workarea/ScriptWidget.cpp
    robomongo/gui/widgets/workarea/PagingWidget.cpp
    robomongo/gui/widgets/workarea/OutputItemHeaderWidget.cpp
    robomongo/gui/widgets/workarea/OutputItemWidget.cpp
    robomongo/gui/widgets/workarea/IndicatorLabel.cpp
    robomongo/gui/widgets/workarea/ProgressBarPopup.cpp
    robomongo/gui/dialogs/DocumentTextEditor.cpp
    robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.cpp
    robomongo/gui/widgets/workarea/CollectionStatsTreeItem.cpp
    robomongo/gui/dialogs/CreateDatabaseDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerUserTreeItem.cpp
    robomongo/gui/dialogs/CreateUserDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.cpp
    robomongo/gui/dialogs/FunctionTextEditor.cpp
    robomongo/gui/dialogs/AboutDialog.cpp)

SET(HEADERS_SHELL
    robomongo/shell/db/json.h
    robomongo/shell/db/ptimeutil.h)
SET(SOURCES_SHELL
    robomongo/shell/scripting/utils.cpp
    robomongo/shell/scripting/sm_db.cpp
    robomongo/shell/scripting/engine_spidermonkey.cpp
    robomongo/shell/scripting/engine.cpp
    robomongo/shell/mongo.cpp
    #robomongo/shell/util/version.cpp
    robomongo/shell/util/processinfo.cpp
    robomongo/shell/shell/shell_utils.cpp
    robomongo/shell/shell/shell_utils_launcher.cpp
    robomongo/shell/shell/shell_utils_extended.cpp
    robomongo/shell/util/ramlog.cpp
    robomongo/shell/mongo-server.cpp
    robomongo/shell/util/base64.cpp
    robomongo/shell/util/file.cpp
    robomongo/shell/scripting/bench.cpp
    robomongo/shell/scripting/bson_template_evaluator.cpp
    robomongo/shell/db/json.cpp
    robomongo/shell/db/ptimeutil.cpp)

SET(HEADERS_CORE_TO_MOC
    robomongo/core/settings/ConnectionSettings.h
    robomongo/core/settings/SettingsManager.h
    robomongo/core/mongodb/MongoWorker.h
    robomongo/core/domain/MongoServer.h
    robomongo/core/domain/MongoDatabase.h
    robomongo/core/domain/MongoCollection.h
    robomongo/core/domain/MongoElement.h
    robomongo/core/EventBus.h
    robomongo/core/domain/App.h
    robomongo/core/domain/MongoShell.h
    robomongo/core/engine/ScriptEngine.h
    robomongo/core/EventBusDispatcher.h  
    )
SET(HEADERS_CORE
    robomongo/core/KeyboardManager.h
    robomongo/core/domain/MongoDocumentIterator.h
    robomongo/core/domain/MongoDocument.h
    robomongo/core/EventBusSubscriber.h
    robomongo/core/EventWrapper.h
    robomongo/core/EventError.h
    robomongo/core/Event.h
    robomongo/core/domain/MongoShellResult.h
    robomongo/core/events/MongoEvents.h
    robomongo/core/events/MongoEventsInfo.h
    robomongo/core/Wrapper.h
    robomongo/core/AppRegistry.h
    robomongo/core/Core.h
    robomongo/core/examples/ImplicitlyShared.h
    robomongo/core/settings/CredentialSettings.h
    robomongo/core/domain/MongoQueryInfo.h
    robomongo/core/domain/MongoCollectionInfo.h
    robomongo/core/mongodb/MongoClient.h
    robomongo/core/domain/MongoUtils.h
    robomongo/core/domain/MongoNamespace.h
    robomongo/core/domain/CursorPosition.h
    robomongo/core/domain/ScriptInfo.h
    robomongo/core/engine/JsonBuilder.h
    robomongo/core/domain/MongoUser.h
    robomongo/core/domain/MongoFunction.h
    robomongo/core/domain/Enums.h
    robomongo/core/HexUtils.h
    )
SET(SOURCES_CORE
    robomongo/core/AppRegistry.cpp
    robomongo/core/settings/ConnectionSettings.cpp
    robomongo/core/settings/SettingsManager.cpp
    robomongo/core/examples/ImplicitlyShared.cpp
    robomongo/core/mongodb/MongoWorker.cpp
    robomongo/core/domain/MongoServer.cpp
    robomongo/core/domain/MongoDatabase.cpp
    robomongo/core/domain/MongoCollection.cpp
    robomongo/core/domain/MongoElement.cpp
    robomongo/core/domain/MongoDocumentIterator.cpp
    robomongo/core/domain/MongoDocument.cpp
    robomongo/core/Wrapper.cpp
    robomongo/core/events/MongoEvents.cpp
    robomongo/core/events/MongoEventsInfo.cpp
    robomongo/core/EventBus.cpp
    robomongo/core/domain/App.cpp
    robomongo/core/domain/MongoShell.cpp
    robomongo/core/engine/ScriptEngine.cpp
    robomongo/core/domain/MongoShellResult.cpp
    robomongo/core/EventError.cpp
    robomongo/core/Event.cpp
    robomongo/core/EventBusSubscriber.cpp
    robomongo/core/EventBusDispatcher.cpp
    robomongo/core/EventWrapper.cpp
    robomongo/core/KeyboardManager.cpp
    robomongo/core/settings/CredentialSettings.cpp
    robomongo/core/domain/MongoQueryInfo.cpp
    robomongo/core/domain/MongoCollectionInfo.cpp
    robomongo/core/mongodb/MongoClient.cpp
    robomongo/core/domain/MongoUtils.cpp
    robomongo/core/domain/MongoNamespace.cpp
    robomongo/core/domain/CursorPosition.cpp
    robomongo/core/domain/ScriptInfo.cpp
    robomongo/core/engine/JsonBuilder.cpp
    robomongo/core/domain/MongoUser.cpp
    robomongo/core/domain/MongoFunction.cpp
    robomongo/core/HexUtils.cpp)

IF(POSIX_OS)
    ADD_DEFINITIONS(-DXP_UNIX)
ELSEIF(WINDOWS)
    ADD_DEFINITIONS(-D_WINSOCKAPI_ -DXP_WIN -DUNICODE -DPSAPI_VERSION=1)
ENDIF()

SET_DESKTOP_TARGET()

ADD_DEFINITIONS(-DROBOMONGO -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)
SET(LIBMONGOCLIENT_RELEASE_FILENAME ${CMAKE_STATIC_LIBRARY_PREFIX}mongoclient${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(LIBMONGOCLIENT_DEBUG_FILENAME ${CMAKE_STATIC_LIBRARY_PREFIX}mongoclient_d${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(MONGOCLIENT_DEBUG_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${LIBMONGOCLIENT_DEBUG_FILENAME})
SET(MONGOCLIENT_RELEASE_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${LIBMONGOCLIENT_RELEASE_FILENAME})

IF(LINUX_OS)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_linux2.cpp)
    SET(PLATFORM_LIBRARIES pthread)
ELSEIF(WINDOWS)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_win32.cpp)
    SET(PLATFORM_LIBRARIES ws2_32 kernel32 advapi32 psapi dbghelp user32 gdi32 comdlg32 shell32 ole32 oleaut32 odbc32 odbccp32 uuid)
ELSEIF(MAC_OS)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_darwin.cpp)
    SET(PLATFORM_LIBRARIES)
ENDIF()

SET(HEADERS ${HEADERS_GUI} ${HEADERS_CORE} ${HEADERS_SHELL})
SET(SOURCES robomongo/app/main.cpp ${SOURCES_GUI} ${SOURCES_SHELL} ${SOURCES_CORE})

IF(BOOST_ENABLED)
    SET(ALL_LIBS ${ALL_LIBS} ${Boost_LIBRARIES})
ELSE()   
    ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)
    ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/boost)
    INCLUDE_DIRECTORIES(third-party/mongodb/src/third_party/boost)
    SET(ALL_LIBS ${ALL_LIBS} boost_mongo_third_party)
ENDIF()

SET(HEADERS_TOMOC ${HEADERS_GUI_TO_MOC} ${HEADERS_CORE_TO_MOC})
QTX_WRAP_CPP(MOC_FILES ${HEADERS_TOMOC})
SET(RESOURCES robomongo/gui/resources/gui.qrc)
QTX_ADD_RESOURCES(RES_SRC ${RESOURCES})
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR} ../include/pcre ../include third-party/qscintilla/Qt4Qt5 third-party third-party/mongodb/src/mongo)
INCLUDE_DIRECTORIES(${INCLUDE_DIRS} )
SOURCE_GROUP("Header Files" FILES ${HEADERS} ${HEADERS_TOMOC})
SOURCE_GROUP("Source Files" FILES ${SOURCES})
SOURCE_GROUP("Generated" FILES ${MOC_FILES})
SOURCE_GROUP("Fileitems Files" FILES ${FILEITEMS_HDRS} ${FILEITEMS_SRCS})
SOURCE_GROUP("Controls" FILES ${CONTROLS_HDRS} ${CONTROLS_SRCS})
SOURCE_GROUP("Platform Files" FILES ${PLATFORM_HDRS} ${PLATFORM_SRCS})
SOURCE_GROUP("Resources" FILES ${RES_SRC})

SET(ALL_SOURCES ${HEADERS} ${HEADERS_TOMOC} ${SOURCES} ${MOC_FILES} ${PLATFORM_HDRS} ${PLATFORM_SRCS} ${RES_SRC} ${RESOURCE_OS} ${BOOST_SOURCES})
SET(ALL_LIBS ${ALL_LIBS} ${QT_LIBRARIES} ${Qt5Core_QTMAIN_LIBRARIES} ${PLATFORM_LIBRARIES} qscintilla2 pcrecpp qjson js)

IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX OR MAC_OS)
    IF(BOOST_ENABLED)
            IF ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
                ADD_CUSTOM_TARGET(build_mongoclient
                COMMAND scons --release --extrapath=${BOOST_ROOT} --cxx=${CMAKE_CXX_COMPILER} --cc=${CMAKE_C_COMPILER} --${ARCH_TEXT} mongoclient
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                COMMENT "Build mongoclient library")
            ELSE()
                ADD_CUSTOM_TARGET(build_mongoclient
                COMMAND scons -d --extrapath=${BOOST_ROOT} --cxx=${CMAKE_CXX_COMPILER} --cc=${CMAKE_C_COMPILER} --${ARCH_TEXT} mongoclient
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                COMMENT "Build mongoclient library")
            ENDIF()
        ELSE()
            IF ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
                ADD_CUSTOM_TARGET(build_mongoclient
                COMMAND scons --release --${ARCH_TEXT} mongoclient
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                COMMENT "Build mongoclient library")
            ELSE()
                ADD_CUSTOM_TARGET(build_mongoclient
                COMMAND scons --d --${ARCH_TEXT} mongoclient
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                COMMENT "Build mongoclient library")
            ENDIF()
    ENDIF()
    IF(LINUX_OS)
        SET(PROJECT_NAME ${PROJECT_NAME_LOWERCASE})
    ENDIF()
        SET(ALL_LIBS ${ALL_LIBS} ${MONGOCLIENT_RELEASE_LIBRARY})
        ADD_APP_EXECUTABLE(${PROJECT_NAME} "${ALL_SOURCES}" "${ALL_LIBS}")
        ADD_DEPENDENCIES(${PROJECT_NAME} build_mongoclient)
ELSEIF(MSVC)
    IF(BOOST_ENABLED)
        ADD_CUSTOM_TARGET(mongoclient_release
        COMMAND call scons --release --extrapath=${BOOST_ROOT} --cxx=${CMAKE_CXX_COMPILER} --cc=${CMAKE_C_COMPILER} --${ARCH_TEXT} mongoclient
        COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_RELEASE_LIBRARY}\"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
        COMMENT "Build mongoclient release library")
                    ADD_CUSTOM_TARGET(mongoclient_debug
        COMMAND call scons --d --extrapath=${BOOST_ROOT} --cxx=${CMAKE_CXX_COMPILER} --cc=${CMAKE_C_COMPILER} --${ARCH_TEXT} mongoclient
        COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_DEBUG_LIBRARY}\"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
        COMMENT "Build mongoclient debug library")
    ELSE(BOOST_ENABLED)
        ADD_CUSTOM_TARGET(mongoclient_release
        COMMAND call scons --release --${ARCH_TEXT} mongoclient
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_RELEASE_LIBRARY}\"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
        COMMENT "Build mongoclient release library")
                    ADD_CUSTOM_TARGET(mongoclient_debug
        COMMAND call scons --d --${ARCH_TEXT} mongoclient
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_DEBUG_LIBRARY}\"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
    COMMENT "Build mongoclient debug library")
    ENDIF(BOOST_ENABLED)
        SET(MONGO_LIBRARIES optimized ${MONGOCLIENT_RELEASE_LIBRARY} debug ${MONGOCLIENT_DEBUG_LIBRARY})
        SET(ALL_LIBS ${ALL_LIBS} ${MONGO_LIBRARIES})

        ADD_APP_EXECUTABLE_MSVC(${PROJECT_NAME} "${ALL_SOURCES}" "${ALL_LIBS}")
        ADD_DEPENDENCIES(${PROJECT_NAME} mongoclient_release)

        SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT /DEBUG")
        ADD_DEPENDENCIES(${PROJECT_NAME} mongoclient_debug)
        ADD_DLIB_TO_POSTBUILD_STEP(${PROJECT_NAME} qjson)
ELSE(MINGW OR CMAKE_COMPILER_IS_GNUCXX OR MAC_OS)
	MESSAGE(FATAL_ERROR "NOT SUPPORTED COMPILER!!!")
ENDIF(MINGW OR CMAKE_COMPILER_IS_GNUCXX OR MAC_OS)

IF(APPLE)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${MACOSX_BUNDLE_ICON_FILE})
endif(APPLE)
GET_TARGET_PROPERTY(BINARY_FILE ${PROJECT_NAME} LOCATION)
GET_FILENAME_COMPONENT(BINARY_PATH ${BINARY_FILE} PATH)
# Start to install
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION .)
INSTALL_QT(${PROJECT_NAME} ${QT_COMPONENTS_TO_USE})
INSTALL_QT_PLUGINS(${PROJECT_NAME})
INSTALL_RUNTIME_LIBRARIES()

