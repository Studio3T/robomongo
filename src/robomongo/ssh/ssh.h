#ifndef ROBOMONGO_SSH_H
#define ROBOMONGO_SSH_H

#ifdef __cplusplus
extern "C" {
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

/*
 * SSH Tunnel configuration
 */
struct rbm_ssh_tunnel_config {
    enum rbm_ssh_auth_type authtype;

    // Keys and optional passphrase
    char *privatekeyfile;
    char *publickeyfile;
    char *passphrase;   // May be NULL or ""

    // Local IP and port to bind and listen to
    char *localip;
    unsigned int localport;

    // Remote host (domain or IPv4/v6) and port to connect to
    char *remotehost;   // Resolved by the remote server
    unsigned int remoteport;

    // Username and password of remote user
    char *username;
    char *password;     // May be NULL or ""

    // Remote host (domain or IPv4/v6)
    char *sshserverhost;
    unsigned int sshserverport;

    // Logging facilities
    enum rbm_ssh_log_type loglevel;
    void *logcontext;   // Pointer to user-defined data (can be NULL)
    void (*logcallback)(void *logcontext, char *message, int level);
};

struct rbm_ssh_session {
    char *lasterror;
    void *handle;    // opaque pointer to rbm_session
};

int rbm_ssh_init();
void rbm_ssh_cleanup();

struct rbm_ssh_session* rbm_ssh_session_create(struct rbm_ssh_tunnel_config *config);
int rbm_ssh_open_tunnel(struct rbm_ssh_session *connection);
int rbm_ssh_session_setup(struct rbm_ssh_session *session);
void rbm_ssh_session_close(struct rbm_ssh_session *session);


#ifdef __cplusplus
}
#endif
#endif // ROBOMONGO_SSH_H
