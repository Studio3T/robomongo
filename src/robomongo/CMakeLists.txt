
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
    -DPCRE_STATIC
    -DBOOST_THREAD_VERSION=4
    -DBOOST_THREAD_DONT_PROVIDE_VARIADIC_THREAD
    -DBOOST_THREAD_NO_DEPRECATED
    -DBOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS
    -DBOOST_THREAD_HAS_BUG
    -DMONGO_CONFIG_HAVE_HEADER_UNISTD_H
    -DZZZZZZ
#    -D_GLIBCXX_DEBUG
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

    # Isolated scope #5
    gui/editors/PlainJavaScriptEditor.cpp
    gui/editors/JSLexer.cpp
    gui/editors/FindFrame.cpp
    gui/widgets/explorer/EditIndexDialog.cpp
    gui/widgets/workarea/ScriptWidget.cpp

    # Isolated scope #6
    gui/widgets/explorer/ExplorerCollectionTreeItem.cpp
    gui/widgets/explorer/ExplorerTreeItem.cpp
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


    # More to come here..
    gui/widgets/LogWidget.cpp
    gui/MainWindow.cpp

    gui/resources/gui.qrc

)

SET(STATIC_LIBS
    build/debug/mongo/bson/mutable/mutable_bson
    build/debug/mongo/bson/util/libbson_extract.a
    build/debug/mongo/client/libauthentication.a
    build/debug/mongo/client/libclientdriver.a
    build/debug/mongo/client/libconnection_string.a
    build/debug/mongo/client/libread_preference.a
    build/debug/mongo/client/libsasl_client.a
    build/debug/mongo/crypto/libcrypto_tom.a
    build/debug/mongo/crypto/libscramauth.a
    build/debug/mongo/crypto/tom/libtomcrypt.a
    build/debug/mongo/db/auth/libauthcommon.a
    build/debug/mongo/db/catalog/libindex_key_validate.a
    build/debug/mongo/db/commands/libtest_commands_enabled.a
    build/debug/mongo/db/fts/libbase.a
    build/debug/mongo/db/fts/unicode/libunicode.a
    build/debug/mongo/db/geo/libgeometry.a
    build/debug/mongo/db/geo/libgeoparser.a
    build/debug/mongo/db/index/libexpression_params.a
    build/debug/mongo/db/index/libexternal_key_generator.a
    build/debug/mongo/db/index/libkey_generator.a
    build/debug/mongo/db/libcommon.a
    build/debug/mongo/db/libdbmessage.a
    build/debug/mongo/db/libindex_names.a
    build/debug/mongo/db/libmongohasher.a
    build/debug/mongo/db/libnamespace_string.a
    build/debug/mongo/db/libserver_options_core.a
    build/debug/mongo/db/libserver_parameters.a
    build/debug/mongo/db/libservice_context.a
    build/debug/mongo/db/query/libcommand_request_response.a
    build/debug/mongo/db/query/liblite_parsed_query.a
    build/debug/mongo/db/repl/liboptime.a
    build/debug/mongo/db/repl/libread_concern_args.a
    build/debug/mongo/executor/libremote_command.a
    build/debug/mongo/libbase.a
    build/debug/mongo/liblinenoise_utf8.a
    build/debug/mongo/libshell_core.a
    build/debug/mongo/libshell_options.a
    build/debug/mongo/platform/libplatform.a
    build/debug/mongo/rpc/libcommand_reply.a
    build/debug/mongo/rpc/libcommand_request.a
    build/debug/mongo/rpc/libcommand_status.a
    build/debug/mongo/rpc/libdocument_range.a
    build/debug/mongo/rpc/liblegacy_reply.a
    build/debug/mongo/rpc/liblegacy_request.a
    build/debug/mongo/rpc/libmetadata.a
    build/debug/mongo/rpc/libprotocol.a
    build/debug/mongo/rpc/librpc.a
    build/debug/mongo/scripting/libbson_template_evaluator.a
    build/debug/mongo/scripting/libscripting.a
    build/debug/mongo/scripting/libscripting_common.a
    build/debug/mongo/shell/libmongojs.a
    build/debug/mongo/util/concurrency/libspin_lock.a
    build/debug/mongo/util/concurrency/libsynchronization.a
    build/debug/mongo/util/concurrency/libticketholder.a
    build/debug/mongo/util/libbackground_job.a
    build/debug/mongo/util/libdebugger.a
    build/debug/mongo/util/libdecorable.a
    build/debug/mongo/util/libfail_point.a
    build/debug/mongo/util/libfoundation.a
    build/debug/mongo/util/libmd5.a
    build/debug/mongo/util/libpassword.a
    build/debug/mongo/util/libprocessinfo.a
    build/debug/mongo/util/libquick_exit.a
    build/debug/mongo/util/libsafe_num.a
    build/debug/mongo/util/libsignal_handlers.a
    build/debug/mongo/util/net/libhostandport.a
    build/debug/mongo/util/net/libnetwork.a
    build/debug/mongo/util/options_parser/liboptions_parser.a
    build/debug/mongo/util/options_parser/liboptions_parser_init.a
    build/debug/third_party/boost-1.56.0/libboost_chrono.a
    build/debug/third_party/boost-1.56.0/libboost_filesystem.a
    build/debug/third_party/boost-1.56.0/libboost_program_options.a
    build/debug/third_party/boost-1.56.0/libboost_regex.a
    build/debug/third_party/boost-1.56.0/libboost_system.a
    build/debug/third_party/boost-1.56.0/libboost_thread.a
    build/debug/third_party/gperftools-2.2/libtcmalloc_minimal.a
    build/debug/third_party/libshim_allocator.a
    build/debug/third_party/libshim_boost.a
    build/debug/third_party/libshim_mozjs.a
    build/debug/third_party/libshim_pcrecpp.a
    build/debug/third_party/libshim_stemmer.a
    build/debug/third_party/libshim_tz.a
    build/debug/third_party/libshim_yaml.a
    build/debug/third_party/libshim_zlib.a
    build/debug/third_party/libstemmer_c/libstemmer.a
    build/debug/third_party/mozjs-38/libmozjs.a
    build/debug/third_party/murmurhash3/libmurmurhash3.a
    build/debug/third_party/pcre-8.37/libpcrecpp.a
    build/debug/third_party/s2/base/libbase.a
    build/debug/third_party/s2/libs2.a
    build/debug/third_party/s2/strings/libstrings.a
    build/debug/third_party/s2/util/coding/libcoding.a
    build/debug/third_party/s2/util/math/libmath.a
    build/debug/third_party/yaml-cpp-0.5.1/libyaml.a
    build/debug/third_party/zlib-1.2.8/libzlib.a
)


add_executable(exe app/main.cpp ${FILES})
target_link_libraries(exe Qt5::Widgets qjson-qt5 qscintilla2 -Wl,--whole-archive -Wl,--start-group ${STATIC_LIBS} -Wl,--end-group -Wl,--no-whole-archive m rt dl)

# It works!
# add_executable(exe shell/shell/dbshell.cpp)
# target_link_libraries(exe -Wl,--whole-archive -Wl,--start-group ${STATIC_LIBS} -Wl,--end-group -Wl,--no-whole-archive m rt dl)

message("Include")
print_include_dirs()

message("Link")
print_link_dirs()
#print_vars(include)
