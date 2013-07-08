macro(SET_DESKTOP_TARGET)
	# Set default target to build (under win32 its allows console debug)
	if(WIN32)
		option(DEVELOPER_WINCONSOLE "Enable windows console windows for debug" OFF)
		if(DEVELOPER_FEATURES AND DEVELOPER_WINCONSOLE)
			set(DESKTOP_TARGET 
				""
			)
		else(DEVELOPER_FEATURES AND DEVELOPER_WINCONSOLE)
			set(DESKTOP_TARGET 
				WIN32
			)
		endif(DEVELOPER_FEATURES AND DEVELOPER_WINCONSOLE)
	elseif(APPLE)
		set(DESKTOP_TARGET 
			MACOSX_BUNDLE
		)
	endif(WIN32)
endmacro(SET_DESKTOP_TARGET)

macro(DEFINE_DEFAULT_DEFINITIONS)
	add_definitions(
		-DNOMINMAX # do not define min() and max()
		-D_CRT_SECURE_NO_WARNINGS 
		-D_CRT_NONSTDC_NO_WARNINGS 
		-D__STDC_CONSTANT_MACROS
		#-DWIN32_LEAN_AND_MEAN # remove obsolete things from windows headers
	)

	add_definitions(
		-DPROJECT_NAME="${PROJECT_NAME}"
		-DPROJECT_FULLNAME="${PROJECT_FULLNAME}" 
		-DPROJECT_DOMAIN="${PROJECT_DOMAIN}" 
		-DPROJECT_COMPANYNAME="${PROJECT_COMPANYNAME}"
                -DPROJECT_VERSION="${${PROJECT_NAME}_VERSION}"
                -DPROJECT_NAME_LOWERCASE="${PROJECT_NAME_LOWERCASE}"
	)

	if(DEVELOPER_NEWFEATURE)
		add_definitions(
			-DDEVELOPER_NEWFEATURE
		)
	endif(DEVELOPER_NEWFEATURE)
	
	if(DEVELOPER_FEATURES)
		add_definitions(
			-DDEVELOPER_FEATURES
		)
		if(WIN32)
			set(CMAKE_EXE_LINKER_FLAGS_RELEASE 
				"${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG"
			)
			set(ADDITIONAL_LIBS 
				imagehlp.lib 
				Psapi.lib
			)
		endif(WIN32)
	endif(DEVELOPER_FEATURES)

	if(DEVELOPER_DISABLE_SETTINGS)
		add_definitions(-DDEVELOPER_DISABLE_SETTINGS)
	endif(DEVELOPER_DISABLE_SETTINGS)

	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zi")
	endif(WIN32)

endmacro(DEFINE_DEFAULT_DEFINITIONS)

