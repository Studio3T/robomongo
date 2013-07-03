macro(DETECT_QT)
	find_package( Qt5Core QUIET )
	if(Qt5Core_FOUND)
  		set(DEVELOPER_QT5 1)
	else(Qt5Core_FOUND)
  		set(DEVELOPER_QT5 0)
	endif(Qt5Core_FOUND)
endmacro(DETECT_QT)

macro(QTX_WRAP_CPP)
	if(DEVELOPER_QT5)
		QT5_WRAP_CPP(${ARGN})
	else(DEVELOPER_QT5)
		QT4_WRAP_CPP(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_WRAP_CPP)

macro(QTX_GENERATE_MOC)
	if(DEVELOPER_QT5)
		QT5_GENERATE_MOC(${ARGN})
	else(DEVELOPER_QT5)
		QT4_GENERATE_MOC(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_GENERATE_MOC)

macro(QTX_ADD_TRANSLATION)
	if(DEVELOPER_QT5)
		QT5_ADD_TRANSLATION(${ARGN})
	else(DEVELOPER_QT5)
		QT4_ADD_TRANSLATION(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_ADD_TRANSLATION)

macro(QTX_CREATE_TRANSLATION)
	if(DEVELOPER_QT5)
		QT5_CREATE_TRANSLATION(${ARGN})
	else(DEVELOPER_QT5)
		QT4_CREATE_TRANSLATION(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_CREATE_TRANSLATION)

macro(QTX_WRAP_UI)
	if(DEVELOPER_QT5)
		QT5_WRAP_UI(${ARGN})
	else(DEVELOPER_QT5)
		QT4_WRAP_UI(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_WRAP_UI)

macro(QTX_ADD_RESOURCES)
	if(DEVELOPER_QT5)
		QT5_ADD_RESOURCES(${ARGN})
	else(DEVELOPER_QT5)
		QT4_ADD_RESOURCES(${ARGN})
	endif(DEVELOPER_QT5)
endmacro(QTX_ADD_RESOURCES)


macro(INTEGRATE_QT)

if(DEVELOPER_QT5)
	set(USE_QT_DYNAMIC ON)
	set(QT_COMPONENTS_TO_USE ${ARGV})
	
	if(DEVELOPER_BUILD_TESTS)
		set(QT_COMPONENTS_TO_USE ${QT_COMPONENTS_TO_USE} Qt5Test)
	endif(DEVELOPER_BUILD_TESTS)	
	
	foreach(qtComponent ${QT_COMPONENTS_TO_USE})
		if(NOT ${qtComponent} STREQUAL "Qt5ScriptTools")
			find_package(${qtComponent} REQUIRED)
		else()
			find_package(${qtComponent} QUIET)
		endif()		
		
		include_directories( ${${qtComponent}_INCLUDE_DIRS} )		
		#STRING(REGEX REPLACE "Qt5" "" COMPONENT_SHORT_NAME ${qtComponent})		
		#set(QT_MODULES_TO_USE ${QT_MODULES_TO_USE} ${COMPONENT_SHORT_NAME})
		if(${${qtComponent}_FOUND} AND NOT(${qtComponent} STREQUAL "Qt5LinguistTools"))
			STRING(REGEX REPLACE "Qt5" "" componentShortName ${qtComponent})		
			set(QT_LIBRARIES ${QT_LIBRARIES} "Qt5::${componentShortName}")
		endif()
	
	endforeach(qtComponent ${QT_COMPONENTS_TO_USE})

	if(NOT Qt5ScriptTools_FOUND)
		add_definitions(-DQT_NO_SCRIPTTOOLS)
	endif()

	if(DEVELOPER_OPENGL)
		find_package(Qt5OpenGL REQUIRED)
		include_directories(
			${Qt5OpenGL_INCLUDE_DIRS}
		)
        endif(DEVELOPER_OPENGL)	
