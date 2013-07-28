MACRO(DETECT_QT)
    FIND_PACKAGE( Qt5Core QUIET )
    IF(Qt5Core_FOUND)
            SET(DEVELOPER_QT5 1)
    ELSE(Qt5Core_FOUND)
            SET(DEVELOPER_QT5 0)
    ENDIF(Qt5Core_FOUND)
ENDMACRO(DETECT_QT)

MACRO(QTX_WRAP_CPP)
    IF(DEVELOPER_QT5)
        QT5_WRAP_CPP(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_WRAP_CPP(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_WRAP_CPP)

MACRO(QTX_GENERATE_MOC)
    IF(DEVELOPER_QT5)
        QT5_GENERATE_MOC(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_GENERATE_MOC(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_GENERATE_MOC)

MACRO(QTX_ADD_TRANSLATION)
    IF(DEVELOPER_QT5)
        QT5_ADD_TRANSLATION(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_ADD_TRANSLATION(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_ADD_TRANSLATION)

MACRO(QTX_CREATE_TRANSLATION)
    IF(DEVELOPER_QT5)
        QT5_CREATE_TRANSLATION(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_CREATE_TRANSLATION(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_CREATE_TRANSLATION)

MACRO(QTX_WRAP_UI)
    IF(DEVELOPER_QT5)
        QT5_WRAP_UI(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_WRAP_UI(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_WRAP_UI)

MACRO(QTX_ADD_RESOURCES)
    IF(DEVELOPER_QT5)
        QT5_ADD_RESOURCES(${ARGN})
    ELSE(DEVELOPER_QT5)
        QT4_ADD_RESOURCES(${ARGN})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(QTX_ADD_RESOURCES)

MACRO(INTEGRATE_QT)

IF(DEVELOPER_QT5)
    SET(USE_QT_DYNAMIC ON)
    SET(QT_COMPONENTS_TO_USE ${ARGV})

    IF(DEVELOPER_BUILD_TESTS)
        SET(QT_COMPONENTS_TO_USE ${QT_COMPONENTS_TO_USE} Qt5Test)
    ENDIF(DEVELOPER_BUILD_TESTS)

    FOREACH(qtComponent ${QT_COMPONENTS_TO_USE})
        IF(NOT ${qtComponent} STREQUAL "Qt5ScriptTools")
            FIND_PACKAGE(${qtComponent} REQUIRED)
        ELSE()
            FIND_PACKAGE(${qtComponent} QUIET)
        ENDIF()
    
        INCLUDE_DIRECTORIES( ${${qtComponent}_INCLUDE_DIRS} )
        #STRING(REGEX REPLACE "Qt5" "" COMPONENT_SHORT_NAME ${qtComponent})		
        #set(QT_MODULES_TO_USE ${QT_MODULES_TO_USE} ${COMPONENT_SHORT_NAME})
        IF(${${qtComponent}_FOUND} AND NOT(${qtComponent} STREQUAL "Qt5LinguistTools"))
        STRING(REGEX REPLACE "Qt5" "" componentShortName ${qtComponent})		
                    SET(QT_LIBRARIES ${QT_LIBRARIES} "Qt5::${componentShortName}")
        ENDIF()
    ENDFOREACH(qtComponent ${QT_COMPONENTS_TO_USE})
    
    IF(NOT Qt5ScriptTools_FOUND)
        ADD_DEFINITIONS(-DQT_NO_SCRIPTTOOLS)
    ENDIF()

    IF(DEVELOPER_OPENGL)
        FIND_PACKAGE(Qt5OpenGL REQUIRED)
        INCLUDE_DIRECTORIES( ${Qt5OpenGL_INCLUDE_DIRS})
    ENDIF(DEVELOPER_OPENGL)
    SETUP_COMPILER_SETTINGS(${USE_QT_DYNAMIC})
ELSE(DEVELOPER_QT5)
    SET(QT_COMPONENTS_TO_USE ${ARGV})
	# repeat this for every debug/optional package
    LIST(FIND QT_COMPONENTS_TO_USE "QtScriptTools" QT_SCRIPT_TOOLS_INDEX)
    IF(NOT ${QT_SCRIPT_TOOLS_INDEX} EQUAL -1)
        LIST(REMOVE_ITEM QT_COMPONENTS_TO_USE "QtScriptTools")
        SET(QT_DEBUG_COMPONENTS_TO_USE ${QT_DEBUG_COMPONENTS_TO_USE} "QtScriptTools")
    ENDIF()
    IF(DEVELOPER_BUILD_TESTS)
        SET(QT_COMPONENTS_TO_USE ${QT_COMPONENTS_TO_USE} QtTest)
    ENDIF(DEVELOPER_BUILD_TESTS)
    FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS_TO_USE} REQUIRED)
    FIND_PACKAGE(Qt4 COMPONENTS ${QT_DEBUG_COMPONENTS_TO_USE} QUIET)
    IF(NOT QT_QTSCRIPTTOOLS_FOUND)
        ADD_DEFINITIONS(-DQT_NO_SCRIPTTOOLS)
    ENDIF()
    INCLUDE(${QT_USE_FILE})
    SET(QT_3RDPARTY_DIR ${QT_BINARY_DIR}/../src/3rdparty)

	#checking is Qt dynamic or statis version will be used
    GET_FILENAME_COMPONENT(QTCORE_DLL_NAME_WE ${QT_QTCORE_LIBRARY_RELEASE} NAME_WE)
    GET_FILENAME_COMPONENT(QT_LIB_PATH ${QT_QTCORE_LIBRARY_RELEASE} PATH)

    IF(WIN32)
        SET(DLL_EXT dll)
    # message("searching ${QTCORE_DLL_NAME_WE}.${DLL_EXT} in " ${QT_LIB_PATH} ${QT_LIB_PATH}/../bin)
        FIND_FILE(QTCORE_DLL_FOUND_PATH ${QTCORE_DLL_NAME_WE}.${DLL_EXT} PATHS ${QT_LIB_PATH} ${QT_LIB_PATH}/../bin
            NO_DEFAULT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_CMAKE_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            )
            MESSAGE("QTCORE_DLL_FOUND_PATH=" ${QTCORE_DLL_FOUND_PATH})
            IF(EXISTS ${QTCORE_DLL_FOUND_PATH})
                SET(USE_QT_DYNAMIC ON)
            ELSE(EXISTS ${QTCORE_DLL_FOUND_PATH})
                SET(USE_QT_DYNAMIC OFF)
            ENDIF(EXISTS ${QTCORE_DLL_FOUND_PATH})
    ELSE()
        SET(USE_QT_DYNAMIC ON)
    ENDIF(WIN32)
    SETUP_COMPILER_SETTINGS(${USE_QT_DYNAMIC})
    # use jscore
    IF(MSVC AND NOT USE_QT_DYNAMIC)
        FIND_LIBRARY(JSCORE_LIB_RELEASE jscore
            PATHS
            ${QT_3RDPARTY_DIR}/webkit/Source/JavaScriptCore/release
            ${QT_3RDPARTY_DIR}/webkit/JavaScriptCore/release
            )
        FIND_LIBRARY(JSCORE_LIB_DEBUG NAMES jscored jscore
            PATHS
            ${QT_3RDPARTY_DIR}/webkit/Source/JavaScriptCore/debug
            ${QT_3RDPARTY_DIR}/webkit/JavaScriptCore/debug
            )
    SET(JSCORE_LIBS optimized ${JSCORE_LIB_RELEASE} debug ${JSCORE_LIB_DEBUG} )
    ENDIF(MSVC AND NOT USE_QT_DYNAMIC)
	# end of use jscore
ENDIF(DEVELOPER_QT5)

ENDMACRO(INTEGRATE_QT)


##############################################################################

MACRO(INSTALL_QT TARGET_NAME LIB_DIST)
IF(NOT DEVELOPER_QT5)
    IF(WIN32 OR APPLE)
        SET(QT_COMPONENTS_TO_USE ${ARGV})
        LIST(REMOVE_ITEM QT_COMPONENTS_TO_USE ${TARGET_NAME})
        FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS_TO_USE} REQUIRED)
        FIND_PACKAGE(Qt4 COMPONENTS ${QT_DEBUG_COMPONENTS_TO_USE} QUIET)
        INCLUDE(${QT_USE_FILE})
        GET_TARGET_PROPERTY(targetLocation ${TARGET_NAME} LOCATION)
        GET_FILENAME_COMPONENT(targetDir ${targetLocation} PATH)
        
        IF(WIN32)
            SET(REPLACE_IN_LIB ".lib" ".dll")
            SET(REPLACE_IN_LIB2 "/lib/([^/]+)$" "/bin/\\1")
        ELSEIF(APPLE)
            SET(REPLACE_IN_LIB ".a" ".dylib")
            SET(REPLACE_IN_LIB2 "" "")
        ENDIF()

        FOREACH(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})
            STRING(TOUPPER "${qtComponent}" QtComCap)
            TARGET_LINK_LIBRARIES(${TARGET_NAME} ${QT_${QtComCap}_LIBRARY})
            IF(NOT ${qtComponent} STREQUAL "QtMain")
                STRING(REPLACE ${REPLACE_IN_LIB} dllNameDeb "${QT_${QtComCap}_LIBRARY_DEBUG}")
                IF(WIN32)
                    STRING(REGEX REPLACE ${REPLACE_IN_LIB2} dllNameDeb ${dllNameDeb})
                ENDIF()
                SET(DLIBS_TO_COPY_DEBUG ${DLIBS_TO_COPY_DEBUG} ${dllNameDeb})
                IF(NOT ${qtComponent} STREQUAL "QtScriptTools")
                    STRING(REPLACE ${REPLACE_IN_LIB} dllNameRel "${QT_${QtComCap}_LIBRARY_RELEASE}")
                    IF(WIN32)
                        STRING(REGEX REPLACE ${REPLACE_IN_LIB2} dllNameRel ${dllNameRel})
                    ENDIF()
                    SET(DLIBS_TO_COPY_RELEASE ${DLIBS_TO_COPY_RELEASE} ${dllNameRel})
                ENDIF()
                # TODO: check this code @ MAC and *ux
                ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
                    ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:
                    ${dllNameDeb}> $<$<NOT:$<CONFIG:Debug>>:
                    ${dllNameRel}>  $<TARGET_FILE_DIR:${TARGET_NAME}>
                    )
            ENDIF(NOT ${qtComponent} STREQUAL "QtMain")
        ENDFOREACH(qtComponent ${QT_COMPONENTS_TO_USE})
    ENDIF(WIN32 OR APPLE)
