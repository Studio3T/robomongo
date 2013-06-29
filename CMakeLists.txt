CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#################### Project Settings ####################
SET(PROJECT_NAME "robomongo" CACHE STRING "Project Name")
SET(${PROJECT_NAME}_PROJECT_DOMAIN "www.robomongo.com" CACHE STRING "Internet domain name for ${ProjectName}")
SET(${PROJECT_NAME}_COPYRIGHT "robomongo Inc.")
##########################################################
STRING(TOLOWER ${PROJECT_NAME} PROJECT_NAME_lowercase)
SET (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake")
########################## Options #########################
OPTION(GENERATE_TRANSLATIONS "Generate ts-files for ${ProjectName}" OFF)
OPTION(MANIFEST_ENABLE "Enable Manifest" OFF)
OPTION(ENABLE_SVN "Enable Svn build incrementing" OFF)
#############################################################
# Configure project
PROJECT(${PROJECT_NAME_lowercase})
OPTION(BOOST_ENABLED "Enable boost system support" ON)
OPTION(BUILD_PCRECPP_LIBRARY "Build pcrecpp library from sources" ON)
OPTION(QT_ENABLED "Enable Qt support" ON)
OPTION(UNICODE_ENABLE "Support unicode" OFF)
INCLUDE(cmake/config.cmake)

IF(WINDOWS)
	set(CMAKE_INSTALL_PREFIX "$ENV{PROGRAMFILES}\\${ProjectName_lowercase}")
ENDIF()

SET(MAJOR "1")
SET(MINOR "0")
SET(PATCH "0")
SET(BUILD "0")

IF(ENABLE_SVN)
FIND_PACKAGE(Subversion)

IF(Subversion_FOUND AND WIN32)
    Subversion_WC_INFO(${PROJECT_SOURCE_DIR}/source Project)
	SET(BUILD ${Project_WC_REVISION})
    MESSAGE("Current svn revision is ${Project_WC_REVISION}")
ENDIF(Subversion_FOUND AND WIN32)

ENDIF(ENABLE_SVN)
SET(${ProjectName}_VERSION "${MAJOR}.${MINOR}.${PATCH}.${BUILD}" CACHE STRING "${ProjectName} version")
##################### Compiler options and definitions ####################
ADD_DEFINITIONS(
        -DPROJECT_VERSION=${${PROJECT_NAME}_VERSION}
        -DPROJECT_NAME="${PROJECT_NAME}"
        -DPROJECT_NAME_LOWERCASE="${PROJECT_NAME_lowercase}"
        -DPROJECT_DOMAIN="${${PROJECT_NAME}_PROJECT_DOMAIN}"
		-DPROJECT_COPYRIGHT="${${PROJECT_NAME}_COPYRIGHT}"
)
##################### Libraries search ###################
SET(QT_COMPONENTS_TO_USE 
	Qt5Core 
	Qt5Gui 
	Qt5Widgets
)

# Search libraries
# use Qt
DETECT_QT()
INTEGRATE_QT(${QT_COMPONENTS_TO_USE})
##########################################################

## Qt static or dinamic detector ##

# enable GIF and ICO support
IF(WINDOWS)
        FIND_LIBRARY(QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
        FIND_LIBRARY(QT_ICO_PLUGIN_DEBUG qicod PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
ELSEIF(MAC_OS)
        SET(SUFFIX ".a")
        SET(PREFFIX "lib")
        FIND_LIBRARY(QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
        FIND_LIBRARY(QT_ICO_PLUGIN_DEBUG qico_debug PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
ENDIF()

#IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fpermissive")
#IF(CMAKE_COMPILER_IS_GNUCC)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -fPIC")
#ENDIF()
#ENDIF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
IF(APPLE)
	SET(CMAKE_OSX_ARCHITECTURES i386)
	SET(CMAKE_OSX_ARCHITECTURES_DEBUG i386)
	SET(CMAKE_CXX_FLAGS Ò-arch i386")
	SET(LINK_FLAGS Ò-arch i386")

	set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -stdlib=libc++ -g -Wall")

ENDIF()

IF(WIN32 AND MSVC)
    SET(CMAKE_EXE_LINKER_FLAGS "/VERSION:\"${${PROJECT_NAME}_VERSION}\"" CACHE STRING "Resetting the default linker options." FORCE)
    STRING(REPLACE "." "," FILEVERSION ${${ProjectName}_VERSION})
    SET(VERSION_STR ${${ProjectName}_VERSION})
ENDIF()

############################################################################

IF(MANIFEST_ENABLE)
	IF(WIN32)
		add_custom_command(TARGET ${ProjectName_lowercase} POST_BUILD COMMAND mt -manifest "${CMAKE_CURRENT_SOURCE_DIR}/resources/win7.manifest" -outputresource:"$(TargetDir)$(TargetFileName)"\;\#1)
	ENDIF(WIN32)

#	SET_TARGET_PROPERTIES(${ProjectName_lowercase} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT /DEBUG")
ENDIF(MANIFEST_ENABLE)

IF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
MESSAGE(SEND_ERROR "In-source builds are not allowed.")
ENDIF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
# project sources
ADD_SUBDIRECTORY(src)