else(DEVELOPER_QT5)

	set(QT_COMPONENTS_TO_USE ${ARGV})
	
	# repeat this for every debug/optional package
	list(FIND QT_COMPONENTS_TO_USE "QtScriptTools" QT_SCRIPT_TOOLS_INDEX) 
	if(NOT ${QT_SCRIPT_TOOLS_INDEX} EQUAL -1)
		list(REMOVE_ITEM QT_COMPONENTS_TO_USE "QtScriptTools")
		set(QT_DEBUG_COMPONENTS_TO_USE ${QT_DEBUG_COMPONENTS_TO_USE} "QtScriptTools")
	endif()

	if(DEVELOPER_BUILD_TESTS)
		set(QT_COMPONENTS_TO_USE ${QT_COMPONENTS_TO_USE} QtTest)
	endif(DEVELOPER_BUILD_TESTS)	

	
	find_package(Qt4 COMPONENTS
		${QT_COMPONENTS_TO_USE}
		REQUIRED
	)
	
	find_package(Qt4 COMPONENTS 
		${QT_DEBUG_COMPONENTS_TO_USE}
		QUIET
	)
	
	if(NOT QT_QTSCRIPTTOOLS_FOUND)
		add_definitions(-DQT_NO_SCRIPTTOOLS)
	endif()
	
	include(${QT_USE_FILE})
	
	set(QT_3RDPARTY_DIR
		${QT_BINARY_DIR}/../src/3rdparty
	)

	#checking is Qt dynamic or statis version will be used
	get_filename_component(QTCORE_DLL_NAME_WE ${QT_QTCORE_LIBRARY_RELEASE} NAME_WE)
	get_filename_component(QT_LIB_PATH ${QT_QTCORE_LIBRARY_RELEASE} PATH)

	if(WIN32)
		set(DLL_EXT dll)

		# message("searching ${QTCORE_DLL_NAME_WE}.${DLL_EXT} in " ${QT_LIB_PATH} ${QT_LIB_PATH}/../bin)

		find_file(QTCORE_DLL_FOUND_PATH ${QTCORE_DLL_NAME_WE}.${DLL_EXT}
			PATHS ${QT_LIB_PATH} ${QT_LIB_PATH}/../bin

			NO_DEFAULT_PATH
			NO_CMAKE_ENVIRONMENT_PATH
			NO_CMAKE_PATH
			NO_SYSTEM_ENVIRONMENT_PATH
			NO_CMAKE_SYSTEM_PATH
		)
		message("QTCORE_DLL_FOUND_PATH=" ${QTCORE_DLL_FOUND_PATH})
			
		if(EXISTS ${QTCORE_DLL_FOUND_PATH})
			set(USE_QT_DYNAMIC ON)
		else(EXISTS ${QTCORE_DLL_FOUND_PATH})
			set(USE_QT_DYNAMIC OFF)
		endif(EXISTS ${QTCORE_DLL_FOUND_PATH})

	else()
		set(USE_QT_DYNAMIC ON)
	endif()

	# use jscore
	if(WIN32 AND NOT USE_QT_DYNAMIC)
		find_library(JSCORE_LIB_RELEASE jscore
			PATHS
			${QT_3RDPARTY_DIR}/webkit/Source/JavaScriptCore/release
			${QT_3RDPARTY_DIR}/webkit/JavaScriptCore/release
		)
	
		find_library(JSCORE_LIB_DEBUG NAMES jscored jscore
			PATHS
			${QT_3RDPARTY_DIR}/webkit/Source/JavaScriptCore/debug
			${QT_3RDPARTY_DIR}/webkit/JavaScriptCore/debug
		)
	
		set(JSCORE_LIBS
			optimized ${JSCORE_LIB_RELEASE}
			debug ${JSCORE_LIB_DEBUG}
		)
	endif(WIN32 AND NOT USE_QT_DYNAMIC)
	# end of use jscore
endif(DEVELOPER_QT5)	

endmacro(INTEGRATE_QT)


