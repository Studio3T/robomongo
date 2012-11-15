
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = gui
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += mainwindow.cpp \
    Dialogs/ConnectionsDialog.cpp \
    Dialogs/EditConnectionDialog.cpp

HEADERS  += mainwindow.h = \
    Dialogs/ConnectionsDialog.h \
    Dialogs/EditConnectionDialog.h

INCLUDEPATH += \
            $$ROOT/src/core
