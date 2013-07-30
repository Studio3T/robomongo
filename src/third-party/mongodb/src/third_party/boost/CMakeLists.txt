cmake_minimum_required(VERSION 2.8)
project(boost_mongo_third_party)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR})
ADD_DEFINITIONS(-DBOOST_THREAD_BUILD_LIB)
 IF(WIN32)		
        SET(BOOST_SOURCES_PLATFORM
        libs/thread/src/win32/thread.cpp
        libs/thread/src/win32/tss_dll.cpp
        libs/thread/src/win32/tss_pe.cpp)
    ELSE()
        SET(BOOST_SOURCES_PLATFORM
        libs/thread/src/pthread/once.cpp
        libs/thread/src/pthread/thread.cpp)
    ENDIF()
set(HEADERS

)
set(SOURCES
 ${BOOST_SOURCES_PLATFORM}
	libs/program_options/src/cmdline.cpp
	libs/program_options/src/config_file.cpp
	libs/program_options/src/convert.cpp
	libs/program_options/src/options_description.cpp
	libs/program_options/src/parsers.cpp
	libs/program_options/src/positional_options.cpp
	libs/program_options/src/split.cpp
	libs/program_options/src/utf8_codecvt_facet.cpp
	libs/program_options/src/value_semantic.cpp
	libs/program_options/src/variables_map.cpp
	libs/program_options/src/winmain.cpp
	
	libs/filesystem/v2/src/v2_operations.cpp
	libs/filesystem/v2/src/v2_path.cpp
	libs/filesystem/v2/src/v2_portability.cpp
	libs/filesystem/v3/src/codecvt_error_category.cpp
	libs/filesystem/v3/src/operations.cpp
	libs/filesystem/v3/src/path.cpp
	libs/filesystem/v3/src/path_traits.cpp
	libs/filesystem/v3/src/portability.cpp
	libs/filesystem/v3/src/unique_path.cpp
	libs/filesystem/v3/src/utf8_codecvt_facet.cpp
	libs/filesystem/v3/src/windows_file_codecvt.cpp

    libs/system/src/error_code.cpp
)
	
source_group("Boost mongo third party files" FILES ${HEADERS} ${SOURCES})
add_library(boost_mongo_third_party STATIC ${HEADERS} ${SOURCES})
