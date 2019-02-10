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
add_executable(robo_unit_tests MACOSX_BUNDLE WIN32 ${SOURCES_TEST})
add_dependencies(robo_unit_tests robomongo)

if(SYSTEM_WINDOWS)
    set(OBJ_DIR 
        ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/src/robomongo/robomongo.dir/${CMAKE_BUILD_TYPE}/
    )
    target_link_libraries(robo_unit_tests 
        gtest gtest_main mongodb
        # todo: Find better solution instead of this manual adding files
        ${OBJ_DIR}/string_operations.obj
        ${OBJ_DIR}/HexUtils.obj
    )
elseif(SYSTEM_MACOSX)
    find_library(SECURITY NAMES Security)
    find_library(CORE_FOUNDATION NAMES CoreFoundation)
    set(SSL_LIBRARIES ${SECURITY} ${CORE_FOUNDATION})
    set(OBJ_DIR 
        ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/src/robomongo/CMakeFiles/robomongo.dir/
    )
    target_link_libraries(robo_unit_tests 
        gtest gtest_main mongodb ${SSL_LIBRARIES} -lresolv
        ${OBJ_DIR}/utils/string_operations.cpp.o
        ${OBJ_DIR}/core/HexUtils.cpp.o
    )
elseif(SYSTEM_LINUX)
    # todo
endif()

target_include_directories(robo_unit_tests
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)