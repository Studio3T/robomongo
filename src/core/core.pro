
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    AppRegistry.cpp \
    settings/ConnectionRecord.cpp \
    settings/SettingsManager.cpp

HEADERS  += \
    AppRegistry.h \
    settings/ConnectionRecord.h \
    settings/SettingsManager.h

