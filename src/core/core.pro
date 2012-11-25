
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    AppRegistry.cpp \
    settings/ConnectionRecord.cpp \
    settings/SettingsManager.cpp \
    examples/ImplicitlyShared.cpp \
    mongodb/MongoManager.cpp \
    mongodb/MongoServer.cpp \
    mongodb/MongoDatabase.cpp \
    Wrapper.cpp \
    mongodb/MongoClient.cpp \
    mongodb/MongoCollection.cpp \
    events/MongoEvents.cpp \
    Dispatcher.cpp

HEADERS  += \
    AppRegistry.h \
    settings/ConnectionRecord.h \
    settings/SettingsManager.h \
    Core.h \
    examples/ImplicitlyShared.h \
    mongodb/MongoManager.h \
    mongodb/MongoServer.h \
    mongodb/MongoException.h \
    mongodb/MongoDatabase.h \
    Wrapper.h \
    mongodb/MongoClient.h \
    mongodb/MongoCollection.h \
    events/MongoEvents.h \
    Dispatcher.h

message(inside core huevo: $$THIRDPARTY_LIBS_PATH/qjson)

# third party libs
LIBS += -L$$THIRDPARTY_LIBS_PATH/qjson
LIBS += -lqjsond
