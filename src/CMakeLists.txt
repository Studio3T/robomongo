IF(BOOST_ENABLED)
    INTEGRATE_BOOST(thread system filesystem)
ENDIF()
ADD_SUBDIRECTORY(third-party/qjson)
ADD_SUBDIRECTORY(third-party/qscintilla/Qt4Qt5)
ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/js-1.7)

SET(PCRE_BUILD_TESTS OFF CACHE BOOL "Build the tests pcre tests.")
SET(PCRE_BUILD_PCREGREP OFF CACHE BOOL "Build pcregrep")
ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/pcre-8.30 pcrecpp)

SET(HEADERS_GUI_TO_MOC
    robomongo/gui/dialogs/ConnectionsDialog.h
    robomongo/gui/dialogs/ConnectionDialog.h
    robomongo/gui/widgets/LogWidget.h
    robomongo/gui/widgets/explorer/ExplorerWidget.h
    robomongo/gui/widgets/explorer/ExplorerTreeWidget.h
    robomongo/gui/MainWindow.h
    robomongo/gui/AppStyle.h
    robomongo/gui/widgets/explorer/ExplorerTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h
    robomongo/gui/widgets/explorer/EditIndexDialog.h
    robomongo/gui/widgets/workarea/QueryWidget.h
    robomongo/gui/widgets/workarea/WorkAreaTabWidget.h
    robomongo/gui/widgets/workarea/WorkAreaTabBar.h
    robomongo/gui/widgets/workarea/JsonPrepareThread.h
    robomongo/gui/widgets/workarea/BsonTreeView.h
    robomongo/gui/widgets/workarea/BsonTreeModel.h
    robomongo/gui/widgets/workarea/BsonTreeItem.h
    robomongo/gui/widgets/workarea/BsonTableModel.h
    robomongo/gui/widgets/workarea/BsonTableView.h
    robomongo/gui/widgets/workarea/OutputWidget.h
    robomongo/gui/widgets/workarea/OutputItemContentWidget.h
    robomongo/gui/dialogs/ConnectionAuthTab.h
    robomongo/gui/dialogs/ConnectionBasicTab.h
    robomongo/gui/dialogs/ConnectionAdvancedTab.h
    robomongo/gui/dialogs/ConnectionDiagnosticDialog.h
    robomongo/gui/widgets/workarea/ScriptWidget.h
    robomongo/gui/widgets/workarea/PagingWidget.h
    robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h
    robomongo/gui/widgets/workarea/IndicatorLabel.h
    robomongo/gui/widgets/workarea/ProgressBarPopup.h
    robomongo/gui/dialogs/DocumentTextEditor.h
    robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h
    robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h
    robomongo/gui/dialogs/CreateDatabaseDialog.h
    robomongo/gui/dialogs/CopyCollectionDialog.h
    robomongo/gui/dialogs/CreateUserDialog.h
    robomongo/gui/dialogs/FunctionTextEditor.h
    robomongo/gui/dialogs/PreferencesDialog.h
    robomongo/gui/dialogs/AboutDialog.h
    robomongo/gui/editors/FindFrame.h
    robomongo/gui/editors/PlainJavaScriptEditor.h
    )   
SET(HEADERS_GUI
    robomongo/gui/utils/DialogUtils.h
    robomongo/gui/utils/ComboBoxUtils.h
    robomongo/gui/GuiRegistry.h
    robomongo/gui/editors/JSLexer.h
    robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h
    )
