
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    AppRegistry.cpp \
    settings/ConnectionSettings.cpp \
    settings/SettingsManager.cpp \
    examples/ImplicitlyShared.cpp \
    mongodb/MongoClient.cpp \
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
    mongodb/MongoClientThread.cpp \
    KeyboardManager.cpp \
    settings/CredentialSettings.cpp \
    domain/MongoQueryInfo.cpp


HEADERS  += \
    AppRegistry.h \
    settings/ConnectionSettings.h \
    settings/SettingsManager.h \
    Core.h \
    examples/ImplicitlyShared.h \
    mongodb/MongoException.h \
    mongodb/MongoClient.h \
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
    mongodb/MongoClientThread.h \
    KeyboardManager.h \
    settings/CredentialSettings.h \
    domain/MongoQueryInfo.h

INCLUDEPATH += $$SRC_ROOT

# third party libs
#LIBS += -L$$THIRDPARTY_LIBS_PATH/qjson
#LIBS += -lqjsond
