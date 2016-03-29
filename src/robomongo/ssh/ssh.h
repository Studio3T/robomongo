#ifndef ROBOMONGO_SSH_H
#define ROBOMONGO_SSH_H

#include <libssh2.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  define rbm_socket_t SOCKET
#  define rbm_socket_invalid INVALID_SOCKET
#else
#  define rbm_socket_t int
#  define rbm_socket_invalid (-1)
#endif

enum rbm_ssh_log_type {
    RBM_SSH_LOG_TYPE_ERROR  = 1,
    RBM_SSH_LOG_TYPE_WARN   = 2,
    RBM_SSH_LOG_TYPE_INFO   = 3,
    RBM_SSH_LOG_TYPE_DEBUG  = 100 // log as much as possible
};

enum rbm_ssh_auth_type {
    RBM_SSH_AUTH_TYPE_NONE       = 0,
    RBM_SSH_AUTH_TYPE_PASSWORD   = 1,
    RBM_SSH_AUTH_TYPE_PUBLICKEY  = 2
};

struct rbm_ssh_session;

/*
 * SSH Tunnel configuration
 */
typedef struct rbm_ssh_tunnel_config {
    enum rbm_ssh_auth_type authtype;
    enum rbm_ssh_log_type loglevel;

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

    void *context; // pointer to user-defined data
    void (*logcallback)(struct rbm_ssh_session* session, char *message, int level);
} rbm_ssh_tunnel_config;

typedef struct rbm_ssh_channel {
    struct rbm_ssh_session* session;
    LIBSSH2_CHANNEL *channel;
    rbm_socket_t socket;
    char *inbuf;
    char *outbuf;
    int bufmaxsize;
} rbm_ssh_channel;

typedef struct rbm_ssh_session {
    rbm_socket_t localsocket;
    rbm_socket_t sshsocket;
    LIBSSH2_SESSION *sshsession;
    rbm_ssh_tunnel_config *config;

    rbm_ssh_channel **channels;
    int channelssize; // number of channels

    char lasterror[2048];
} rbm_ssh_session;

int rbm_ssh_init();
void rbm_ssh_cleanup();

rbm_ssh_session* rbm_ssh_session_create(struct rbm_ssh_tunnel_config *config);
int rbm_ssh_session_setup(rbm_ssh_session *session);
void rbm_ssh_session_close(rbm_ssh_session *session);

int rbm_ssh_open_tunnel(struct rbm_ssh_session *connection);

#ifdef __cplusplus
}
#endif

#endif // ROBOMONGO_SSH_H
