IF(WIN32 AND MINGW)
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
        #add_definitions(-D_WIN32_WINNT=0x0601 -DCCH_RM_MAX_APP_NAME=256 -DCCH_RM_MAX_SVC_NAME=256)
ENDIF(WIN32 AND MINGW)

INTEGRATE_BOOST(thread system filesystem)
IF(POSIX)
ADD_SUBDIRECTORY(third-party/qjson)
ENDIF()
ADD_SUBDIRECTORY(third-party/qscintilla/Qt4Qt5)
ADD_SUBDIRECTORY(third-party)
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
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h
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
    robomongo/gui/dialogs/CreateDatabaseDialog.h
    robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h
    robomongo/gui/dialogs/CreateUserDialog.h
    robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h
    robomongo/gui/dialogs/FunctionTextEditor.h
    robomongo/gui/dialogs/AboutDialog.h)
SET(HEADERS_GUI
    robomongo/gui/GuiRegistry.h
    robomongo/gui/editors/PlainJavaScriptEditor.h
    robomongo/gui/editors/JSLexer.h
    robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h
    robomongo/gui/ViewMode.h)
SET(SOURCES_GUI
    robomongo/gui/ExampleWindow.cpp
    robomongo/gui/dialogs/ConnectionsDialog.cpp
    robomongo/gui/dialogs/ConnectionDialog.cpp
    robomongo/gui/widgets/LogWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerTreeWidget.cpp
    robomongo/gui/MainWindow.cpp
    robomongo/gui/GuiRegistry.cpp
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.cpp
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
    robomongo/shell/util/version.cpp
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
    robomongo/core/mongodb/MongoService.h
    robomongo/core/domain/MongoServer.h
    robomongo/core/domain/MongoDatabase.h
    robomongo/core/domain/MongoCollection.h
    robomongo/core/domain/MongoElement.h
    robomongo/core/domain/MongoDocumentIterator.h
    robomongo/core/domain/MongoDocument.h
    robomongo/core/EventBus.h
    robomongo/core/domain/App.h
    robomongo/core/domain/MongoShell.h
    robomongo/core/engine/ScriptEngine.h
    robomongo/core/EventBusDispatcher.h
    robomongo/core/mongodb/MongoWorkerThread.h
    robomongo/core/KeyboardManager.h
    )