macro(SETUP_COMPILER_SETTINGS IS_DYNAMIC)
	set(IS_DYNAMIC ${IS_DYNAMIC})

	string(REPLACE ";" " " cmake_cl_release_init_str "${ADDITIONAL_CL_OPTIMIZATION_OPTIONS} /D NDEBUG /EHsc")
	string(REPLACE ";" " " cmake_linker_release_init_str "${ADDITIONAL_LINKER_OPTIMIZATION_OPTIONS}")
		
	if(IS_DYNAMIC)

		set(makeRulesOwerrideContent "

			if(WIN32)

				set(CMAKE_C_FLAGS_DEBUG_INIT 			\"/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1\")
				set(CMAKE_C_FLAGS_MINSIZEREL_INIT     	\"/MD /O1 /Ob1 /D NDEBUG\")
				set(CMAKE_C_FLAGS_RELEASE_INIT       	\"/MD /O2 /Ob2 /D NDEBUG\")
				set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT 	\"/MD /Zi /O2 /Ob1 /D NDEBUG\")

				set(CMAKE_CXX_FLAGS_DEBUG_INIT 			\"/D_DEBUG /MDd /Zi /Ob0 /Od /EHsc /RTC1\")
				set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     \"/MD /O1 ${cmake_cl_release_init_str}\")
				set(CMAKE_CXX_FLAGS_RELEASE_INIT        \"/MD /O2 ${cmake_cl_release_init_str}\")
				set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT \"/MD /Zi /O2 /Ob1 /D NDEBUG /EHsc\")

				set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT		\"${cmake_linker_release_init_str}\")
				set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT			\"${cmake_linker_release_init_str}\")
				set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT	\"${cmake_linker_release_init_str}\")

			endif(WIN32)

"		)

		if(WIN32)
			set(CMAKE_CXX_FLAGS_DEBUG "/MDd /D_DEBUG /Zi /Ob0 /Od /RTC1")
			set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 ${cmake_cl_release_init_str}")
			set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /O1 ${cmake_cl_release_init_str}")
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /O2 /Zi ${cmake_cl_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${cmake_linker_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${cmake_linker_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${cmake_linker_release_init_str}")
		endif(WIN32)

		message("Using setting for DYNAMIC run-time")

	else(IS_DYNAMIC)

		set(makeRulesOwerrideContent "

			if(WIN32)

				set(CMAKE_C_FLAGS_INIT \"/MT\")
				set(CMAKE_CXX_FLAGS_INIT \"/MT /EHsc\")

				set(CMAKE_C_FLAGS_DEBUG_INIT 			\"/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1\")
				set(CMAKE_C_FLAGS_MINSIZEREL_INIT     	\"/MT /O1 /Ob1 /D NDEBUG\")
				set(CMAKE_C_FLAGS_RELEASE_INIT       	\"/MT /O2 /Ob2 /D NDEBUG\")
				set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT 	\"/MT /Zi /O2 /Ob1 /D NDEBUG\")

				set(CMAKE_CXX_FLAGS_DEBUG_INIT 			\"/D_DEBUG /MTd /Zi /Ob0 /Od /EHsc /RTC1\")
				set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     \"/MT /O1 ${cmake_cl_release_init_str}\")
				set(CMAKE_CXX_FLAGS_RELEASE_INIT        \"/MT /O2 ${cmake_cl_release_init_str}\")
				set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT \"/MT /Zi /O2 /Ob1 /D NDEBUG /EHsc\")

				set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT		\"${cmake_linker_release_init_str}\")
				set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT			\"${cmake_linker_release_init_str}\")
				set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT	\"${cmake_linker_release_init_str}\")
			endif(WIN32)

"		)

		if(WIN32)
			set(CMAKE_CXX_FLAGS_DEBUG "/MTd /D_DEBUG /Zi /Ob0 /Od /RTC1")
			set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 ${cmake_cl_release_init_str}")
			set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /O1 ${cmake_cl_release_init_str}")
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /O2 /Zi ${cmake_cl_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${cmake_linker_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${cmake_linker_release_init_str}")
			set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${cmake_linker_release_init_str}")
		endif(WIN32)

		message("Using setting for STATIC run-time")

		add_definitions(
			-DQT_STATICPLUGINS
		)
	endif(IS_DYNAMIC)

	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/makeRulesOwerride.cmake" "${makeRulesOwerrideContent}")
	set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_BINARY_DIR}/makeRulesOwerride.cmake)
	
	if(APPLE)
		#set(LIBCXX_DIR	${CMAKE_CURRENT_SOURCE_DIR}/imports/libc++10.7)
		#set(CMAKE_OSX_ARCHITECTURES i386)
		#set(CMAKE_OSX_ARCHITECTURES_DEBUG i368)
		#set(CMAKE_CXX_FLAGS "-arch i386")
		#set(LINK_FLAGS "-arch i386")
		#set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
		#set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
		#set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -nostdinc++ -I${LIBCXX_DIR}/include -g -Wall")
		#set(LINK_FLAGS "${LINK_FLAGS} -L${LIBCXX_DIR}/lib -arch i386")
		
		# Detection target arch automaticly
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -g -Wall")
		set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	elseif(UNIX)
                #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
	endif()

	if(MSVC)
		set(DEVELOPER_NUM_BUILD_PROCS 3 CACHE STRING "Parameter for /MP option")
		if(${DEVELOPER_NUM_BUILD_PROCS} GREATER 1)
			string(REGEX REPLACE "/MP([0-9]+)" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${DEVELOPER_NUM_BUILD_PROCS}")
		endif(${DEVELOPER_NUM_BUILD_PROCS} GREATER 1)
	endif(MSVC)
endmacro(SETUP_COMPILER_SETTINGS IS_DYNAMIC)


macro(INSTALL_RUNTIME_LIBRARIES)
	# Install CRT
	if(WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
		include (InstallRequiredSystemLibraries)
	endif(WIN32)

# TODO: add code for other platforms here

endmacro(INSTALL_RUNTIME_LIBRARIES)

macro(ADD_DLIB_TO_POSTBUILD_STEP TARGET_NAME DLIB_NAME)
if(MSVC OR APPLE)
	get_target_property(dlib_location ${DLIB_NAME} LOCATION)
	get_filename_component(dlib_targetdir ${dlib_location} PATH)
	set(DLIB_DEBUG ${dlib_targetdir}/${CMAKE_SHARED_LIBRARY_PREFIX}${DLIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
	set(DLIB_RELEASE ${dlib_targetdir}/${CMAKE_SHARED_LIBRARY_PREFIX}${DLIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
		 ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:${DLIB_DEBUG}> $<$<NOT:$<CONFIG:Debug>>:${DLIB_RELEASE}>  $<TARGET_FILE_DIR:${TARGET_NAME}>
        )
endif(MSVC OR APPLE)
endmacro(ADD_DLIB_TO_POSTBUILD_STEP)

macro(INSTALL_DEBUG_INFO_FILE)

if(WIN32)
	#install pbd files
	foreach(buildCfg ${CMAKE_CONFIGURATION_TYPES})
		install(
			FILES ${CMAKE_CURRENT_BINARY_DIR}/${buildCfg}/${PROJECT_NAME}.pdb
			DESTINATION .
			CONFIGURATIONS ${buildCfg})
	endforeach(buildCfg ${CMAKE_CONFIGURATION_TYPES})
endif(WIN32)
# TODO: add code for other platforms here

endmacro(INSTALL_DEBUG_INFO_FILE)


macro(TARGET_BUNDLEFIX TARGET_NAME)
	if(APPLE)
		get_target_property(PROJECT_LOCATION ${TARGET_NAME} LOCATION)
		string(REPLACE "/Contents/MacOS/${TARGET_NAME}" "" MACOSX_BUNDLE_LOCATION ${PROJECT_LOCATION})
		string(REPLACE "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" "$(CONFIGURATION)" BUNDLE_ROOT ${MACOSX_BUNDLE_LOCATION})

		install(CODE "
			include(BundleUtilities)
			message(STATUS \"Fixing bundle: ${BUNDLE_ROOT}\")
			fixup_bundle(
					\"${BUNDLE_ROOT}\" 
					\"${BUNDLE_LIBRARIES_MOVE}\" 
					\"${BUNDLE_DIRECTORY_MOVE}\"
			)					
			 " 
			COMPONENT Runtime
		)
	endif(APPLE)
endmacro(TARGET_BUNDLEFIX TARGET_NAME)

