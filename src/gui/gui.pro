
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = gui
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += ExampleWindow.cpp \
    dialogs/ConnectionsDialog.cpp \
    dialogs/EditConnectionDialog.cpp \
    MainWindow.cpp \
    GuiRegistry.cpp

HEADERS  += ExampleWindow.h = \
    dialogs/ConnectionsDialog.h \
    dialogs/EditConnectionDialog.h \
    MainWindow.h \
    GuiRegistry.h

INCLUDEPATH += \
            $$ROOT/src/core
