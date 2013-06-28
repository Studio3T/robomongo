#
# this module look for libxml (http://www.xmlsoft.org) support
# it will define the following values
#
# LibXml2_INCLUDE_DIR  = where libxml/xpath.h can be found
# LibXml2_LIBRARIES    = the library to link against libxml2
# LibXml2_FOUND        = set to 1 if libxml2 is found
#
IF(EXISTS ${PROJECT_CMAKE}/Libxml2Config.cmake)
  INCLUDE(${PROJECT_CMAKE}/Libxml2Config.cmake)
ENDIF(EXISTS ${PROJECT_CMAKE}/Libxml2Config.cmake)


set(Libxml2 xml2)
if(QMC_BUILD_STATIC)
  set(Libxml2 libxml2.a)
endif(QMC_BUILD_STATIC)


IF(Libxml2_INCLUDE_DIRS)


  FIND_PATH(LIBXML2_INCLUDE_DIR libxml/xpath.h ${Libxml2_INCLUDE_DIRS})
  FIND_LIBRARY(LIBXML2_LIBRARY xml2 ${Libxml2_LIBRARY_DIRS})


ELSE(Libxml2_INCLUDE_DIRS)


  FIND_PATH(LIBXML_INCLUDE_DIR libxml2/libxml/xpath.h PATHS ${LIBXML2_HOME}/include $ENV{LIBXML2_HOME}/include /usr/local/include /usr/include NO_DEFAULT_PATH)
  if(LIBXML_INCLUDE_DIR)
    set(LIBXML2_INCLUDE_DIR "${LIBXML_INCLUDE_DIR}/libxml2")
  else(LIBXML_INCLUDE_DIR)
    FIND_PATH(LIBXML2_INCLUDE_DIR libxml/xpath.h ${LIBXML2_HOME}/include $ENV{LIBXML2_HOME}/include)
  endif(LIBXML_INCLUDE_DIR)


  FIND_LIBRARY(LIBXML2_LIBRARIES ${Libxml2} 
    PATHS ${LIBXML2_HOME}/lib $ENV{LIBXML2_HOME}/lib /usr/local/lib /usr/lib NO_DEFAULT_PATH)


ENDIF(Libxml2_INCLUDE_DIRS)


SET(LIBXML2_FOUND FALSE)
IF(LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
  message(STATUS "LIBXML2_INCLUDE_DIR="${LIBXML2_INCLUDE_DIR})
  message(STATUS "LIBXML2_LIBRARIES="${LIBXML2_LIBRARIES})
  SET(LIBXML2_FOUND TRUE)
ENDIF()


MARK_AS_ADVANCED(
  LIBXML2_INCLUDE_DIR 
  LIBXML2_LIBRARIES 
  LIBXML2_FOUND
  )