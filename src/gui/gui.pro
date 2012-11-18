
! include( ../common.pri ) { error( Error: couldnt find the common.pri file. ) }

TARGET   = gui
TEMPLATE = lib
CONFIG  += staticlib

SOURCES += ExampleWindow.cpp \
    dialogs/ConnectionsDialog.cpp \
    dialogs/EditConnectionDialog.cpp \
    widgets/LogWidget.cpp \
    widgets/explorer/ExplorerWidget.cpp \
    widgets/explorer/ExplorerTreeWidget.cpp \
    MainWindow.cpp \
    GuiRegistry.cpp \
    widgets/explorer/ExplorerServerTreeItem.cpp \
    widgets/explorer/ExplorerDatabaseTreeItem.cpp \
    widgets/explorer/ExplorerDatabaseCategoryTreeItem.cpp \
    widgets/explorer/ExplorerCollectionTreeItem.cpp

HEADERS  += ExampleWindow.h = \
    dialogs/ConnectionsDialog.h \
    dialogs/EditConnectionDialog.h \
    widgets/LogWidget.h \
    widgets/explorer/ExplorerWidget.h \
    widgets/explorer/ExplorerTreeWidget.h \
    MainWindow.h \
    GuiRegistry.h \
    widgets/explorer/ExplorerServerTreeItem.h \
    widgets/explorer/ExplorerDatabaseTreeItem.h \
    widgets/explorer/ExplorerDatabaseCategoryTreeItem.h \
    widgets/explorer/ExplorerCollectionTreeItem.h

INCLUDEPATH += \
            $$ROOT/src/core
