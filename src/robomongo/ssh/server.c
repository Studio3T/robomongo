#include "libssh2_config.h"
#include <libssh2.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (in_addr_t)-1
#endif

#ifndef socket_type
#  ifdef WIN32
#    define socket_type SOCKET
#    define socket_invalid INVALID_SOCKET
#  else
#    define socket_type int
#    define socket_invalid (-1)
#  endif
#endif

/*
 * SSH Tunnel configuration
 */
struct ssh_tunnel_config {
    // Local IP and port to bind and listen to
    char *localip;
    unsigned int localport;

    // Username and password of remote user
    char *username;
    char *password;     // May be NULL or ""

    // Keys and optional passphrase
    char *privatekeyfile;
    char *publickeyfile;
    char *passphrase;   // May be NULL or ""

    // Remote IP and (host, port) in remote network
    char *serverip;
    unsigned int serverport;  // SSH port
    char *remotehost;   // Resolved by the remote server
    unsigned int remoteport;
};

void init() {

}

void cleanup() {

}

int main(int argc, char *argv[]) {
    struct ssh_tunnel_config config;
    config.localip = "127.0.0.1";
    config.localport = 27040;
    config.username = "dmitry";
    config.password = "";
    config.privatekeyfile = "/Users/dmitry/.ssh/ubuntik";
    config.publickeyfile = "/Users/dmitry/.ssh/ubuntik.pub";
    config.passphrase = "";
    config.serverip = "198.61.166.171";
    config.serverport = 22;
    config.remotehost = "localhost";
    config.remoteport = 27017;

    printf("Connecting to %s...\n", config.serverip);

}

