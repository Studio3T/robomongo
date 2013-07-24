CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
#CMAKE_POLICY(SET CMP0020 NEW)
#################### Project Settings ####################
SET(PROJECT_NAME "Robomongo" CACHE STRING "Project Name")
SET(${PROJECT_DOMAIN} "www.robomongo.org" CACHE STRING "Internet domain name for ${PROJECT_NAME}")
SET(PROJECT_COMPANYNAME "Paralect")
SET(PROJECT_COMPANYNAME_DOMAIN "www.paralect.com")
SET(PROJECT_GITHUB_FORK "www.github.com/paralect/robomongo")
SET(PROJECT_GITHUB_ISSUES "www.github.com/paralect/robomongo/issues")
##########################################################
STRING(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
SET (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake")
########################## Options #########################
OPTION(GENERATE_TRANSLATIONS "Generate ts-files for ${PROJECT_NAME}" OFF)
IF(WIN32)
	OPTION(MANIFEST_ENABLE "Enable Manifest" OFF)
ENDIF(WIN32)
#############################################################
# Configure project
PROJECT(${PROJECT_NAME})
SET(MAJOR "0")
SET(MINOR "7")
SET(PATCH "2")
SET(BUILD " beta 1")
SET(${PROJECT_NAME}_VERSION "${MAJOR}.${MINOR}.${PATCH}${BUILD}" CACHE STRING "${PROJECT_NAME} version")
OPTION(BOOST_ENABLED "Enable boost system support" OFF)
OPTION(BUILD_PCRECPP_LIBRARY "Build pcrecpp library from sources" ON)
OPTION(QT_ENABLED "Enable Qt support" ON)
OPTION(CPACK_SUPPORT "Enable package support" ON)
OPTION(UNICODE_ENABLE "Support unicode" OFF)
OPTION(DEVELOPER_DYNAMIC_RUNTIME "Build with dynamic C++ runtime" ON)
OPTION(DEVELOPER_FEATURES "use some debug tricks for developers" OFF)
IF(MSVC)
	IF(CMAKE_CL_64)
		OPTION(BUILD_64X "Build 64bit project" ON)
	ELSE()
		OPTION(BUILD_64X "Build 64bit project" OFF)
	ENDIF()
ELSE()
	IF("${OS_ARC}" STREQUAL "64")
		OPTION(BUILD_64X "Build 64bit project" ON)
	ELSEIF("${OS_ARC}" STREQUAL "32")
		OPTION(BUILD_64X "Build 64bit project" OFF)
	ELSE()
		IF(CMAKE_SIZEOF_VOID_P MATCHES 8) 
			OPTION(BUILD_64X "Build 64bit project" ON)
		ELSE(CMAKE_SIZEOF_VOID_P MATCHES 8) 
			OPTION(BUILD_64X "Build 64bit project" OFF)
		ENDIF(CMAKE_SIZEOF_VOID_P MATCHES 8)
	ENDIF()
ENDIF()
INCLUDE(cmake/config.cmake)

DEFINE_DEFAULT_DEFINITIONS()

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
	IF (NOT CMAKE_BUILD_TYPE)
		SET(CMAKE_BUILD_TYPE DEBUG)
	ENDIF()
	IF(NOT MINGW)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	ENDIF()
ENDIF()
IF(BUILD_64X)
	IF(CMAKE_COMPILER_IS_GNUCXX)
		SET(CFLAGS "${CFLAGS} -m64")
		SET(CXXFLAGS "${CXXFLAGS} -m64")
	ELSEIF(MSVC)
		SET(CMAKE_EXE_LINKER_FLAGS "/machine:x64")
	ENDIF()
	IF(APPLE)
                SET(CMAKE_OSX_ARCHITECTURES x86_64)
	ENDIF()
	SET(ARCH_TEXT 64)
ELSE()
	IF(CMAKE_COMPILER_IS_GNUCXX)
		SET(CFLAGS "${CFLAGS} -m32")
		SET(CXXFLAGS "${CXXFLAGS} -m32")
	ELSEIF(MSVC)
		SET(CMAKE_EXE_LINKER_FLAGS "/machine:x86")
	ENDIF()
	IF(APPLE)
        SET(CMAKE_OSX_ARCHITECTURES i386)
	#-DCMAKE_OSX_ARCHITECTURES=x86_64;i386
	ENDIF()
	SET(ARCH_TEXT 32)
ENDIF()

IF(NOT CMAKE_INSTALL_PREFIX})
	SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
ENDIF()
############################################################################

IF(MANIFEST_ENABLE)
	IF(WIN32)
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME_LOWERCASE} POST_BUILD COMMAND mt -manifest "${CMAKE_CURRENT_SOURCE_DIR}/resources/win7.manifest" -outputresource:"$(TargetDir)$(TargetFileName)"\;\#1)
	ENDIF(WIN32)
