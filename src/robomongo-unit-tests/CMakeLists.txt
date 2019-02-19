###
### Unit Testing with Google Test 
###

## !! Important Note: Currently unit testing is disabled for Linux due to MongoDB linking problem
if (SYSTEM_LINUX)        
    message("\n *Note: Currently unit testing is disabled for Linux due to MongoDB linking problem")
    return()
endif()

enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(ROBO_SRC_DIR ${CMAKE_HOME_DIRECTORY}/src/robomongo)

## New test & source file pairs MUST have the following format: 
#  Test file:   /path/Foo_test.cpp
#  Source file: /path/Foo.cpp
set(TEST_FILES 
    /utils/StringOperations_test.cpp
    /core/HexUtils_test.cpp
)

set(SOURCES_TEST "")
foreach(file ${TEST_FILES}) 
    set (SOURCES_TEST "${SOURCES_TEST}${ROBO_SRC_DIR}${file};")
endforeach()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_executable(robo_unit_tests ${SOURCES_TEST})
add_dependencies(robo_unit_tests robomongo)

if(SYSTEM_WINDOWS)
    set(OBJ_DIR 
        ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/src/robomongo/robomongo.dir/${CMAKE_BUILD_TYPE}/
    )
    set(OBJ_FILES_WITH_PATHS "")
    foreach(FILE ${TEST_FILES}) 
        get_filename_component(FILENAME ${FILE} NAME)
        string(REPLACE "_test.cpp" ".obj" OBJ_FILENAME ${FILENAME})
        set(OBJ_FILES_WITH_PATHS "${OBJ_FILES_WITH_PATHS}${OBJ_DIR}/${OBJ_FILENAME};")        
    endforeach()

    target_link_libraries(robo_unit_tests 
        gtest gtest_main mongodb
        # todo: Find better solution for this manually created list of obj files        
        ${OBJ_FILES_WITH_PATHS}
    )
elseif(SYSTEM_MACOSX)
    find_library(SECURITY NAMES Security)
    find_library(CORE_FOUNDATION NAMES CoreFoundation)
    set(SSL_LIBRARIES ${SECURITY} ${CORE_FOUNDATION})
    set(OBJ_DIR 
        ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/src/robomongo/CMakeFiles/robomongo.dir/
    )

    set(OBJ_FILES_WITH_PATHS "")
    foreach(FILE ${TEST_FILES}) 
        string(REPLACE "_test.cpp" ".cpp.o" OBJ_FILENAME ${FILE})
        set(OBJ_FILES_WITH_PATHS "${OBJ_FILES_WITH_PATHS}${OBJ_DIR}${OBJ_FILENAME};")        
    endforeach()
    
    target_link_libraries(robo_unit_tests 
        gtest gtest_main mongodb ${SSL_LIBRARIES} -lresolv
        ${OBJ_FILES_WITH_PATHS}
    )    
# elseif(SYSTEM_LINUX)  
#     set(CMAKE_BUILD_TYPE_L ${CMAKE_BUILD_TYPE})
#     string(TOLOWER ${CMAKE_BUILD_TYPE_L} CMAKE_BUILD_TYPE_L)
#     set(OBJ_DIR 
#         ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE_L}/src/robomongo/CMakeFiles/robomongo.dir
#     )
#     # todo: Getting openssl link errors when mongodb lib is linked
#     target_link_libraries(robo_unit_tests
#         gtest gtest_main #ssl crypto #mongodb 
#         ${OBJ_DIR}/utils/StringOperations.cpp.o
#         # ${OBJ_DIR}/core/HexUtils.cpp.o
#     )
endif()

target_include_directories(robo_unit_tests
    PRIVATE
        ${CMAKE_HOME_DIRECTORY}/src)