SET(SOURCES_GUI
    robomongo/gui/utils/DialogUtils.cpp
    robomongo/gui/utils/ComboBoxUtils.cpp
    robomongo/gui/dialogs/ConnectionsDialog.cpp
    robomongo/gui/dialogs/ConnectionDialog.cpp
    robomongo/gui/widgets/LogWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerWidget.cpp
    robomongo/gui/widgets/explorer/ExplorerTreeWidget.cpp
    robomongo/gui/MainWindow.cpp
    robomongo/gui/AppStyle.cpp
    robomongo/gui/GuiRegistry.cpp
    robomongo/gui/widgets/explorer/ExplorerTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerServerTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.cpp
    robomongo/gui/widgets/explorer/EditIndexDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.cpp
    robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.cpp
    robomongo/gui/widgets/workarea/QueryWidget.cpp
    robomongo/gui/widgets/workarea/WorkAreaTabWidget.cpp
    robomongo/gui/widgets/workarea/WorkAreaTabBar.cpp
    robomongo/gui/widgets/workarea/JsonPrepareThread.cpp
    robomongo/gui/widgets/workarea/BsonTreeView.cpp
    robomongo/gui/widgets/workarea/BsonTreeModel.cpp
    robomongo/gui/widgets/workarea/BsonTreeItem.cpp
    robomongo/gui/widgets/workarea/BsonTableModel.cpp
    robomongo/gui/widgets/workarea/BsonTableView.cpp
    robomongo/gui/editors/PlainJavaScriptEditor.cpp
    robomongo/gui/editors/JSLexer.cpp
    robomongo/gui/editors/FindFrame.cpp
    robomongo/gui/widgets/workarea/OutputWidget.cpp
    robomongo/gui/widgets/workarea/OutputItemContentWidget.cpp
    robomongo/gui/dialogs/ConnectionAuthTab.cpp
    robomongo/gui/dialogs/ConnectionBasicTab.cpp
    robomongo/gui/dialogs/ConnectionAdvancedTab.cpp
    robomongo/gui/dialogs/ConnectionDiagnosticDialog.cpp
    robomongo/gui/widgets/workarea/ScriptWidget.cpp
    robomongo/gui/widgets/workarea/PagingWidget.cpp
    robomongo/gui/widgets/workarea/OutputItemHeaderWidget.cpp
    robomongo/gui/widgets/workarea/IndicatorLabel.cpp
    robomongo/gui/widgets/workarea/ProgressBarPopup.cpp
    robomongo/gui/dialogs/DocumentTextEditor.cpp
    robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.cpp
    robomongo/gui/widgets/workarea/CollectionStatsTreeItem.cpp
    robomongo/gui/dialogs/CreateDatabaseDialog.cpp
    robomongo/gui/dialogs/CopyCollectionDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerUserTreeItem.cpp
    robomongo/gui/dialogs/CreateUserDialog.cpp
    robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.cpp
    robomongo/gui/dialogs/FunctionTextEditor.cpp
    robomongo/gui/dialogs/PreferencesDialog.cpp
    robomongo/gui/dialogs/AboutDialog.cpp
    )

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
    robomongo/shell/db/ptimeutil.cpp
    )

SET(HEADERS_CORE_TO_MOC
    robomongo/core/settings/ConnectionSettings.h
    robomongo/core/mongodb/MongoWorker.h
    robomongo/core/domain/MongoServer.h
    robomongo/core/domain/MongoDatabase.h
    robomongo/core/EventBus.h
    robomongo/core/domain/App.h
    robomongo/core/domain/MongoShell.h
    robomongo/core/EventBusDispatcher.h 
    robomongo/core/domain/Notifier.h
    robomongo/core/utils/Logger.h
    )
SET(HEADERS_CORE
    robomongo/core/settings/SettingsManager.h
    robomongo/core/engine/ScriptEngine.h
    robomongo/core/KeyboardManager.h
    robomongo/core/domain/MongoDocument.h
    robomongo/core/EventBusSubscriber.h
    robomongo/core/EventWrapper.h
    robomongo/core/EventError.h
    robomongo/core/Event.h
    robomongo/core/domain/MongoShellResult.h
    robomongo/core/events/MongoEvents.h
    robomongo/core/events/MongoEventsInfo.h
    robomongo/core/AppRegistry.h
    robomongo/core/Core.h
    robomongo/core/settings/CredentialSettings.h
    robomongo/core/domain/MongoQueryInfo.h
    robomongo/core/domain/MongoCollection.h
    robomongo/core/domain/MongoCollectionInfo.h
    robomongo/core/mongodb/MongoClient.h
    robomongo/core/domain/MongoUtils.h
    robomongo/core/domain/MongoNamespace.h
    robomongo/core/domain/CursorPosition.h
    robomongo/core/domain/ScriptInfo.h
    robomongo/core/domain/MongoUser.h
    robomongo/core/domain/MongoFunction.h
    robomongo/core/Enums.h
    robomongo/core/HexUtils.h
    robomongo/core/utils/BsonUtils.h
    robomongo/core/utils/QtUtils.h
    robomongo/core/utils/StdUtils.h
    robomongo/core/utils/SingletonPattern.hpp
    )
