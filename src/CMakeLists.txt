include_directories(.)
include_directories(/home/dmitry/main/dev/robomongo-shell/src)

# Temporary absolute includes
include_directories(/home/dmitry/main/dev/robomongo-shell/src/third_party/boost-1.56.0)
include_directories(/home/dmitry/main/dev/robomongo-shell/build/debug)
link_directories(/home/dmitry/main/dev/robomongo-shell/build/debug/mongo)

add_subdirectory(robomongo)
