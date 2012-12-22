
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    AppRegistry.cpp \
    settings/ConnectionRecord.cpp \
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
    Dispatcher.cpp \
    domain/App.cpp \
    domain/MongoShell.cpp \
    engine/ScriptEngine.cpp \
    engine/Result.cpp \
    domain/MongoShellResult.cpp \
    Events.cpp


HEADERS  += \
    AppRegistry.h \
    settings/ConnectionRecord.h \
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
    Dispatcher.h \
    domain/App.h \
    domain/MongoShell.h \
    engine/ScriptEngine.h \
    engine/Result.h \
    domain/MongoShellResult.h \
    Events.h

# third party libs
#LIBS += -L$$THIRDPARTY_LIBS_PATH/qjson
#LIBS += -lqjsond
