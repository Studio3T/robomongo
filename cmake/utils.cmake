macro(VersionConf prjName from_file to_file)
set(COMPANYNAME "\"${PROJECT_COPYRIGHT}\"")
string(REPLACE "." ";" versionList ${PROJECT_VERSION})

list(GET versionList 0 MAJOR_VER)
list(GET versionList 1 MINOR_VER1)
list(GET versionList 2 MINOR_VER2)
list(GET versionList 3 MINOR_VER3)

set(filecontent "
	set(MAJOR_VER	${MAJOR_VER} ) 	
	set(MINOR_VER1	${MINOR_VER1})
	set(MINOR_VER2	${MINOR_VER2})
	set(MINOR_VER3	${MINOR_VER3})
	set(PRODUCTNAME ${prjName})
	set(COMPANYNAME ${COMPANYNAME})
	set(PRODUCTDOMAIN ${PROJECT_DOMAIN})
	configure_file(${from_file} ${to_file})
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/versionConfFile.cmake" "${filecontent}")

add_custom_target(VersionConfSvn ALL DEPENDS ${to_file})
add_custom_command(OUTPUT ${to_file}
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_BINARY_DIR}/versionConfFile.cmake)

set_source_files_properties(
	${to_file}
    PROPERTIES GENERATED TRUE
    HEADER_FILE_ONLY TRUE
)

add_dependencies(${prjName} VersionConfSvn)

endmacro(VersionConf)