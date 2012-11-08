
! include( ../common.pri ) {
    error( Error: couldnt find the common.pri file. )
}

# This specifies the name of the target file.
# It will produce an executable named robomongo on unix and robomongo.exe on windows.
TARGET   = robomongo

TEMPLATE = app

SOURCES += main.cpp
        
INCLUDEPATH += \
            $$ROOT/src/gui \
            $$ROOT/src/core

LIBS += -lgui -lcore



win32 {
    contains(QMAKE_HOST.arch, x86_64) {

    } else {
        # Copy qjson.dll to to app/out folder
        QMAKE_POST_LINK += $$quote(xcopy \"$$THIRDPARTY_LIBS_PATH\\qjson\\qjson.dll\" \"$$OUT_PWD\\out\" $$escape_expand(\\n))
    }
}