ELSE()# Qt5
    LIST(FIND QT_COMPONENTS_TO_USE "Qt5Xml" QT_XML_INDEX)
    IF(NOT ${QT_XML_INDEX} EQUAL -1)
        GET_TARGET_PROPERTY(libLocation ${Qt5Xml_LIBRARIES} LOCATION)
        STRING(REGEX REPLACE "Xml" "XmlPatterns" libLocation ${libLocation})
        QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${libLocation} "")
    ENDIF()
    FOREACH(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})
        IF(NOT ${qtComponent} STREQUAL "Qt5LinguistTools")
            IF(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
            GET_TARGET_PROPERTY(libLocation ${${qtComponent}_LIBRARIES} LOCATION)
            QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${libLocation} "")
            ELSE(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
                MESSAGE("Canont find library ${qtComponent}_LIBRARIES")
            ENDIF(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
        ENDIF()
    ENDFOREACH(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})
ENDIF(NOT DEVELOPER_QT5)

IF(MSVC)
	# Visual studio install
    FOREACH(buildconfig ${CMAKE_CONFIGURATION_TYPES})
        MESSAGE(STATUS "VC configuration install for ${buildconfig} ${DLIBS_TO_COPY_RELEASE}")
        IF(${buildconfig} STREQUAL "Debug")
            SET(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_DEBUG})
        ELSE()
            SET(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_RELEASE})
        ENDIF()
        INSTALL(FILES ${DLIBS_TO_COPY} DESTINATION ${LIB_DIST} CONFIGURATIONS ${buildconfig} )
    ENDFOREACH(buildconfig ${CMAKE_CONFIGURATION_TYPES})
