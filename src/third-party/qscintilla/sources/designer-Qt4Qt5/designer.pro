# This is the qmake file for the QScintilla plugin for Qt Designer.


TEMPLATE = lib
TARGET = qscintillaplugin

CONFIG += release plugin qscintilla2

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer

    # Work around QTBUG-39300.
    CONFIG -= android_install
} else {
    CONFIG += designer
}

macx {
    QMAKE_POST_LINK = install_name_tool -change libqscintilla2.12.dylib $$[QT_INSTALL_LIBS]/libqscintilla2.12.dylib $(TARGET)
}

HEADERS = qscintillaplugin.h
SOURCES = qscintillaplugin.cpp

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
