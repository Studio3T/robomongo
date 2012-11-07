
! include( ../common.pri ) {
    error( Error: couldnt find the common.pri file. )
}

# This specifies the name of the target file.
# It will produce an executable named robomongo on unix and robomongo.exe on windows.
TARGET   = robomongo

TEMPLATE = app

SOURCES += main.cpp
        
INCLUDEPATH += ../gui

LIBS += -lgui


