ELSE()
	# Make install
    MESSAGE(STATUS "Make configuration install ${DLIBS_TO_COPY_RELEASE}")
    IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        SET(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_DEBUG})
    ELSE()
        SET(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_RELEASE})
    ENDIF()
    INSTALL(FILES ${DLIBS_TO_COPY} DESTINATION ${LIB_DIST})	
    #create symlinks
	FOREACH(dllsToCopy ${DLIBS_TO_COPY})		
		GET_FILENAME_COMPONENT(name ${dllsToCopy} NAME)
		STRING(REGEX REPLACE "[^5]+$" "" lnname ${name})
		INSTALL(CODE "EXECUTE_PROCESS (COMMAND ln -sf ${CMAKE_INSTALL_PREFIX}/${LIB_DIST}/${name} ${CMAKE_INSTALL_PREFIX}/${LIB_DIST}/${lnname})")    
    ENDFOREACH(dllsToCopy ${DLIBS_TO_COPY})
ENDIF(MSVC)
		
ENDMACRO(INSTALL_QT)

MACRO(QT_ADD_POSTBUILD_STEP TARGET_NAME libLocation copyToSubdirectory)
    SET(libLocation_release ${libLocation})
    SET(REPLACE_PATTERN "/lib/([^/]+)$" "/bin/\\1") # from lib to bin
    IF(NOT EXISTS "${libLocation_release}")
	STRING(REGEX REPLACE ${REPLACE_PATTERN} libLocation_release ${libLocation_release})
	IF(NOT EXISTS "${libLocation_release}")
	    MESSAGE(WARNING "cannot add post_build step to ${libLocation_release}")
	ENDIF()
    ENDIF()
    IF(EXISTS "${libLocation_release}")
	SET(DLIBS_TO_COPY_RELEASE ${DLIBS_TO_COPY_RELEASE} ${libLocation_release})
	SET(DLIBS_TO_COPY_DEBUG ${DLIBS_TO_COPY_DEBUG} ${libLocation_debug})  
	IF(WIN32)
	    STRING(REGEX REPLACE ".dll" "d.dll" libLocation_debug ${libLocation_release})
	ELSEIF(APPLE)
	    STRING(REGEX REPLACE ".dylib" "d.dylib" libLocation_debug ${libLocation_release})
	ELSE()
	    STRING(REGEX REPLACE ".so" "d.so" libLocation_debug ${libLocation_release})
	ENDIF()
	    IF(WIN32 OR APPLE)
	        ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
		        ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:
		        ${libLocation_debug}> $<$<NOT:$<CONFIG:Debug>>:
		        ${libLocation_release}>  $<TARGET_FILE_DIR:${TARGET_NAME}>${copyToSubdirectory})
	    ENDIF()
	ENDIF()