SET(SOURCES_CORE
    robomongo/core/domain/Notifier.cpp
    robomongo/core/AppRegistry.cpp
    robomongo/core/settings/ConnectionSettings.cpp
    robomongo/core/settings/SettingsManager.cpp
    robomongo/core/mongodb/MongoWorker.cpp
    robomongo/core/domain/MongoServer.cpp
    robomongo/core/domain/MongoDatabase.cpp
    robomongo/core/domain/MongoCollection.cpp
    robomongo/core/domain/MongoDocument.cpp
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
    robomongo/core/domain/MongoUser.cpp
    robomongo/core/domain/MongoFunction.cpp
    robomongo/core/HexUtils.cpp
    robomongo/core/Enums.cpp
    robomongo/core/utils/BsonUtils.cpp
    robomongo/core/utils/QtUtils.cpp
    robomongo/core/utils/StdUtils.cpp
    robomongo/core/utils/Logger.cpp
    )

IF(WIN32)
    ADD_DEFINITIONS(-D_WINSOCKAPI_ -DXP_WIN -DUNICODE -D_UNICODE)
ELSEIF(UNIX)
    ADD_DEFINITIONS(-DXP_UNIX)
ENDIF()
ADD_DEFINITIONS(-DROBOMONGO -DJS_C_STRINGS_ARE_UTF8 -DMONGO_EXPOSE_MACROS)

SET_DESKTOP_TARGET()

SET(LIBMONGOCLIENT_RELEASE_FILENAME ${CMAKE_STATIC_LIBRARY_PREFIX}mongoclient${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(LIBMONGOCLIENT_DEBUG_FILENAME ${CMAKE_STATIC_LIBRARY_PREFIX}mongoclient_d${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(MONGOCLIENT_DEBUG_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${LIBMONGOCLIENT_DEBUG_FILENAME})
SET(MONGOCLIENT_RELEASE_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${LIBMONGOCLIENT_RELEASE_FILENAME})

IF(WIN32)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_win32.cpp)
    SET(PLATFORM_LIBRARIES ws2_32 psapi dbghelp user32)
ELSEIF(APPLE)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_darwin.cpp)
    SET(PLATFORM_LIBRARIES)
ELSEIF(UNIX)
    SET(PLATFORM_HDRS)
    SET(PLATFORM_SRCS robomongo/shell/util/processinfo_linux2.cpp)
    SET(PLATFORM_LIBRARIES pthread)
    #SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF()

IF(BOOST_ENABLED)
    SET(ALL_LIBS ${ALL_LIBS} ${Boost_LIBRARIES})
    SET(MONGODB_EXTRAPATH "${MONGODB_EXTRAPATH} ${BOOST_ROOT}")
ELSE()   
    ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)
    ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/boost)
    SET(INCLUDE_DIRS ${INCLUDE_DIRS} third-party/mongodb/src/third_party/boost)
    SET(ALL_LIBS ${ALL_LIBS} boost_mongo_third_party)
ENDIF()

IF(OPENSSL_ENABLED)
    SET(MONGODB_COMMAND ${MONGODB_COMMAND} --ssl)
    SET(HEADERS_GUI_TO_MOC ${HEADERS_GUI_TO_MOC}
        robomongo/gui/dialogs/ConnectionSslTab.h
    )
    SET(SOURCES_GUI ${SOURCES_GUI}
        robomongo/gui/dialogs/ConnectionSslTab.cpp
    )
    SET(MONGODB_EXTRAPATH "${MONGODB_EXTRAPATH} ${OPENSSL_ROOT_DIR}")
    SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
    ADD_DEFINITIONS(-DMONGO_SSL)
ENDIF(OPENSSL_ENABLED)


IF(SSH_ENABLED)
    SET(MONGODB_COMMAND ${MONGODB_COMMAND} --ssh)
    SET(HEADERS_GUI_TO_MOC ${HEADERS_GUI_TO_MOC}
        robomongo/gui/dialogs/SSHTunnelTab.h
    )
    SET(SOURCES_GUI ${SOURCES_GUI}
        robomongo/gui/dialogs/SSHTunnelTab.cpp
    )
    ADD_SUBDIRECTORY(third-party/mongodb/src/third_party/libssh2)
    SET(ALL_LIBS ${ALL_LIBS} libssh2_third_party)    
ENDIF(SSH_ENABLED)

