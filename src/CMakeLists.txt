include_directories(.)
include_directories(/home/dmitry/main/dev/robomongo-shell/src)

# Temporary absolute includes
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/boost-1.56.0)
include_directories(/home/dmitry/main/dev/robomongo-shell/build/debug)
include_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/src)

link_directories(/home/dmitry/main/dev/robomongo-shell/build/debug/mongo)
link_directories(/home/dmitry/main/dev/robomongo/src/third-party/qjson/build/lib)

add_subdirectory(robomongo)