ENDMACRO(QT_ADD_POSTBUILD_STEP)


MACRO(INSTALL_IMAGEFORMATS_HELPER TYPE)
    IF(${TYPE} STREQUAL "Debug")
        INSTALL(FILES ${QT_${imgFormatPlugin}_PLUGIN_DEBUG} DESTINATION ${IMAGEFORMATS_DIR} CONFIGURATIONS ${TYPE} COMPONENT Runtime)
        STRING(REPLACE "${QT_IMAGEFORMATS_PLUGINS_DIR}" "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/imageformats" 
            QT_IMAGEFORMATS_PLUGIN_LOCAL ${QT_${imgFormatPlugin}_PLUGIN_DEBUG}
            )
        LIST(APPEND BUNDLE_LIBRARIES_MOVE ${QT_IMAGEFORMATS_PLUGIN_LOCAL})
    ELSE()
        INSTALL(FILES ${QT_${imgFormatPlugin}_PLUGIN_RELEASE} DESTINATION ${IMAGEFORMATS_DIR} CONFIGURATIONS ${TYPE} COMPONENT Runtime)
        STRING(REPLACE "${QT_IMAGEFORMATS_PLUGINS_DIR}" "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/imageformats" 
            QT_IMAGEFORMATS_PLUGIN_LOCAL ${QT_${imgFormatPlugin}_PLUGIN_RELEASE}
            )
        LIST(APPEND BUNDLE_LIBRARIES_MOVE ${QT_IMAGEFORMATS_PLUGIN_LOCAL})
    ENDIF()
ENDMACRO(INSTALL_IMAGEFORMATS_HELPER TYPE)