IF(NOT MONGODB_EXTRAPATH STREQUAL "")
    STRING(REGEX REPLACE "([^\ ]+)" "\\1," MONGODB_EXTRAPATH_OUT ${MONGODB_EXTRAPATH})
    
    # Remove trailing comma
    STRING(REGEX REPLACE ",$" "" MONGODB_EXTRAPATH_OUT_CLEAR ${MONGODB_EXTRAPATH_OUT})    
    
    # Remove spaces
    STRING(REGEX REPLACE "([ ]+)" "" MONGODB_EXTRAPATH_OUT_CLEAR_NO_SPACES ${MONGODB_EXTRAPATH_OUT_CLEAR})    
    
    SET(MONGODB_COMMAND ${MONGODB_COMMAND} --extrapath=${MONGODB_EXTRAPATH_OUT_CLEAR_NO_SPACES})
    SET(ALL_LIBS ${ALL_LIBS} ${OPENSSL_LIBRARIES})
ENDIF()

SET(MONGODB_COMMAND ${MONGODB_COMMAND} --${ARCH_TEXT} mongoclient)

SET(HEADERS ${HEADERS} ${HEADERS_GUI} ${HEADERS_CORE} ${HEADERS_SHELL})
SET(SOURCES ${SOURCES} ${SOURCES_GUI} ${SOURCES_SHELL} ${SOURCES_CORE})
SET(HEADERS_TOMOC ${HEADERS_GUI_TO_MOC} ${HEADERS_CORE_TO_MOC})

QTX_WRAP_CPP(MOC_FILES ${HEADERS_TOMOC})
SET(RESOURCES robomongo/gui/resources/gui.qrc)
QTX_ADD_RESOURCES(RES_SRC ${RESOURCES})
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR} third-party/mongodb/src/third_party/pcre-8.30 third-party/qscintilla/Qt4Qt5 third-party/qjson third-party/mongodb/src)
INCLUDE_DIRECTORIES(${INCLUDE_DIRS} )
SOURCE_GROUP("Header Files" FILES ${HEADERS} ${HEADERS_TOMOC})
SOURCE_GROUP("Source Files" FILES ${SOURCES})
SOURCE_GROUP("Generated" FILES ${MOC_FILES})
SOURCE_GROUP("Platform Files" FILES ${PLATFORM_HDRS} ${PLATFORM_SRCS})
SOURCE_GROUP("Resources" FILES ${RES_SRC} ${RESOURCE_OS})

SET(ALL_SOURCES ${HEADERS} ${HEADERS_TOMOC} ${SOURCES} ${MOC_FILES} ${PLATFORM_HDRS} ${PLATFORM_SRCS} ${BOOST_SOURCES})
SET(ALL_LIBS ${ALL_LIBS} ${QT_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${Qt5Core_QTMAIN_LIBRARIES} ${PLATFORM_LIBRARIES} qscintilla2 pcrecpp qjson js)

SET(ROBOMONGO_LIB "${PROJECT_NAME}_lib")
SET(ROBOMONGO_EXECUTABLE_SOURCES robomongo/app/main.cpp ${RES_SRC} ${ICON_FILE} ${RESOURCE_OS})

IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX OR APPLE)
    IF(BOOST_ENABLED)
            IF ("${CMAKE_BUILD_TYPE_STABLE}" STREQUAL "RELEASE")
                ADD_CUSTOM_TARGET(build_mongoclient
                    COMMAND scons --release ${MONGODB_COMMAND}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                    COMMENT "Build mongoclient library")
            ELSE()
                ADD_CUSTOM_TARGET(build_mongoclient
                    COMMAND scons -d ${MONGODB_COMMAND}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                    COMMENT "Build mongoclient library")
            ENDIF()
        ELSE()
            IF ("${CMAKE_BUILD_TYPE_STABLE}" STREQUAL "RELEASE")
                ADD_CUSTOM_TARGET(build_mongoclient
                    COMMAND scons --release ${MONGODB_COMMAND}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                    COMMENT "Build mongoclient library")
            ELSE()
                ADD_CUSTOM_TARGET(build_mongoclient
                    COMMAND scons --d ${MONGODB_COMMAND}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBMONGOCLIENT_RELEASE_FILENAME}  ${MONGOCLIENT_RELEASE_LIBRARY}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
                    COMMENT "Build mongoclient library")
            ENDIF()
    ENDIF()
    SET(MONGO_LIBRARIES ${MONGOCLIENT_RELEASE_LIBRARY})
    SET(ALL_LIBS ${MONGO_LIBRARIES} ${ALL_LIBS})

    # Creating lib without main.cpp
    ADD_APP_LIBRARY(${ROBOMONGO_LIB} "${ALL_SOURCES}" "${ALL_LIBS}")
    ADD_DEPENDENCIES(${ROBOMONGO_LIB} build_mongoclient)

    # Creating executable
    ADD_APP_EXECUTABLE(${PROJECT_NAME} "${ROBOMONGO_EXECUTABLE_SOURCES}" "${ALL_LIBS}")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ROBOMONGO_LIB})

