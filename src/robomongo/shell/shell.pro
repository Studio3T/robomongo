
! include( ../common.pri ) {
    error( Error: couldnt find the common.pri file. )
}

# This specifies the name of the target file.
# It will produce an executable named robomongo on unix and robomongo.exe on windows.
TARGET   = shell

TEMPLATE = lib
CONFIG  += staticlib
INCLUDEPATH += $$SRC_ROOT

SOURCES += \
    scripting/utils.cpp \
    scripting/sm_db.cpp \
    scripting/engine_spidermonkey.cpp \
    scripting/engine.cpp \
    util/text.cpp \
    mongo.cpp \
    util/version.cpp \
    util/processinfo.cpp \
    shell/shell_utils.cpp \
    shell/shell_utils_launcher.cpp \
    shell/shell_utils_extended.cpp \
    util/ramlog.cpp \
    mongo-server.cpp \
    util/base64.cpp \
    util/file.cpp \
    scripting/bench.cpp \
    scripting/bson_template_evaluator.cpp \
    db/json.cpp \
    db/ptimeutil.cpp

unix:SOURCES += util/processinfo_linux2.cpp
win32:SOURCES += util/processinfo_win32.cpp

HEADERS += \
    db/json.h \
    db/ptimeutil.h

#PRECOMPILED_HEADER = mongo/pch.h