MACRO(INSTALL_IMAGEFORMATS TARGET_NAME LIB_DIST)
IF(NOT DEVELOPER_QT5)
    IF(WIN32 OR APPLE)
        SET(MyImageFormats QICO QGIF QJPEG)
        IF(APPLE)
            GET_TARGET_PROPERTY(PROJECT_LOCATION ${TARGET_NAME} LOCATION)
            STRING(REPLACE "/Contents/MacOS/${TARGET_NAME}" "" MACOSX_BUNDLE_LOCATION ${PROJECT_LOCATION})
            STRING(REPLACE "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" "$(CONFIGURATION)" BUNDLE_ROOT ${MACOSX_BUNDLE_LOCATION})
            SET(IMAGEFORMATS_DIR ${BUNDLE_ROOT}/Contents/MacOS/imageformats)
        ELSE(APPLE)
            SET(IMAGEFORMATS_DIR imageformats)
        ENDIF(APPLE)
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} COMMAND ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats)
        
        FOREACH(imgFormatPlugin ${MyImageFormats})
            IF(QT_${imgFormatPlugin}_PLUGIN_DEBUG AND QT_${imgFormatPlugin}_PLUGIN_RELEASE)
                ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
                    ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:
                    ${QT_${imgFormatPlugin}_PLUGIN_DEBUG}> $<$<NOT:$<CONFIG:Debug>>:
                    ${QT_${imgFormatPlugin}_PLUGIN_RELEASE}>  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
                    )
            ELSEIF(QT_${imgFormatPlugin}_PLUGIN_RELEASE)
                ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
                    ${CMAKE_COMMAND} -E copy ${QT_${imgFormatPlugin}_PLUGIN_RELEASE} $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
                    )
            ELSEIF(QT_${imgFormatPlugin}_PLUGIN_DEBUG)
                ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
                    ${CMAKE_COMMAND} -E copy ${QT_${imgFormatPlugin}_PLUGIN_DEBUG} $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
                    )
            ENDIF(QT_${imgFormatPlugin}_PLUGIN_DEBUG AND QT_${imgFormatPlugin}_PLUGIN_RELEASE)
            
            IF(NOT CMAKE_BUILD_TYPE AND CMAKE_CONFIGURATION_TYPES)
                FOREACH(buildconfig ${CMAKE_CONFIGURATION_TYPES})
                    INSTALL_IMAGEFORMATS_HELPER(${buildconfig})
                ENDFOREACH(buildconfig ${CMAKE_CONFIGURATION_TYPES})
            ELSEIF(CMAKE_BUILD_TYPE)
                    INSTALL_IMAGEFORMATS_HELPER(${CMAKE_BUILD_TYPE})
            ENDIF(NOT CMAKE_BUILD_TYPE AND CMAKE_CONFIGURATION_TYPES)
        ENDFOREACH(imgFormatPlugin ${MyImageFormats})
    MESSAGE(STATUS ${BUNDLE_LIBRARIES_MOVE})
    ENDIF(WIN32 OR APPLE)
ELSE()
    GET_TARGET_PROPERTY(qtCoreLocation ${Qt5Core_LIBRARIES} LOCATION)
    STRING(REGEX REPLACE "(lib|bin)/Qt5Core(.*)" "plugins/imageformats" imageFormatsPath ${qtCoreLocation})
    STRING(REGEX REPLACE "(.*)(lib|bin)/Qt5Core." "" dllExtension ${qtCoreLocation})
    SET(MyImageFormats qico qgif qjpeg)

    IF(APPLE)
        GET_TARGET_PROPERTY(PROJECT_LOCATION ${TARGET_NAME} LOCATION)
        STRING(REPLACE "/Contents/MacOS/${TARGET_NAME}" "" MACOSX_BUNDLE_LOCATION ${PROJECT_LOCATION})
        STRING(REPLACE "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" "$(CONFIGURATION)" BUNDLE_ROOT ${MACOSX_BUNDLE_LOCATION})
        SET(IMAGEFORMATS_DIR ${BUNDLE_ROOT}/Contents/MacOS/imageformats)
    ELSE()
        SET(IMAGEFORMATS_DIR imageformats)
    ENDIF(APPLE)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} COMMAND
        ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats
        )

    FOREACH(imgFormatPlugin ${MyImageFormats})
        SET(imagePlugin_release "${imageFormatsPath}/${imgFormatPlugin}.${dllExtension}")
        QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${imagePlugin_release} "/imageformats/")
    ENDFOREACH(imgFormatPlugin ${MyImageFormats})
