##### Probably don't want to edit below this line #####

# Use it
INCLUDE( ${QT_USE_FILE} )

INCLUDE(AddFileDependencies)

# Include the library include directories, and the current build directory (moc)
INCLUDE_DIRECTORIES(
  ../../include
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_EXECUTABLE(
  cmdline_tester
  cmdline_tester.cpp
  cmdlineparser.cpp
)

TARGET_LINK_LIBRARIES(
  cmdline_tester
  ${QT_LIBRARIES}
  qjson
)