##############################################################################

macro(INSTALL_QT TARGET_NAME)
if(NOT DEVELOPER_QT5)
if(WIN32 OR APPLE)
	set(QT_COMPONENTS_TO_USE ${ARGV})
	list(REMOVE_ITEM QT_COMPONENTS_TO_USE ${TARGET_NAME})

	find_package(Qt4 COMPONENTS
		${QT_COMPONENTS_TO_USE}
		REQUIRED
	)
	find_package(Qt4 COMPONENTS
		${QT_DEBUG_COMPONENTS_TO_USE}
		QUIET
	)
	include(${QT_USE_FILE})

	get_target_property(targetLocation ${TARGET_NAME} LOCATION)
	get_filename_component(targetDir ${targetLocation} PATH)

		if(WIN32)
			set(REPLACE_IN_LIB ".lib" ".dll")
			set(REPLACE_IN_LIB2 "/lib/([^/]+)$" "/bin/\\1")
		elseif(APPLE)
			set(REPLACE_IN_LIB ".a" ".dylib")
			set(REPLACE_IN_LIB2 "" "")
		endif()

		foreach(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})
			string(TOUPPER "${qtComponent}" QtComCap)
			target_link_libraries(${TARGET_NAME} ${QT_${QtComCap}_LIBRARY})
			if(NOT ${qtComponent} STREQUAL "QtMain")
				string(REPLACE ${REPLACE_IN_LIB} dllNameDeb "${QT_${QtComCap}_LIBRARY_DEBUG}")
				if(WIN32)
					string(REGEX REPLACE ${REPLACE_IN_LIB2} dllNameDeb ${dllNameDeb})
				endif()
				set(DLIBS_TO_COPY_DEBUG
					${DLIBS_TO_COPY_DEBUG}
					${dllNameDeb}
				)
				if(NOT ${qtComponent} STREQUAL "QtScriptTools")
					string(REPLACE ${REPLACE_IN_LIB} dllNameRel "${QT_${QtComCap}_LIBRARY_RELEASE}")
					if(WIN32)
						string(REGEX REPLACE ${REPLACE_IN_LIB2} dllNameRel ${dllNameRel})
					endif()
					set(DLIBS_TO_COPY_RELEASE
						${DLIBS_TO_COPY_RELEASE}
						${dllNameRel}
					)
				endif()
				# TODO: check this code @ MAC and *ux
				add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:${dllNameDeb}> $<$<NOT:$<CONFIG:Debug>>:${dllNameRel}>  $<TARGET_FILE_DIR:${TARGET_NAME}>
				)

			endif(NOT ${qtComponent} STREQUAL "QtMain")
			
		endforeach(qtComponent ${QT_COMPONENTS_TO_USE})
endif(WIN32 OR APPLE)
else()# Qt5

	list(FIND QT_COMPONENTS_TO_USE "Qt5Xml" QT_XML_INDEX) 
	if(NOT ${QT_XML_INDEX} EQUAL -1)
		get_target_property(libLocation ${Qt5Xml_LIBRARIES} LOCATION)
		string(REGEX REPLACE "Xml" "XmlPatterns" libLocation ${libLocation})
		QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${libLocation} "")
	endif()

	foreach(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})
		if(NOT ${qtComponent} STREQUAL "Qt5LinguistTools")
			if(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
			get_target_property(libLocation ${${qtComponent}_LIBRARIES} LOCATION)
			QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${libLocation} "")
			else(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
				message("Canont find library ${qtComponent}_LIBRARIES")
			endif(NOT "${${qtComponent}_LIBRARIES}" STREQUAL "")
		endif()
	endforeach(qtComponent ${QT_COMPONENTS_TO_USE} ${QT_DEBUG_COMPONENTS_TO_USE})	
endif(NOT DEVELOPER_QT5)

if(WIN32)
	if(NOT CMAKE_BUILD_TYPE)
		# Visual studio install
		foreach(buildconfig ${CMAKE_CONFIGURATION_TYPES})
			message(STATUS "VC configuration install for ${buildconfig} ${DLIBS_TO_COPY_RELEASE}")
			if(${buildconfig} STREQUAL "Debug")
				set(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_DEBUG})
			else()
				set(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_RELEASE})
			endif()

			install(FILES
				${DLIBS_TO_COPY}
				DESTINATION .
				CONFIGURATIONS ${buildconfig}
			)
		endforeach(buildconfig ${CMAKE_CONFIGURATION_TYPES})

	else(NOT CMAKE_BUILD_TYPE)
		# Make install
		message(STATUS "Make configuration install ${DLIBS_TO_COPY_RELEASE}")
		if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
			set(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_DEBUG})
		else()
			set(DLIBS_TO_COPY ${DLIBS_TO_COPY_ALL} ${DLIBS_TO_COPY_RELEASE})
		endif()

		install(FILES
			${DLIBS_TO_COPY}
			DESTINATION .
		)
	endif(NOT CMAKE_BUILD_TYPE)
