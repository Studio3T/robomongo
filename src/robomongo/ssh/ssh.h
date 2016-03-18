#ifndef ROBOMONGO_SSH_H
#define ROBOMONGO_SSH_H

#ifdef __cplusplus
extern "C" {
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
    char *sshserverip;
    unsigned int sshserverport;  // SSH port
    char *remotehost;   // Resolved by the remote server
    unsigned int remoteport;
};

int ssh_init();
void ssh_cleanup();
int ssh_open_tunnel(struct ssh_tunnel_config config);

#ifdef __cplusplus
}
#endif

#endif // ROBOMONGO_SSH_H
