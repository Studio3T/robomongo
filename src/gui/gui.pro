
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = gui
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += mainwindow.cpp \
    Dialogs/ConnectionsDialog.cpp

HEADERS  += mainwindow.h = \
    Dialogs/ConnectionsDialog.h

INCLUDEPATH += \
            $$ROOT/src/core