ENDIF(MANIFEST_ENABLE)

IF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	MESSAGE(SEND_ERROR "In-source builds are not allowed.")
ENDIF("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

############################################################################
if(WIN32)
	set(RESOURCE_OS    "${CMAKE_SOURCE_DIR}/src/robomongo/gui/resources/win.rc")
	set(ICON_FILE      "${CMAKE_SOURCE_DIR}/src/robomongo/gui/resources/robo.ico")
	#set(RESOURCE_OS_IN "${CMAKE_SOURCE_DIR}/src/robomongo/gui/resources/winres.rc.in")
elseif(APPLE)
	set(MACOSX_BUNDLE_ICON_FILE 
		${PROJECT_NAME}.icns
	)
	set(MACOSX_APPICON 
		${CMAKE_CURRENT_SOURCE_DIR}/resources/images/${MACOSX_BUNDLE_ICON_FILE}
	)
	
	#Stub. rewrite on plist required
	set(RESOURCE_OS
			${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.desktop
	)
	set(RESOURCE_OS_IN
			${CMAKE_CURRENT_SOURCE_DIR}/resources/${PROJECT_NAME}.desktop.in
	)
	set(ICON_FILE
		${MACOSX_APPICON}
	)
elseif(UNIX)
	set(RESOURCE_OS
			${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.desktop
	)
	set(RESOURCE_OS_IN
			${CMAKE_CURRENT_SOURCE_DIR}/resources/${PROJECT_NAME}.desktop.in
	)
endif()
# project sources
ADD_SUBDIRECTORY(src)

IF(CPACK_SUPPORT)
    INCLUDE(InstallRequiredSystemLibraries)
    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_NAME}" project")
    SET(CPACK_PACKAGE_VENDOR ${PROJECT_COMPANYNAME})
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    SET(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR})
    SET(CPACK_PACKAGE_VERSION_MINOR ${MINOR})
    STRING(REGEX REPLACE " " "_" BUILD_ESCAPE ${BUILD})
    SET(CPACK_PACKAGE_VERSION_PATCH ${PATCH}${BUILD_ESCAPE})
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
    IF(WIN32)
        # There is a bug in NSI that does not handle full unix paths properly. Make
        # sure there is at least one set of four (4) backlasshes.
        SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/install/windows\\\\robomongo.ico")
        SET(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\${PROJECT_NAME}.exe")
		SET(CPACK_NSIS_CREATE_ICONS "CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PROJECT_NAME}.lnk\\\" \\\"$INSTDIR\\\\${PROJECT_NAME}.exe\\\"")

        SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
        SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
        SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
        SET(SIDEBAR_IMAGE ${CMAKE_SOURCE_DIR}/install/windows\\\\database.bmp)
        SET(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "!define MUI_WELCOMEFINISHPAGE_BITMAP \\\"${SIDEBAR_IMAGE}\\\"")
        SET(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
        SET(CPACK_NSIS_MODIFY_PATH ON)
    ELSE()
        SET(CPACK_STRIP_FILES "bin/${PROJECT_NAME}")
        SET(CPACK_SOURCE_STRIP_FILES "")
    ENDIF(WIN32)
    SET(CPACK_SOURCE_IGNORE_FILES "/include/;/lib/;/bin/;\\\\.cxx;\\\\.cpp;\\\\.h")
    SET(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} ${PROJECT_NAME})
    INCLUDE(CPack)
ENDIF(CPACK_SUPPORT)
