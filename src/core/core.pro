
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += settings/ConnectionRecord.cpp \
    settings/SettingsManager.cpp

HEADERS  += settings/ConnectionRecord.h \
    settings/SettingsManager.h

