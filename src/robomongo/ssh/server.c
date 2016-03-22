#include "ssh.h"

#include <unistd.h>

int main(int argc, char *argv[]) {
    struct ssh_tunnel_config config;

    config.authtype = AUTH_PUBLICKEY;
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

    err = ssh_init();
    if (err) {
        return 1; // errors are already logged by init
    }

    int res = 0;
    while (1) {
        res = ssh_open_tunnel(config);

        if (res == 2) {
            usleep(100 * 1000);
            continue;
        }

        break;
    }

    ssh_cleanup();
}
