include_directories(.)

# Temporary absolute includes
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/boost-1.56.0) # Boost
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/mozjs-38/include)                       # MozJS
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/mozjs-38/platform/x86_64/linux/include) # MozJS for Linux x86_64

link_directories(/home/dmitry/main/dev/robomongo-shell)

# PCRECPP
link_directories(/home/dmitry/main/dev/robomongo-shell/build/debug/third_party/pcre-8.37)
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/pcre-8.37)

# Mongo
include_directories(/home/dmitry/main/dev/robomongo-shell/src)
include_directories(/home/dmitry/main/dev/robomongo-shell/build/debug)
link_directories(/home/dmitry/main/dev/robomongo-shell/build/debug/mongo)

# QJson
include_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/src) # QJson
link_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/build/lib)

# QScintilla
include_directories(/home/dmitry/main/dev/robomongo/src/third-party/qscintilla/Qt4Qt5) # QScintilla
link_directories(/home/dmitry/main/dev/robomongo/src/third-party/qscintilla/Qt4Qt5)

add_subdirectory(robomongo)
