
# Automatically generate code for Qt moc, uic and rcc files
# Since CMake 2.8.6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#set(sanitize "-fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${sanitize}")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${sanitize}")

set(SOURCES
    # Isolated Scope #1
    core/utils/QtUtils.cpp
    core/utils/StdUtils.cpp
    core/utils/Logger.cpp
    core/HexUtils.cpp
    core/utils/BsonUtils.cpp
    core/settings/CredentialSettings.cpp
    core/settings/ConnectionSettings.cpp
    core/Event.cpp
    core/Enums.cpp
    core/EventError.cpp
    core/EventBusSubscriber.cpp
    core/EventBusDispatcher.cpp
    core/EventWrapper.cpp
    core/EventBus.cpp
    core/KeyboardManager.cpp
    core/domain/MongoNamespace.cpp
    core/domain/MongoFunction.cpp
    core/domain/MongoUtils.cpp
    core/domain/MongoCollection.cpp
    core/domain/MongoCollectionInfo.cpp
    core/domain/MongoQueryInfo.cpp
    core/domain/CursorPosition.cpp
    core/domain/ScriptInfo.cpp
    core/events/MongoEventsInfo.cpp
    shell/db/ptimeutil.cpp
    shell/bson/json.cpp

    # Isolated Scope #2
    core/engine/ScriptEngine.cpp
    core/events/MongoEvents.cpp
    core/domain/MongoDocument.cpp
    gui/AppStyle.cpp
    core/domain/MongoServer.cpp
    core/domain/MongoShell.cpp
    core/domain/MongoDatabase.cpp
    core/domain/App.cpp
    core/mongodb/MongoClient.cpp
    core/mongodb/MongoWorker.cpp
    core/mongodb/ReplicaSet.cpp
    core/settings/SettingsManager.cpp
    core/AppRegistry.cpp
    utils/StringOperations.cpp
    utils/common.cpp
    utils/SimpleCrypt.cpp
    utils/RoboCrypt.cpp

    # Isolated Scope #3
    gui/GuiRegistry.cpp
    gui/dialogs/AboutDialog.cpp
    gui/dialogs/EulaDialog.cpp
    gui/dialogs/ConnectionAdvancedTab.cpp
    gui/dialogs/ConnectionAuthTab.cpp
    gui/dialogs/ConnectionBasicTab.cpp
    gui/dialogs/ConnectionDiagnosticDialog.cpp
    gui/dialogs/ConnectionDialog.cpp
    gui/dialogs/CopyCollectionDialog.cpp
    gui/widgets/workarea/IndicatorLabel.cpp
    gui/dialogs/CreateCollectionDialog.cpp
    gui/dialogs/CreateDatabaseDialog.cpp
    gui/dialogs/CreateUserDialog.cpp
    gui/utils/ComboBoxUtils.cpp
    gui/utils/DialogUtils.cpp

    # Isolated scope #4
    gui/dialogs/PreferencesDialog.cpp
    gui/dialogs/ConnectionsDialog.cpp
    gui/dialogs/ExportDialog.cpp
    gui/dialogs/ChangeShellTimeoutDialog.cpp

    # Isolated scope #5
    gui/editors/PlainJavaScriptEditor.cpp
    gui/editors/JSLexer.cpp
    gui/editors/FindFrame.cpp
    gui/widgets/explorer/AddEditIndexDialog.cpp
    gui/widgets/workarea/ScriptWidget.cpp

    # Isolated scope #6
    gui/widgets/explorer/ExplorerCollectionTreeItem.cpp
    gui/widgets/explorer/ExplorerCollectionIndexesDir.cpp
    gui/widgets/explorer/ExplorerCollectionIndexItem.cpp
    gui/widgets/explorer/ExplorerTreeItem.cpp
    gui/widgets/explorer/ExplorerReplicaSetFolderItem.cpp
    gui/widgets/explorer/ExplorerReplicaSetTreeItem.cpp
    gui/widgets/explorer/ExplorerDatabaseTreeItem.cpp
    gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.cpp
    gui/widgets/explorer/ExplorerUserTreeItem.cpp
    gui/widgets/explorer/ExplorerFunctionTreeItem.cpp
    gui/dialogs/DocumentTextEditor.cpp
    gui/dialogs/FunctionTextEditor.cpp

    # Isolated scope #7
    gui/widgets/explorer/ExplorerServerTreeItem.cpp
    gui/widgets/explorer/ExplorerTreeWidget.cpp
    gui/widgets/explorer/ExplorerWidget.cpp
    gui/widgets/workarea/BsonTableModel.cpp
    gui/widgets/workarea/BsonTableView.cpp
    gui/widgets/workarea/BsonTreeItem.cpp
    gui/widgets/workarea/BsonTreeModel.cpp
    gui/widgets/workarea/BsonTreeView.cpp
    core/domain/Notifier.cpp

    # Isolated scope #8
    gui/widgets/workarea/CollectionStatsTreeItem.cpp
    gui/widgets/workarea/CollectionStatsTreeWidget.cpp
    gui/widgets/workarea/JsonPrepareThread.cpp
    gui/widgets/workarea/OutputItemContentWidget.cpp
    gui/widgets/workarea/OutputItemHeaderWidget.cpp
    gui/widgets/workarea/OutputWidget.cpp
    gui/widgets/workarea/PagingWidget.cpp
    gui/widgets/workarea/ProgressBarPopup.cpp
    gui/widgets/workarea/QueryWidget.cpp
    gui/widgets/workarea/WorkAreaTabBar.cpp
    gui/widgets/workarea/WorkAreaTabWidget.cpp
    gui/widgets/workarea/WelcomeTab.cpp

    # Final scope
    gui/widgets/LogWidget.cpp
    gui/MainWindow.cpp

    gui/dialogs/SSHTunnelTab.cpp
    gui/dialogs/SSLTab.cpp
    core/settings/SshSettings.cpp
    core/settings/SslSettings.cpp
    core/settings/ReplicaSetSettings.cpp
    core/mongodb/SshTunnelWorker.cpp

    resources/robo.qrc
    gui/resources/gui.qrc
)

