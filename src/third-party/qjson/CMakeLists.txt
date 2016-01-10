project(QJson)

add_library(qjson
    src/parser.cpp
    src/qobjecthelper.cpp
    src/json_scanner.cpp
    src/json_parser.cc
    src/parserrunnable.cpp
    src/serializer.cpp
    src/serializerrunnable.cpp
)

target_link_libraries(qjson Qt5::Core)
