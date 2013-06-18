CONFIG       += release

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

macx {
    QMAKE_POST_LINK = install_name_tool -change libqscintilla2.9.dylib $$[QT_INSTALL_LIBS]/libqscintilla2.9.dylib $(TARGET)
}

HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
RESOURCES     = application.qrc
LIBS         += -L$$[QT_INSTALL_LIBS] -lqscintilla2