# Robomongo target
add_executable(robomongo MACOSX_BUNDLE WIN32 app/main.cpp ${SOURCES})

# Disable WebEngineWidgets for Linux
SET(WebEngineWidgets "Qt5::WebEngineWidgets") 
if(SYSTEM_LINUX)
   SET(WebEngineWidgets)  
endif()
 
target_link_libraries(robomongo
    PRIVATE
        Qt5::Widgets
        Qt5::Network
        Qt5::Xml
        ${WebEngineWidgets}
        qjson
        qscintilla
        mongodb
        ssh
        Threads::Threads)   # Target from FindThreads CMake module

if(APPLE)
  find_library(SECURITY NAMES Security)
  find_library(CORE_FOUNDATION NAMES CoreFoundation)
  set(SSL_LIBRARIES ${SECURITY} ${CORE_FOUNDATION})
  
  target_link_libraries(robomongo
    PRIVATE
        ${SSL_LIBRARIES}
        -lresolv)
endif(APPLE)

target_include_directories(robomongo
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)

# Get build number
find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE BUILD_NUMBER   
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )    
else(GIT_FOUND)
    MESSAGE( FATAL_ERROR "Unable to find Git" )
    set(BUILD_NUMBER 0)
endif(GIT_FOUND)

if(BUILD_NUMBER STREQUAL "")
   MESSAGE( FATAL_ERROR "Failed to get BUILD_NUMBER" )
endif()

set(BUILD_NUMBER ${BUILD_NUMBER} CACHE INTERNAL "")

