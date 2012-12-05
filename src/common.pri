greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG  += qt
CONFIG  -= debug_and_release
QT      += core gui script
DEFINES += ROBOMONGO

# Spider Monkey defines:
win32:DEFINES += XP_WIN
unix:DEFINES  += XP_UNIX

DESTDIR      = $$OUT_PWD/out
OBJECTS_DIR  = $$OUT_PWD/obj
RCC_DIR      = $$OUT_PWD/rcc
UI_DIR       = $$OUT_PWD/ui
ROOT         = $$PWD/..
SRC_ROOT     = $$PWD              # root of src/ folder
OUTPUT_ROOT  = $$OUT_PWD/..       # root of output folder (usually target/debug or target/release)

message(PWD:          $$PWD)
message(ROOT:         $$ROOT)
message(DESTDIR:      $$DESTDIR)
message(OUT_PWD:      $$OUT_PWD)
message(OBJECTS_DIR:  $$OBJECTS_DIR)
message(RCC_DIR:      $$RCC_DIR)
message(UI_DIR:       $$UI_DIR)
message(SRC_ROOT:     $$SRC_ROOT)
message(OUTPUT_ROOT:  $$OUTPUT_ROOT)

# Populate $$OS_CPU variable
contains(QMAKE_HOST.arch, x86_64) {
    win32:OS_CPU=win-amd64
    unix:OS_CPU=unix-amd64
} else {
    win32:OS_CPU=win-i386
    unix:OS_CPU=unix-i386
}

THIRDPARTY_LIBS_PATH=$$ROOT/libs/$$OS_CPU

message(THIRDPARTY_LIBS_PATH: $$THIRDPARTY_LIBS_PATH)

# libs paths:
LIBS += -L$$OUTPUT_ROOT/gui/out
LIBS += -L$$OUTPUT_ROOT/core/out
LIBS += -L$$OUTPUT_ROOT/shell/out

# include paths:
INCLUDEPATH += $$ROOT/include
INCLUDEPATH += $$ROOT/include/boost
INCLUDEPATH += $$ROOT/include/qscintilla

# third party libs
LIBS += -L$$THIRDPARTY_LIBS_PATH/qjson
LIBS += -L$$THIRDPARTY_LIBS_PATH/mongoclient
LIBS += -L$$THIRDPARTY_LIBS_PATH/boost
LIBS += -L$$THIRDPARTY_LIBS_PATH/qscintilla
LIBS += -L$$THIRDPARTY_LIBS_PATH/js
LIBS += -lqjson -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lqscintilla2 -ljs


