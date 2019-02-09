#
# Unit Testing using Google Test (currently disabled)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(SOURCES_TEST
    # todo: new var ROBO_SRC_DIR
    ${CMAKE_HOME_DIRECTORY}/src/robomongo/utils/string_operations_test.cpp
    ${CMAKE_HOME_DIRECTORY}/src/robomongo/core/HexUtils_test.cpp
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_executable(robo_unit_tests ${SOURCES_TEST})
add_dependencies(robo_unit_tests robomongo)

set(COMPILE_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/src/robomongo/robomongo.dir/${CMAKE_BUILD_TYPE}/)
target_link_libraries(robo_unit_tests 
    gtest gtest_main mongodb
    # todo: Add case for Linux and Mac or better find better solution
    ${COMPILE_OUTPUT_DIR}/string_operations.obj
    ${COMPILE_OUTPUT_DIR}/HexUtils.obj
)
target_include_directories(robo_unit_tests
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)