ELSEIF(MSVC)
    IF(BOOST_ENABLED)
        ADD_CUSTOM_TARGET(mongoclient_release
            COMMAND call scons --release ${MONGODB_COMMAND}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_RELEASE_LIBRARY}\"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
            COMMENT "Build mongoclient release library")
        ADD_CUSTOM_TARGET(mongoclient_debug
            COMMAND call scons --d ${MONGODB_COMMAND}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_DEBUG_LIBRARY}\"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
            COMMENT "Build mongoclient debug library")
    ELSE(BOOST_ENABLED)
        ADD_CUSTOM_TARGET(mongoclient_release
            COMMAND call scons --release ${MONGODB_COMMAND}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_RELEASE_LIBRARY}\"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
            COMMENT "Build mongoclient release library")
        ADD_CUSTOM_TARGET(mongoclient_debug
            COMMAND call scons --d ${MONGODB_COMMAND}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${LIBMONGOCLIENT_RELEASE_FILENAME}\" \"${MONGOCLIENT_DEBUG_LIBRARY}\"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/third-party/mongodb
            COMMENT "Build mongoclient debug library")
    ENDIF(BOOST_ENABLED)
    SET(MONGO_LIBRARIES optimized ${MONGOCLIENT_RELEASE_LIBRARY} debug ${MONGOCLIENT_DEBUG_LIBRARY})
    SET(ALL_LIBS ${MONGO_LIBRARIES} ${ALL_LIBS})

    # Creating lib without main.cpp
    ADD_APP_LIBRARY_MSVC(${ROBOMONGO_LIB} "${ALL_SOURCES}" "${ALL_LIBS}")
    ADD_DEPENDENCIES(${ROBOMONGO_LIB} mongoclient_release)
    ADD_DEPENDENCIES(${ROBOMONGO_LIB} mongoclient_debug)

    # Creating executable
    ADD_APP_EXECUTABLE_MSVC(${PROJECT_NAME} "${ROBOMONGO_EXECUTABLE_SOURCES}" "${ALL_LIBS}")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ROBOMONGO_LIB})
ELSE()
    MESSAGE(FATAL_ERROR "NOT SUPPORTED COMPILER!!!")
ENDIF(MINGW OR CMAKE_COMPILER_IS_GNUCXX OR APPLE)

IF(MANIFEST_ENABLE)
    GET_TARGET_PROPERTY(BINARY_FILE ${PROJECT_NAME} LOCATION)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD COMMAND mt -manifest "${CMAKE_SOURCE_DIR}/install/windows/win7.manifest" -outputresource:"${BINARY_FILE}")
ENDIF(MANIFEST_ENABLE)

IF(WIN32)
    SET_SOURCE_FILES_PROPERTIES(${RESOURCE_OS} PROPERTIES LANGUAGE RC)
