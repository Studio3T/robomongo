#ifndef ROBOMONGO_SSH_H
#define ROBOMONGO_SSH_H

#include <libssh2.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#  define socket_type SOCKET
#  define socket_invalid INVALID_SOCKET
#else
#  define socket_type int
#  define socket_invalid (-1)
#endif

enum ssh_auth_type {
    AUTH_NONE       = 0,
    AUTH_PASSWORD   = 1,
    AUTH_PUBLICKEY  = 2
};

/*
 * SSH Tunnel configuration
 */
struct ssh_tunnel_config {
    enum ssh_auth_type authtype;

    // Keys and optional passphrase
    char *privatekeyfile;
    char *publickeyfile;
    char *passphrase;   // May be NULL or ""

    // Local IP and port to bind and listen to
    char *localip;
    unsigned int localport;

    // Remote host and port to connect to
    char *remotehost;   // Resolved by the remote server
    unsigned int remoteport;

    // Username and password of remote user
    char *username;
    char *password;     // May be NULL or ""

    // Remote IP and (host, port) in remote network
    char *sshserverip;
    unsigned int sshserverport;  // SSH port
};

struct ssh_connection {
    socket_type localsocket;
    socket_type sshsocket;
    LIBSSH2_SESSION *sshsession;

    struct ssh_tunnel_config *config;
};

int ssh_init();
void ssh_cleanup();

int ssh_esablish_connection(struct ssh_tunnel_config *config, struct ssh_connection *out);
int ssh_open_tunnel(struct ssh_connection *connection);

#ifdef __cplusplus
}
#endif

#endif // ROBOMONGO_SSH_H
