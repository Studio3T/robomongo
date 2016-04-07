#include "ssh.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
    struct rbm_ssh_tunnel_config config;

    config.authtype = RBM_SSH_AUTH_TYPE_PUBLICKEY;
    config.localip = "127.0.0.1";
    config.localport = 27040;
    config.username = "dmitry";
    config.password = "";

#ifdef WIN32
    config.privatekeyfile = "d:\\ubuntik";
#else
    config.privatekeyfile = "/Users/dmitry/.ssh/ubuntik";
#endif

    config.publickeyfile = NULL; //"/Users/dmitry/.ssh/ubuntik.pub";
    config.passphrase = "";
    config.sshserverhost = "198.61.166.171";
    config.sshserverport = 22;
    config.remotehost = "localhost";
    config.remoteport = 27017;
    config.logcontext = NULL;
    config.loglevel = RBM_SSH_LOG_TYPE_DEBUG;

    if (rbm_ssh_init())
        return 1;

    struct rbm_ssh_session *session = NULL;
    session = rbm_ssh_session_create(&config);
    if (!session) {
        printf("Unable to create session. Shutdown\n");
        return 1;
    }

//    rbm_ssh_session_close(session);
    printf("II About to open tunnel...\n");

    if (rbm_ssh_session_setup(session) == -1) {
        printf("Setup failed. Shutdowning...\n");
        rbm_ssh_session_close(session);
        return 1;
    }

//    rbm_ssh_session_close(session);
//
    printf("About to open tunnel...\n");
    if (rbm_ssh_open_tunnel(session) != 0) {
        printf("Tunnel stopped because of error.\n");
        return 1;
    }
//
//    printf("Planned shutdown of the tunnel.");
    rbm_ssh_cleanup();
    return 0;
}