endif(WIN32)
		
endmacro(INSTALL_QT)

macro(QT_ADD_POSTBUILD_STEP TARGET_NAME libLocation copyToSubdirectory)
	set(libLocation_release ${libLocation})

	#message(-------------QT_ADD_POSTBUILD_STEP---------------)
	set(REPLACE_PATTERN "/lib/([^/]+)$" "/bin/\\1") # from lib to bin
	
	if(NOT EXISTS "${libLocation_release}")
		string(REGEX REPLACE ${REPLACE_PATTERN} libLocation_release ${libLocation_release})
		if(NOT EXISTS "${libLocation_release}")
			message(FATAL_ERROR "cannot add post_build step to ${libLocation_release}")
		endif()
	endif()	
	set(DLIBS_TO_COPY_RELEASE ${DLIBS_TO_COPY_RELEASE} ${libLocation_release})
	set(DLIBS_TO_COPY_DEBUG ${DLIBS_TO_COPY_DEBUG} ${libLocation_debug})
	
	if(WIN32)
		string(REGEX REPLACE ".dll" "d.dll" libLocation_debug ${libLocation_release})
	elseif(APPLE)
		string(REGEX REPLACE ".dylib" "d.dylib" libLocation_debug ${libLocation_release})
	else()
		string(REGEX REPLACE ".so" "d.so" libLocation_debug ${libLocation_release})
	endif()

	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
		 ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:${libLocation_debug}> $<$<NOT:$<CONFIG:Debug>>:${libLocation_release}>  $<TARGET_FILE_DIR:${TARGET_NAME}>${copyToSubdirectory}
	)		
	#message(-------------------------------------------------)
endmacro(QT_ADD_POSTBUILD_STEP)


macro(INSTALL_IMAGEFORMATS_HELPER TYPE)
	if(${TYPE} STREQUAL "Debug")
		install(FILES
			${QT_${imgFormatPlugin}_PLUGIN_DEBUG}
			DESTINATION ${IMAGEFORMATS_DIR}
			CONFIGURATIONS ${TYPE}
			COMPONENT Runtime
		)
		string(REPLACE "${QT_IMAGEFORMATS_PLUGINS_DIR}" "${IMAGEFORMATS_DIR}" QT_IMAGEFORMATS_PLUGIN_LOCAL ${QT_${imgFormatPlugin}_PLUGIN_DEBUG})
		list(APPEND BUNDLE_LIBRARIES_MOVE ${QT_IMAGEFORMATS_PLUGIN_LOCAL})
	else()
		install(FILES
				${QT_${imgFormatPlugin}_PLUGIN_RELEASE}
				DESTINATION ${IMAGEFORMATS_DIR}
				CONFIGURATIONS ${TYPE}
				COMPONENT Runtime
		)
		string(REPLACE "${QT_IMAGEFORMATS_PLUGINS_DIR}" "${IMAGEFORMATS_DIR}" QT_IMAGEFORMATS_PLUGIN_LOCAL ${QT_${imgFormatPlugin}_PLUGIN_RELEASE})
		list(APPEND BUNDLE_LIBRARIES_MOVE ${QT_IMAGEFORMATS_PLUGIN_LOCAL})
	endif()
