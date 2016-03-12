include(CheckIncludeFiles)
include(CheckSymbolExists)

# Direct-tcpip sample
add_executable(direct-tcpip
    direct_tcpip.c)

target_link_libraries(direct-tcpip
    PRIVATE
        libssh2)

target_include_directories(direct-tcpip
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})

# Server
add_executable(server
    server.c)

target_link_libraries(server
    PRIVATE
        libssh2)

target_include_directories(server
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})

## Platform checks
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(winsock2.h HAVE_WINSOCK2_H)

check_symbol_exists(strcasecmp strings.h HAVE_STRCASECMP)
check_symbol_exists(_stricmp string.h HAVE__STRICMP)
check_symbol_exists(snprintf stdio.h HAVE_SNPRINTF)
check_symbol_exists(_snprintf stdio.h HAVE__SNPRINTF)
check_symbol_exists(__func__ "" HAVE___FUNC__)
check_symbol_exists(__FUNCTION__ "" HAVE___FUNCTION__)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libssh2_config_cmake.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/libssh2_config.h)
