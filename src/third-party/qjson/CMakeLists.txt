project(QJson)

add_library(qjson
    sources/src/parser.cpp
    sources/src/qobjecthelper.cpp
    sources/src/json_scanner.cpp
    sources/src/json_parser.cc
    sources/src/parserrunnable.cpp
    sources/src/serializer.cpp
    sources/src/serializerrunnable.cpp
)

target_link_libraries(qjson Qt5::Core)
