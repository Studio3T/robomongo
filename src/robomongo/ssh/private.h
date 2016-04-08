#ifndef ROBOMONGO_SSH_PRIVATE_H
#define ROBOMONGO_SSH_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

// We do not need to use IPv6 for local bind. But we do support IPv6 for remote connection.
// It means we can ignore MSVC warning: "Use inet_ntop() or InetNtop() instead" of "inet_ntoa"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <libssh2.h>
#include <stdarg.h>         // for va_list in log functions

#include "robomongo/ssh/ssh.h"

#ifdef _WIN32
#include "robomongo/ssh/win.h"
#else
#include "robomongo/ssh/unix.h"
#endif


//===----------------------------------------------------------------------===//
// Common constants
//===----------------------------------------------------------------------===//

enum {
    RBM_SUCCESS = 0,
    RBM_ERROR   = -1,
    RBM_CHANNEL_CREATION_ERROR   = -10,
    RBM_BUFSIZE = 16384, // Size of the in/out buffers
};

//===----------------------------------------------------------------------===//
// Data structures
//===----------------------------------------------------------------------===//

struct rbm_channel {
    struct rbm_session* session;
    LIBSSH2_CHANNEL *channel;
    rbm_socket_t socket;
    char *inbuf;
    char *outbuf;
    int bufmaxsize;
};

struct rbm_session {
    rbm_socket_t localsocket;
    rbm_socket_t sshsocket;
    LIBSSH2_SESSION *sshsession;
    struct rbm_ssh_tunnel_config *config;

    struct rbm_channel **channels;      // array of channels
    int channelssize;                       // number of channels

    struct rbm_ssh_session *publicsession;
    char lasterror[2048];
};


// Channels
struct rbm_channel *rbm_channel_create(struct rbm_session* session, rbm_socket_t socket, LIBSSH2_CHANNEL *lchannel);
void rbm_channel_close(struct rbm_channel *channel);
struct rbm_channel *rbm_channel_find_by_socket(struct rbm_session *session, rbm_socket_t socket);

void rbm_session_cleanup(struct rbm_session *session);
int rbm_open_tunnel(struct rbm_session *connection);
int rbm_ssh_setup(struct rbm_session *session);
static rbm_socket_t socket_connect(struct rbm_session* session, char *ip, int port);
LIBSSH2_SESSION *ssh_connect(struct rbm_session *rsession, rbm_socket_t sock, enum rbm_ssh_auth_type type, char *username, char *password,
                             char *publickeypath, char *privatekeypath, char *passphrase);
rbm_socket_t socket_listen(struct rbm_session *rsession, char *ip, int *port);

//===----------------------------------------------------------------------===//
// Logging
//===----------------------------------------------------------------------===//

int log_error(const char *format, ...);
int log_msg(const char *format, ...);

void ssh_log_v(struct rbm_session *session, enum rbm_ssh_log_type type, const char *format, va_list args, int errsave);
void ssh_log_msg(struct rbm_session *session, const char *format, ...);
void ssh_log_warn(struct rbm_session *session, const char *format, ...);
void ssh_log_debug(struct rbm_session *session, const char *format, ...);
void ssh_log_error(struct rbm_session *session, const char *format, ...);


//===----------------------------------------------------------------------===//
// Utils
//===----------------------------------------------------------------------===//

int rbm_array_add(void ***array, int *currentsize, void *data);
int rbm_array_remove(void ***array, int *currentsize, void *data);

#ifdef __cplusplus
}
#endif
#endif // ROBOMONGO_SSH_PRIVATE_H
