# This is the qmake file for the QScintilla plugin for Qt Designer.


TEMPLATE = lib
TARGET = qscintillaplugin

CONFIG += release plugin

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer
} else {
    CONFIG += designer
}

macx {
    QMAKE_POST_LINK = install_name_tool -change libqscintilla2.9.dylib $$[QT_INSTALL_LIBS]/libqscintilla2.9.dylib $(TARGET)
}

HEADERS = qscintillaplugin.h
SOURCES = qscintillaplugin.cpp

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target

LIBS += -L$$[QT_INSTALL_LIBS] -lqscintilla2
