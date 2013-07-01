CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#################### Project Settings ####################
SET(PROJECT_NAME "robomongo" CACHE STRING "Project Name")
SET(${PROJECT_NAME}_PROJECT_DOMAIN "www.robomongo.com" CACHE STRING "Internet domain name for ${ProjectName}")
SET(PROJECT_COMPANYNAME "paralect")
##########################################################
STRING(TOLOWER ${PROJECT_NAME} PROJECT_NAME_lowercase)
SET (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake")
########################## Options #########################
OPTION(GENERATE_TRANSLATIONS "Generate ts-files for ${PROJECT_NAME}" OFF)
OPTION(MANIFEST_ENABLE "Enable Manifest" OFF)
OPTION(ENABLE_SVN "Enable Svn build incrementing" OFF)
#############################################################
# Configure project
PROJECT(${PROJECT_NAME_lowercase})
OPTION(BOOST_ENABLED "Enable boost system support" ON)
OPTION(BUILD_PCRECPP_LIBRARY "Build pcrecpp library from sources" ON)
OPTION(QT_ENABLED "Enable Qt support" ON)
OPTION(UNICODE_ENABLE "Support unicode" OFF)
option(DEVELOPER_DYNAMIC_RUNTIME "Build with dynamic C++ runtime" ON)
INCLUDE(cmake/config.cmake)

DEFINE_DEFAULT_DEFINITIONS()
SETUP_COMPILER_SETTINGS(${DEVELOPER_DYNAMIC_RUNTIME})

# Search libraries
# use Qt
DETECT_QT()
##################### Libraries search ###################
if(DEVELOPER_QT5)
SET(QT_COMPONENTS_TO_USE 
	Qt5Core 
	Qt5Gui 
	Qt5Widgets
)
ELSE()
SET(QT_COMPONENTS_TO_USE 
	QtCore 
	QtGui 
)
ENDIF()
INTEGRATE_QT(${QT_COMPONENTS_TO_USE})
##########################################################

## Qt static or dinamic detector ##

# enable GIF and ICO support
IF(WINDOWS)
        FIND_LIBRARY(QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
        FIND_LIBRARY(QT_ICO_PLUGIN_DEBUG qicod PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
ELSEIF(MAC_OS)
        FIND_LIBRARY(QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
        FIND_LIBRARY(QT_ICO_PLUGIN_DEBUG qico_debug PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
ENDIF()

#IF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fpermissive")
#IF(CMAKE_COMPILER_IS_GNUCC)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -fPIC")
#ENDIF()
#ENDIF(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCXX)
IF(NOT MINGW)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF()
ELSEIF(MSVC)
ENDIF()

IF(WIN32 AND MSVC)
    SET(CMAKE_EXE_LINKER_FLAGS "/VERSION:\"${${PROJECT_NAME}_VERSION}\"" CACHE STRING "Resetting the default linker options." FORCE)
    STRING(REPLACE "." "," FILEVERSION ${${PROJECT_NAME}_VERSION})
    SET(VERSION_STR ${${PROJECT_NAME}_VERSION})
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