endmacro(INSTALL_IMAGEFORMATS_HELPER TYPE)

macro(INSTALL_IMAGEFORMATS TARGET_NAME)
if(NOT DEVELOPER_QT5)
	if(WIN32 OR APPLE)
		set(MyImageFormats QICO QGIF QJPEG)		

		if(APPLE)
			get_target_property(PROJECT_LOCATION ${TARGET_NAME} LOCATION)
			string(REPLACE "/Contents/MacOS/${TARGET_NAME}" "" MACOSX_BUNDLE_LOCATION ${PROJECT_LOCATION})
			string(REPLACE "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" "$(CONFIGURATION)" BUNDLE_ROOT ${MACOSX_BUNDLE_LOCATION})

			set(IMAGEFORMATS_DIR ${BUNDLE_ROOT}/Contents/MacOS/imageformats)
		else(APPLE)
			set(IMAGEFORMATS_DIR imageformats)
		endif(APPLE)


		add_custom_command(TARGET ${PROJECT_NAME} COMMAND
			${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats
		)

		foreach(imgFormatPlugin ${MyImageFormats})
			if(QT_${imgFormatPlugin}_PLUGIN_DEBUG AND QT_${imgFormatPlugin}_PLUGIN_RELEASE)
			    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
				${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:${QT_${imgFormatPlugin}_PLUGIN_DEBUG}> $<$<NOT:$<CONFIG:Debug>>:${QT_${imgFormatPlugin}_PLUGIN_RELEASE}>  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
			    )
			elseif(QT_${imgFormatPlugin}_PLUGIN_RELEASE)
			    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
				${CMAKE_COMMAND} -E copy ${QT_${imgFormatPlugin}_PLUGIN_RELEASE} $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
			    )
			elseif(QT_${imgFormatPlugin}_PLUGIN_DEBUG)
			    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
				${CMAKE_COMMAND} -E copy ${QT_${imgFormatPlugin}_PLUGIN_DEBUG} $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats/
			    )
			endif(QT_${imgFormatPlugin}_PLUGIN_DEBUG AND QT_${imgFormatPlugin}_PLUGIN_RELEASE)
			
			if(NOT CMAKE_BUILD_TYPE AND CMAKE_CONFIGURATION_TYPES)
				foreach(buildconfig ${CMAKE_CONFIGURATION_TYPES})
					INSTALL_IMAGEFORMATS_HELPER(${buildconfig})
				endforeach(buildconfig ${CMAKE_CONFIGURATION_TYPES})
			elseif(CMAKE_BUILD_TYPE)
				INSTALL_IMAGEFORMATS_HELPER(${CMAKE_BUILD_TYPE})
			endif(NOT CMAKE_BUILD_TYPE AND CMAKE_CONFIGURATION_TYPES)
		endforeach(imgFormatPlugin ${MyImageFormats})
	
		message(STATUS ${BUNDLE_LIBRARIES_MOVE})
	endif(WIN32 OR APPLE)
