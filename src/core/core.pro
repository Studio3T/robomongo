
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = core
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += settings/ConnectionRecord.cpp

HEADERS  += settings/ConnectionRecord.h

