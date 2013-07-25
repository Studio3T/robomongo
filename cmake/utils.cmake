macro(VersionConf prjName from_file to_file)
SET(COMPANYNAME "\"${PROJECT_COMPANYNAME}\"")

SET(filecontent "
    SET(ICON_FILE ${ICON_FILE})
	SET(MAJOR_VER	${MAJOR} ) 	
	SET(MINOR_VER1	${MINOR})
	SET(MINOR_VER2	${PATCH})
	SET(PRODUCTNAME ${prjName})
	SET(COMPANYNAME ${COMPANYNAME})
	SET(PRODUCTDOMAIN ${PROJECT_DOMAIN})
    SET(SHORTPRODUCTNAME ${prjName})
	configure_file(${from_file} ${to_file})   
")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/versionConfFile.cmake" "${filecontent}")

add_custom_target(VersionConf ALL DEPENDS ${to_file})
add_custom_command(OUTPUT ${to_file}
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_BINARY_DIR}/versionConfFile.cmake)

set_source_files_properties(
	${to_file}
    PROPERTIES GENERATED TRUE
    #HEADER_FILE_ONLY TRUE
)

add_dependencies(${prjName} VersionConf)

endmacro(VersionConf)
