#ifndef ROBOMONGO_SSH_H
#define ROBOMONGO_SSH_H

#ifdef __cplusplus
extern "C" {
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

int ssh_init();
void ssh_cleanup();
int ssh_open_tunnel(struct ssh_tunnel_config config);

#ifdef __cplusplus
}
#endif

#endif // ROBOMONGO_SSH_H