target_compile_definitions(robomongo
    PRIVATE
        BUILDING_ROBO
        PROJECT_NAME="${PROJECT_NAME}"
        PROJECT_NAME_TITLE="${PROJECT_NAME_TITLE}"
        PROJECT_COPYRIGHT="${PROJECT_COPYRIGHT}"
        PROJECT_DOMAIN="${PROJECT_DOMAIN}"
        PROJECT_COMPANYNAME="${PROJECT_COMPANYNAME}"
        PROJECT_COMPANYNAME_DOMAIN="${PROJECT_COMPANYNAME_DOMAIN}"
        PROJECT_GITHUB_FORK="${PROJECT_GITHUB_FORK}"
        PROJECT_GITHUB_ISSUES="${PROJECT_GITHUB_ISSUES}"
        PROJECT_VERSION="${PROJECT_VERSION}"
        PROJECT_VERSION_SHORT="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        BUILD_NUMBER="${BUILD_NUMBER}"
        PROJECT_NAME_LOWERCASE="${PROJECT_NAME_LOWERCASE}"
        PROJECT_QT_VERSION="${Qt5Core_VERSION}"        
        MongoDB_VERSION="${MongoDB_VERSION}"
        OPENSSL_VERSION="${OPENSSL_VERSION}"
        LIBSSH2_VERSION="${LIBSSH2_VERSION}"
        QJSON_VERSION="${QJSON_VERSION}"
        GOOGLE_TEST_VERSION="${GOOGLE_TEST_VERSION}"
        ESPRIMA_VERSION="${ESPRIMA_VERSION}"
)

if(SYSTEM_WINDOWS)
    # Create Windows Resource file
    set(windows_icon "${CMAKE_SOURCE_DIR}/install/windows/robomongo.ico")
    configure_file(
        "${CMAKE_SOURCE_DIR}/install/windows/winres.rc.in"
        "${CMAKE_BINARY_DIR}/winres.rc")
    unset(windows_icon)

    # Add additional source file to "robomongo" target
    target_sources(robomongo PRIVATE "${CMAKE_BINARY_DIR}/winres.rc")

    # Rename output name for Windows
    set_target_properties(robomongo PROPERTIES OUTPUT_NAME "robo3t")

    # Suppress "warning C4477: 'sprintf'..." from robomongo\shell\db\ptimeutil.cpp
    # Suppress "warning C4291: ...no matching operator delete found" from third_party\mozjs-60
    set_target_properties(robomongo PROPERTIES COMPILE_FLAGS "/wd4477 /wd4291")

    # Start debug program with console
    set_target_properties(robomongo PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")

    # Seems that we do not need to do this with recent versions of Qt
    target_link_libraries(robomongo PRIVATE Qt5::WinMain)

elseif(SYSTEM_MACOSX)
    string(TIMESTAMP CURRENT_YEAR "%Y")
    set_target_properties(robomongo PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        OUTPUT_NAME "Robo 3T"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/install/macosx/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Robo 3T"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.3tsoftwarelabs.robo3t"
        MACOSX_BUNDLE_INFO_STRING "Robo 3T ${PROJECT_VERSION}. Copyright 3T Software Labs Ltd, 2014 - ${CURRENT_YEAR}"
        MACOSX_BUNDLE_SHORT_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_LONG_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright 3T Software Labs Ltd, 2014 - ${CURRENT_YEAR}"
        MACOSX_BUNDLE_ICON_FILE "robomongo")

elseif(SYSTEM_LINUX)
    set_target_properties(robomongo PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib"
        OUTPUT_NAME "robo3t")
endif()

# Install
include(RobomongoInstall)

#
# Tests targets (code below should be moved to separate file)
add_executable(tests WIN32 EXCLUDE_FROM_ALL app/main_test.cpp gui/editors/JSLexer.cpp)
target_link_libraries(tests Qt5::Widgets qjson qscintilla mongodb Threads::Threads)
target_include_directories(tests
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)

# Target that creates original MongoDB shell
# Used to test compilation and linking
add_executable(shell EXCLUDE_FROM_ALL shell/shell/dbshell.cpp)
target_link_libraries(shell
    mongodb
    Threads::Threads)       # Target from FindThreads CMake module
