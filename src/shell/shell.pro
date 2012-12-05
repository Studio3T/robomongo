
! include( ../common.pri ) {
    error( Error: couldnt find the common.pri file. )
}

# This specifies the name of the target file.
# It will produce an executable named robomongo on unix and robomongo.exe on windows.
TARGET   = shell

TEMPLATE = lib
CONFIG  += staticlib

SOURCES += \
    scripting/utils.cpp \
    scripting/sm_db.cpp \
    scripting/engine_spidermonkey.cpp \
    scripting/engine.cpp \
    util/text.cpp \
    mongo.cpp \
    util/version.cpp \
    util/processinfo.cpp \
    util/processinfo_linux2.cpp \
    shell/shell_utils.cpp \
    shell/shell_utils_launcher.cpp \
    shell/shell_utils_extended.cpp

HEADERS +=

PRECOMPILED_HEADER = mongo/pch.h










