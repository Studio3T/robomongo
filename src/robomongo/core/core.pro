
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    AppRegistry.cpp \
    settings/ConnectionSettings.cpp \
    settings/SettingsManager.cpp \
    examples/ImplicitlyShared.cpp \
    mongodb/MongoWorker.cpp \
    mongodb/MongoService.cpp \
    domain/MongoServer.cpp \
    domain/MongoDatabase.cpp \
    domain/MongoCollection.cpp \
    domain/MongoElement.cpp \
    domain/MongoDocumentIterator.cpp \
    domain/MongoDocument.cpp \
    Wrapper.cpp \
    events/MongoEvents.cpp \
    EventBus.cpp \
    domain/App.cpp \
    domain/MongoShell.cpp \
    engine/ScriptEngine.cpp \
    domain/MongoShellResult.cpp \
    EventError.cpp \
    Event.cpp \
    EventBusSubscriber.cpp \
    EventBusDispatcher.cpp \
    EventWrapper.cpp \
    mongodb/MongoWorkerThread.cpp \
    KeyboardManager.cpp \
    settings/CredentialSettings.cpp \
    domain/MongoQueryInfo.cpp \
    domain/MongoCollectionInfo.cpp \
    mongodb/MongoClient.cpp \
    domain/MongoUtils.cpp \
    domain/MongoNamespace.cpp \
    domain/CursorPosition.cpp \
    domain/ScriptInfo.cpp \
    engine/JsonBuilder.cpp \
    domain/MongoUser.cpp


HEADERS  += \
    AppRegistry.h \
    settings/ConnectionSettings.h \
    settings/SettingsManager.h \
    Core.h \
    examples/ImplicitlyShared.h \
    mongodb/MongoException.h \
    mongodb/MongoWorker.h \
    mongodb/MongoService.h \
    domain/MongoServer.h \
    domain/MongoDatabase.h \
    domain/MongoCollection.h \
    domain/MongoElement.h \
    domain/MongoDocumentIterator.h \
    domain/MongoDocument.h \
    Wrapper.h \
    events/MongoEvents.h \
    EventBus.h \
    domain/App.h \
    domain/MongoShell.h \
    engine/ScriptEngine.h \
    domain/MongoShellResult.h \
    EventError.h \
    Event.h \
    EventBusSubscriber.h \
    EventBusDispatcher.h \
    EventWrapper.h \
    mongodb/MongoWorkerThread.h \
    KeyboardManager.h \
    settings/CredentialSettings.h \
    domain/MongoQueryInfo.h \
    domain/MongoCollectionInfo.h \
    mongodb/MongoClient.h \
    domain/MongoUtils.h \
    domain/MongoNamespace.h \
    domain/CursorPosition.h \
    domain/ScriptInfo.h \
    engine/JsonBuilder.h \
    domain/MongoUser.h

INCLUDEPATH += $$SRC_ROOT

# third party libs
#LIBS += -L$$THIRDPARTY_LIBS_PATH/qjson
#LIBS += -lqjsond
