cmake_minimum_required(VERSION 2.8)
project(libssh2_third_party)

FIND_PACKAGE(OpenSSL)
IF(OPENSSL_FOUND)
    #SET(OPENSSL_LIBS optimized ${LIB_EAY_RELEASE} debug ${LIB_EAY_DEBUG} optimized ${SSL_EAY_RELEASE} debug ${SSL_EAY_DEBUG})
    GET_FILENAME_COMPONENT(OPENSSL_ROOT_DIR ${OPENSSL_INCLUDE_DIR} PATH)
ELSE(OPENSSL_FOUND)
    IF(NOT OPENSSL_ROOT_DIR OR NOT EXISTS ${OPENSSL_ROOT_DIR})
        MESSAGE(FATAL_ERROR "OPENSSL_ENABLED but not founded, please specify OPENSSL_ROOT_DIR variable." )
    ENDIF()
    FILE(GLOB OPENSSL_LIBS "${OPENSSL_ROOT_DIR}/*.lib")
ENDIF(OPENSSL_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/include/ ${OPENSSL_INCLUDE_DIR})

SET(HEADERS ${HEADERS} 
include/libssh2.h
)
SET(SOURCES ${SOURCES} 
src/global.c 
src/session.c 
src/openssl.c 
src/keepalive.c 
src/misc.c
src/kex.c
src/transport.c
src/crypt.c
src/packet.c
src/hostkey.c
src/comp.c
src/channel.c
src/mac.c
src/userauth.c
)
	
source_group("Libssh2 third party files" FILES ${HEADERS} ${SOURCES})
add_library(libssh2_third_party STATIC ${HEADERS} ${SOURCES})
