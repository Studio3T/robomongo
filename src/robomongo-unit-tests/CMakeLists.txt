#
# Unit Testing using Google Test (currently disabled)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(ROBO_SRC_DIR ${CMAKE_HOME_DIRECTORY}/src/robomongo)

if(SYSTEM_LINUX)      
    set(SOURCES_TEST      
        ${ROBO_SRC_DIR}/utils/string_operations_test.cpp
        # todo
        # ${CMAKE_HOME_DIRECTORY}/src/robomongo/core/HexUtils_test.cpp 
    )
elseif()
    set(SOURCES_TEST
        ${ROBO_SRC_DIR}/utils/string_operations_test.cpp
        ${ROBO_SRC_DIR}/core/HexUtils_test.cpp
    )
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_executable(robo_unit_tests ${SOURCES_TEST})
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
    set(CMAKE_BUILD_TYPE_L ${CMAKE_BUILD_TYPE})
    string(TOLOWER ${CMAKE_BUILD_TYPE_L} CMAKE_BUILD_TYPE_L)
    set(OBJ_DIR 
        ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE_L}/src/robomongo/CMakeFiles/robomongo.dir
    )
    # todo: Getting openssl link errors when mongodb lib is linked
    target_link_libraries(robo_unit_tests
        gtest gtest_main #ssl crypto #mongodb 
        ${OBJ_DIR}/utils/string_operations.cpp.o
        # ${OBJ_DIR}/core/HexUtils.cpp.o
    )
endif()

target_include_directories(robo_unit_tests
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)