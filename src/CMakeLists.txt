include_directories(.)
include_directories(/home/dmitry/main/dev/robomongo-shell/src)

# Temporary absolute includes
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/boost-1.56.0) # Boost
include_directories(/home/dmitry/main/dev/robomongo-shell/build/debug)
include_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/src) # QJson
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/mozjs-38/include)                       # MozJS
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/mozjs-38/platform/x86_64/linux/include) # MozJS for Linux x86_64
link_directories(/home/dmitry/main/dev/robomongo-shell/build/debug/mongo)
link_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/build/lib)

add_subdirectory(robomongo)
