#include "ssh.h"

#include <unistd.h>

int main(int argc, char *argv[]) {
    struct rbm_ssh_tunnel_config config;

    config.authtype = RBM_SSH_AUTH_TYPE_PUBLICKEY;
    config.localip = "127.0.0.1";
    config.localport = 27040;
    config.username = "dmitry";
    config.password = "";
    config.privatekeyfile = "/Users/dmitry/.ssh/ubuntik";
    config.publickeyfile = NULL; //"/Users/dmitry/.ssh/ubuntik.pub";
    config.passphrase = "";
    config.sshserverip = "198.61.166.171";
    config.sshserverport = 22;
    config.remotehost = "localhost";
    config.remoteport = 27017;

    int err = 0;

    err = rbm_ssh_init();
    if (err) {
        return 1; // errors are already logged by init
    }

    int res = 0;
    while (1) {
        rbm_ssh_session* connection;
        connection = rbm_ssh_session_create(&config);
        if (!connection) {
            break;
        }

        res = rbm_ssh_open_tunnel(connection);
        if (res == 2) {
            usleep(100 * 1000);
            continue;
        }

        break;
    }

    rbm_ssh_cleanup();
}
