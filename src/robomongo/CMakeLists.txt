
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DOS_LINUX -DOS_POSIX)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DOS_MAC -DOS_POSIX)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-D_WIN32 -DOS_WIN)
else()
    message(FATAL_ERROR "Not supported OS: ${CMAKE_SYSTEM_NAME}")
endif()

add_definitions(
    -DPROJECT_NAME="${PROJECT_NAME}"
    -DPROJECT_NAME_TITLE="${PROJECT_NAME_TITLE}"
    -DPROJECT_COPYRIGHT="${PROJECT_COPYRIGHT}"
    -DPROJECT_DOMAIN="${PROJECT_DOMAIN}"
    -DPROJECT_COMPANYNAME="${PROJECT_COMPANYNAME}"
    -DPROJECT_COMPANYNAME_DOMAIN="${PROJECT_COMPANYNAME_DOMAIN}"
    -DPROJECT_GITHUB_FORK="${PROJECT_GITHUB_FORK}"
    -DPROJECT_GITHUB_ISSUES="${PROJECT_GITHUB_ISSUES}"
    -DPROJECT_VERSION="${PROJECT_VERSION}"
    -DPROJECT_NAME_LOWERCASE="${PROJECT_NAME_LOWERCASE}"
)

set(FILES

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
    core/domain/MongoUser.cpp
    core/domain/MongoUser.cpp
    core/domain/MongoFunction.cpp
    core/domain/MongoUtils.cpp
    core/domain/MongoCollection.cpp
    core/domain/MongoCollectionInfo.cpp
    core/domain/MongoQueryInfo.cpp
    core/domain/MongoShellResult.cpp
    core/domain/CursorPosition.cpp
    core/domain/ScriptInfo.cpp
    core/events/MongoEventsInfo.cpp
    shell/db/ptimeutil.cpp

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
    core/settings/SettingsManager.cpp
    core/AppRegistry.cpp

    # Isolated Scope #3
    gui/GuiRegistry.cpp
    gui/dialogs/AboutDialog.cpp
    gui/dialogs/ConnectionAdvancedTab.cpp
    gui/dialogs/ConnectionAuthTab.cpp
    gui/dialogs/ConnectionBasicTab.cpp
    gui/dialogs/ConnectionDiagnosticDialog.cpp
    gui/dialogs/ConnectionDialog.cpp
    gui/dialogs/CopyCollectionDialog.cpp
    gui/widgets/workarea/IndicatorLabel.cpp
    gui/dialogs/CreateDatabaseDialog.cpp
    gui/dialogs/CreateUserDialog.cpp
    gui/utils/ComboBoxUtils.cpp
    gui/utils/DialogUtils.cpp

    # Isolated scope #4
    gui/dialogs/PreferencesDialog.cpp
    gui/dialogs/ConnectionsDialog.cpp
#    gui/widgets/explorer/ExplorerCollectionTreeItem.cpp
#    gui/widgets/explorer/ExplorerTreeItem.cpp
#    gui/widgets/explorer/ExplorerDatabaseTreeItem.cpp


)

add_library(step-by-step ${FILES})
target_link_libraries(step-by-step Qt5::Widgets)

add_executable(exe entry.cpp ${FILES})
target_link_libraries(exe Qt5::Widgets roboshell pthread dl pcre pcrecpp qjson-qt5)

print_include_dirs()
#print_vars(include)