SET(HEADERS_CORE
    robomongo/core/EventBusSubscriber.h
    robomongo/core/EventWrapper.h
    robomongo/core/EventError.h
    robomongo/core/Event.h
    robomongo/core/domain/MongoShellResult.h
    robomongo/core/events/MongoEvents.h
    robomongo/core/Wrapper.h
    robomongo/core/mongodb/MongoException.h
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
set(SOURCES_CORE
    robomongo/core/AppRegistry.cpp
    robomongo/core/settings/ConnectionSettings.cpp
    robomongo/core/settings/SettingsManager.cpp
    robomongo/core/examples/ImplicitlyShared.cpp
    robomongo/core/mongodb/MongoWorker.cpp
    robomongo/core/mongodb/MongoService.cpp
    robomongo/core/domain/MongoServer.cpp
    robomongo/core/domain/MongoDatabase.cpp
    robomongo/core/domain/MongoCollection.cpp
    robomongo/core/domain/MongoElement.cpp
    robomongo/core/domain/MongoDocumentIterator.cpp
    robomongo/core/domain/MongoDocument.cpp
    robomongo/core/Wrapper.cpp
    robomongo/core/events/MongoEvents.cpp
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
    robomongo/core/mongodb/MongoWorkerThread.cpp
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

SET(HEADER_LINKER_FIXES third-party/mongodb/src/third_party/pcre-8.30/pcrecpp.h)
SET(SOURCES_LINKER_FIXES
third-party/mongodb/src/mongo/util/assert_util.cpp
third-party/mongodb/src/mongo/util/log.cpp
third-party/mongodb/src/mongo/util/md5.cpp
third-party/mongodb/src/mongo/util/debug_util.cpp
third-party/mongodb/src/mongo/base/status.cpp
third-party/mongodb/src/mongo/bson/oid.cpp
third-party/mongodb/src/mongo/platform/random.cpp
third-party/mongodb/src/mongo/pch.cpp
third-party/mongodb/src/mongo/client/dbclient.cpp)

IF(LINUX)
ADD_DEFINITIONS(-DXP_UNIX)
ELSEIF(MAC_OS)
ELSEIF(WINDOWS)
ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS -DXP_WIN -DUNICODE -DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -DPSAPI_VERSION=1)
ENDIF()
ADD_DEFINITIONS(-DROBOMONGO -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)

IF(LINUX)
SET(PLATFORM_HDRS)
SET(PLATFORM_SRCS robomongo/shell/util/processinfo_linux2.cpp)
SET(PLATFORM_LIBRARYES pthread qjson)
SET(MONGOCLIENT_DEBUG_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/libmongoclient_d.a)
SET(MONGOCLIENT_RELEASE_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/libmongoclient.a)
ELSEIF(WINDOWS)
SET(PLATFORM_HDRS)
SET(PLATFORM_SRCS robomongo/shell/util/processinfo_win32.cpp)
SET(PLATFORM_LIBRARYES ws2_32 kernel32 advapi32 psapi dbghelp user32 gdi32 lwinspool comdlg32 shell32 ole32 oleaut32 odbc32 odbccp32 uuid)
SET(MONGOCLIENT_DEBUG_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/libmongoclient_d.lib)
SET(MONGOCLIENT_RELEASE_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/libmongoclient.lib)
ELSEIF(MAC_OS)
SET(PLATFORM_HDRS)
SET(PLATFORM_SRCS robomongo/shell/util/processinfo_darwin.cpp)
SET(PLATFORM_LIBRARYES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
SET(MONGOCLIENT_DEBUG_LIBRARY)
SET(MONGOCLIENT_RELEASE_LIBRARY)
ENDIF()

SET(HEADERS ${HEADERS_GUI} ${HEADERS_CORE} ${HEADERS_SHELL} ${HEADERS_CORE} ${HEADER_LINKER_FIXES})
SET(SOURCES robomongo/app/main.cpp ${SOURCES_GUI} ${SOURCES_SHELL} ${SOURCES_CORE} ${SOURCES_LINKER_FIXES})

SET(HEADERS_TOMOC ${HEADERS_GUI_TO_MOC} ${HEADERS_CORE_TO_MOC})
QTX_WRAP_CPP(MOC_FILES ${HEADERS_TOMOC})
SET(RESOURCES robomongo/gui/resources/gui.qrc)
QTX_ADD_RESOURCES(RES_SRC ${RESOURCES})
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR} ../include/pcre ../include ../include/qscintilla third-party third-party/mongodb/src/mongo)
INCLUDE_DIRECTORIES(${INCLUDE_DIRS} )
SOURCE_GROUP("Header Files" FILES ${HEADERS})
SOURCE_GROUP("Source Files" FILES ${SOURCES})
SOURCE_GROUP("Reflection Dialog Files" FILES ${REFLECTION_DIALOG_HDRS} ${REFLECTION_DIALOG_SRCS})
SOURCE_GROUP("Generated" FILES ${MOC_FILES})
SOURCE_GROUP("Fileitems Files" FILES ${FILEITEMS_HDRS} ${FILEITEMS_SRCS})
SOURCE_GROUP("Controls" FILES ${CONTROLS_HDRS} ${CONTROLS_SRCS})
SOURCE_GROUP("Platform Files" FILES ${PLATFORM_HDRS} ${PLATFORM_SRCS})
SOURCE_GROUP("Resources" FILES ${RES_SRC})

SET(ALL_SOURCES ${HEADERS} ${SOURCES} ${MOC_FILES} ${PLATFORM_HDRS} ${PLATFORM_SRCS} ${RES_SRC})
SET(ALL_LIBS ${QT_LIBRARIES} ${Boost_LIBRARIES} ${PLATFORM_LIBRARYES} qscintilla2 js pcrecpp)

SET(ALL_DEBUG_LIBS ${ALL_LIBS} ${MONGOCLIENT_DEBUG_LIBRARY})
SET(ALL_RELEASE_LIBS ${ALL_LIBS} ${MONGOCLIENT_RELEASE_LIBRARY})
IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
add_custom_target(BUILD_MONGOCLIENT_RELEASE
COMMAND scons --use-system-boost --usesm mongoclient
COMMAND ${CMAKE_COMMAND} -E copy libmongoclient.a ${MONGOCLIENT_RELEASE_LIBRARY}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
COMMENT "Build mongoclient release library")
add_custom_target(BUILD_MONGOCLIENT_DEBUG
COMMAND scons --use-system-boost --usesm mongoclient
COMMAND ${CMAKE_COMMAND} -E copy libmongoclient.a ${MONGOCLIENT_DEBUG_LIBRARY}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
COMMENT "Build mongoclient debug library")
ADD_APP_EXECUTABLE(${PROJECT_NAME} "${ALL_SOURCES}" "${ALL_DEBUG_LIBS}" "Debug" )
ADD_APP_EXECUTABLE(${PROJECT_NAME} "${ALL_SOURCES}" "${ALL_RELEASE_LIBS}" "Release" )
add_dependencies(${PROJECT_NAME}_d BUILD_MONGOCLIENT_DEBUG)
add_dependencies(${PROJECT_NAME} BUILD_MONGOCLIENT_RELEASE)
ELSE()
ADD_APP_EXECUTABLE_MSVC(${PROJECT_NAME} "${ALL_SOURCES}" "${ALL_DEBUG_LIBS}")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT /DEBUG")
ENDIF()