ENDIF(NOT DEVELOPER_QT5)
ENDMACRO(INSTALL_IMAGEFORMATS TARGET_NAME)

MACRO(INSTALL_QT5PLUGINS TARGET_NAME LIB_DIST)
	GET_TARGET_PROPERTY(qtCoreLocation ${Qt5Core_LIBRARIES} LOCATION)
IF(APPLE OR WIN32)
    STRING(REGEX REPLACE "(bin|lib)/Qt5Core(.*)" "plugins" qtPluginsPath ${qtCoreLocation})
    STRING(REGEX REPLACE "(.*)(bin|lib)/Qt5Core." "" dllExtension ${qtCoreLocation})
    ####### PLATFORMS #######
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} COMMAND
        ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/platforms
        )
    SET(platformPlugin_release "${qtPluginsPath}/platforms/qwindows.${dllExtension}")
    QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${platformPlugin_release} "/platforms/")
    #########################

    ####### accessible #######
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} COMMAND
        ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/accessible
        )
    SET(accessiblePlugin_release "${qtPluginsPath}/accessible/qtaccessiblewidgets.${dllExtension}")
    QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${accessiblePlugin_release} "/accessible/")
    #########################

    ######### MISC LIBS ONLY RELEASE ##########
    STRING(REGEX REPLACE "(bin|lib)/Qt5Core(.*)" "bin" qtBinPath ${qtCoreLocation})
    SET(MISC_LIBS icuuc49 icuin49 icudt49 icuuc51 icuin51 icudt51 D3DCompiler_43)
    FOREACH(miscLib ${MISC_LIBS})
        SET(miscLib_release "${qtBinPath}/${miscLib}.${dllExtension}")
        IF(EXISTS "${miscLib_release}")
            ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD COMMAND
                ${CMAKE_COMMAND} -E copy ${miscLib_release}  $<TARGET_FILE_DIR:${TARGET_NAME}>
                )		
        ENDIF()
    ENDFOREACH(miscLib ${MISC_LIBS})
    ########################

    ######## MISC LIBS DEBUG AND RELEASE #########
    SET(MISC_LIBS libEGL libGLESv2)
    FOREACH(miscLib ${MISC_LIBS})
        SET(miscLib_release "${qtBinPath}/${miscLib}.${dllExtension}")
        IF(EXISTS "${miscLib_release}")
            QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${miscLib_release} "")
        ENDIF()
    ENDFOREACH(miscLib ${MISC_LIBS})
    ###########################################
ELSEIF(UNIX)
    GET_FILENAME_COMPONENT(qtPluginsPath ${qtCoreLocation} PATH)
    SET(MISC_LIBS libicuuc.so.49 libicui18n.so.49 libicudata.so.49)
    FOREACH(miscLib ${MISC_LIBS})
        GET_FILENAME_COMPONENT(LibWithoutSymLink ${qtPluginsPath}/${miscLib} REALPATH)	
        IF(EXISTS "${LibWithoutSymLink}")
            INSTALL(FILES ${LibWithoutSymLink} DESTINATION ${LIB_DIST})	
            GET_FILENAME_COMPONENT(SymLinkName ${LibWithoutSymLink} NAME)
            INSTALL(CODE "EXECUTE_PROCESS (COMMAND ln -sf ${CMAKE_INSTALL_PREFIX}/${LIB_DIST}/${SymLinkName} ${CMAKE_INSTALL_PREFIX}/${LIB_DIST}/${miscLib})")		
        ENDIF()
    ENDFOREACH(miscLib ${MISC_LIBS})
ENDIF()	
ENDMACRO(INSTALL_QT5PLUGINS TARGET_NAME)

MACRO(INSTALL_QT_PLUGINS TARGET_NAME LIB_DIST)
	INSTALL_IMAGEFORMATS(${TARGET_NAME} ${LIB_DIST})
    IF(DEVELOPER_QT5)
        INSTALL_QT5PLUGINS(${TARGET_NAME} ${LIB_DIST})
    ENDIF(DEVELOPER_QT5)
ENDMACRO(INSTALL_QT_PLUGINS TARGET_NAME)