else()
	if(WIN32 OR APPLE)
		get_target_property(qtCoreLocation ${Qt5Core_LIBRARIES} LOCATION)
		string(REGEX REPLACE "lib/Qt5Core(.*)" "plugins/imageformats" imageFormatsPath ${qtCoreLocation})
		string(REGEX REPLACE "(.*)lib/Qt5Core." "" dllExtension ${qtCoreLocation})
		set(MyImageFormats qico qgif qjpeg)
		
		if(APPLE)
			get_target_property(PROJECT_LOCATION ${TARGET_NAME} LOCATION)
			string(REPLACE "/Contents/MacOS/${TARGET_NAME}" "" MACOSX_BUNDLE_LOCATION ${PROJECT_LOCATION})
			string(REPLACE "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" "$(CONFIGURATION)" BUNDLE_ROOT ${MACOSX_BUNDLE_LOCATION})

			set(IMAGEFORMATS_DIR ${BUNDLE_ROOT}/Contents/MacOS/imageformats)
		else(APPLE)
			set(IMAGEFORMATS_DIR imageformats)
		endif(APPLE)
		
		add_custom_command(TARGET ${PROJECT_NAME} COMMAND
			${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats
		)

		foreach(imgFormatPlugin ${MyImageFormats})
			set(imagePlugin_release "${imageFormatsPath}/${imgFormatPlugin}.${dllExtension}")
			QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${imagePlugin_release} "/imageformats/")
		endforeach(imgFormatPlugin ${MyImageFormats})
	endif(WIN32 OR APPLE)
endif(NOT DEVELOPER_QT5)
endmacro(INSTALL_IMAGEFORMATS TARGET_NAME)

macro(INSTALL_QT5PLUGINS TARGET_NAME)
	if(WIN32 OR APPLE)
		get_target_property(qtCoreLocation ${Qt5Core_LIBRARIES} LOCATION)
		string(REGEX REPLACE "(bin|lib)/Qt5Core(.*)" "plugins" qtPluginsPath ${qtCoreLocation})
		string(REGEX REPLACE "(.*)(bin|lib)/Qt5Core." "" dllExtension ${qtCoreLocation})
		
		####### PLATFORMS #######
		add_custom_command(TARGET ${PROJECT_NAME} COMMAND
			${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/platforms
		)
		set(platformPlugin_release "${qtPluginsPath}/platforms/qwindows.${dllExtension}")
		QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${platformPlugin_release} "/platforms/")
		#########################
		
		####### accessible #######
		add_custom_command(TARGET ${PROJECT_NAME} COMMAND
			${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/accessible
		)
		set(accessiblePlugin_release "${qtPluginsPath}/accessible/qtaccessiblewidgets.${dllExtension}")
		QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${accessiblePlugin_release} "/accessible/")
		#########################	
		
		######### MISC LIBS ONLY RELEASE ##########
		string(REGEX REPLACE "(bin|lib)/Qt5Core(.*)" "bin" qtBinPath ${qtCoreLocation})
		set(MISC_LIBS icuuc49 icuin49 icudt49 icuuc51 icuin51 icudt51 D3DCompiler_43)
		foreach(miscLib ${MISC_LIBS})
			set(miscLib_release "${qtBinPath}/${miscLib}.${dllExtension}")
			if(EXISTS "${miscLib_release}")
				add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy ${miscLib_release}  $<TARGET_FILE_DIR:${TARGET_NAME}>
				)		
			endif()
		endforeach(miscLib ${MISC_LIBS})
		########################
		
		######## MISC LIBS DEBUG AND RELEASE #########
		set(MISC_LIBS libEGL libGLESv2)
		foreach(miscLib ${MISC_LIBS})
			set(miscLib_release "${qtBinPath}/${miscLib}.${dllExtension}")		
			if(EXISTS "${miscLib_release}")
				QT_ADD_POSTBUILD_STEP(${TARGET_NAME} ${miscLib_release} "")
			endif()
		endforeach(miscLib ${MISC_LIBS})
		###########################################
	endif(WIN32 OR APPLE)
endmacro(INSTALL_QT5PLUGINS TARGET_NAME)

macro(INSTALL_QT_PLUGINS TARGET_NAME)
	INSTALL_IMAGEFORMATS(${TARGET_NAME})
	if(DEVELOPER_QT5)
		INSTALL_QT5PLUGINS(${TARGET_NAME})
	endif(DEVELOPER_QT5)
endmacro(INSTALL_QT_PLUGINS TARGET_NAME)