ELSEIF(APPLE)
    SET_SOURCE_FILES_PROPERTIES(${RESOURCE_OS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    SET_SOURCE_FILES_PROPERTIES(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    #SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/install/qt.conf PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
ENDIF(WIN32)

GET_FILENAME_COMPONENT(ICON_FILE_NAME ${ICON_FILE} NAME)
IF(APPLE)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/install/osx/Info.plist.in)
    SET(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME} )
    SET(MACOSX_BUNDLE_GUI_IDENTIFIER ${PROJECT_NAME})
    SET(MACOSX_BUNDLE_INFO_STRING "${PROJECT_VERSION},${PROJECT_COPYRIGHT}" )
    SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${SHORT_VERSION}" )
    SET(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
    SET(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    SET(MACOSX_BUNDLE_COPYRIGHT ${PROJECT_COPYRIGHT})
    SET(MACOSX_BUNDLE_ICON_FILE ${ICON_FILE_NAME})
ENDIF(APPLE)

# Start to install
VersionConf(${PROJECT_NAME} ${RESOURCE_OS_IN} ${RESOURCE_OS} ${ICON_FILE_NAME})
#IF(WIN32)
#   SET(CMAKE_EXE_LINKER_FLAGS "/VERSION:\"${MAJOR}.${MINOR}\"" CACHE STRING "Resetting the default linker options." FORCE)
#   STRING(REPLACE "." "," FILEVERSION ${PROJECT_VERSION})
#   SET(VERSION_STR ${PROJECT_VERSION})
#ENDIF()
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${TARGET_INSTALL_DESTINATION} COMPONENT APPLICATIONS)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION . COMPONENT LICENSE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/COPYRIGHT DESTINATION . COMPONENT LICENSE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/whats-new.txt DESTINATION . COMPONENT LICENSE RENAME CHANGELOG)
INSTALL_QT(${PROJECT_NAME} ${LIB_INSTALL_DESTINATION})

IF(WIN32)
    DEPLOY_QT_PLUGIN("platforms/qwindows" ${QT_PLUGINS_DIR} ${LIB_INSTALL_DESTINATION})
    DEPLOY_QT_PLUGIN("imageformats/qgif" ${QT_PLUGINS_DIR} ${LIB_INSTALL_DESTINATION})
    SET(MISC_LIBS icuuc52 icuin52 icudt52 D3DCompiler_43 libEGL libGLESv2)
    SET(PLACE_WHERE_SEARCH ${QT_BINS_DIR})
ELSEIF(APPLE)
    SET(BUNDLE_NAME ${MACOSX_BUNDLE_BUNDLE_NAME}.app)
    DEPLOY_QT_PLUGIN("platforms/qcocoa" ${QT_PLUGINS_DIR} ${BUNDLE_NAME}/Contents/plugins)
    DEPLOY_QT_PLUGIN("accessible/qtaccessiblewidgets" ${QT_PLUGINS_DIR} ${BUNDLE_NAME}/Contents/plugins)
    DEPLOY_QT_PLUGIN("imageformats/qgif" ${QT_PLUGINS_DIR} ${BUNDLE_NAME}/Contents/plugins)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/install/qt.conf DESTINATION ${BUNDLE_NAME}/Contents/Resources COMPONENT RESOURCES)
ELSEIF(UNIX)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/install/linux/README DESTINATION . COMPONENT LICENSE)
    INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/install/linux/${PROJECT_NAME}.sh DESTINATION ${TARGET_INSTALL_DESTINATION})
    INSTALL(FILES ${RESOURCE_OS} DESTINATION share/applications COMPONENT RESOURCES)
    INSTALL(FILES ${ICON_FILE} DESTINATION share/icons COMPONENT RESOURCES)
    DEPLOY_QT_PLUGIN("platforms/qxcb" ${QT_PLUGINS_DIR} ${LIB_INSTALL_DESTINATION})
    DEPLOY_QT_PLUGIN("platformthemes/qgtk2" ${QT_PLUGINS_DIR} ${LIB_INSTALL_DESTINATION})
    DEPLOY_QT_PLUGIN("imageformats/qgif" ${QT_PLUGINS_DIR} ${LIB_INSTALL_DESTINATION})
    SET(MISC_LIBS icuuc49 icudata49 icui18n49 icuuc51 icudata51 icui18n51 icuuc52 icudata52 icui18n52)
    SET(PLACE_WHERE_SEARCH ${QT_LIBS_DIR})
ENDIF()

FOREACH(miscLib ${MISC_LIBS})
    IF(DEVELOPER_QT5)
        DEPLOY_QT_PLUGIN(${miscLib} ${PLACE_WHERE_SEARCH} ${LIB_INSTALL_DESTINATION})
    ENDIF()
ENDFOREACH(miscLib ${MISC_LIBS})

INSTALL_RUNTIME_LIBRARIES()
TARGET_BUNDLEFIX(${PROJECT_NAME} ${LIB_INSTALL_DESTINATION})

IF(DEVELOPER_ENABLE_TESTS)
########## PREPARE GTEST LIBRARY ##########
    IF(MSVC)
        SET(gtest_force_shared_crt ON CACHE BOOL "Build shared gtest library")
        ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
    ENDIF(MSVC)
    ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/tests/gtest-1.7.0 gtest)
    INCLUDE_DIRECTORIES(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
########## PREPARE GTEST LIBRARY ##########

    ADD_EXECUTABLE(tests
        ${CMAKE_SOURCE_DIR}/tests/test_parser.cpp
        ${CMAKE_SOURCE_DIR}/tests/test_connection_string.cpp
        ${CMAKE_SOURCE_DIR}/tests/test_build_collection_query.cpp)

    TARGET_LINK_LIBRARIES(tests ${ROBOMONGO_LIB} gtest gtest_main)
    ADD_TEST(NAME tests COMMAND tests)
    SET_PROPERTY(TARGET tests PROPERTY FOLDER "Tests")

ENDIF(DEVELOPER_ENABLE_TESTS)
