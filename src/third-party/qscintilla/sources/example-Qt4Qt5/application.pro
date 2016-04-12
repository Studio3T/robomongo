CONFIG      += release qscintilla2

macx {
    QMAKE_POST_LINK = install_name_tool -change libqscintilla2.11.dylib $$[QT_INSTALL_LIBS]/libqscintilla2.11.dylib $(TARGET)
}

HEADERS      = mainwindow.h
SOURCES      = main.cpp mainwindow.cpp
RESOURCES    = application.qrc
