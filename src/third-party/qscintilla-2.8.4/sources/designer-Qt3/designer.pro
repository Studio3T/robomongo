# This is the qmake file for the QScintilla plugin for Qt Designer.


TEMPLATE = lib
TARGET = qscintillaplugin
DESTDIR = $(QTDIR)/plugins/designer

CONFIG += qt warn_on release plugin

SOURCES += qscintillaplugin.cpp

LIBS += -lqscintilla2
