#include "robomongo/ssh/ssh.h"
#include "robomongo/ssh/internal.h"
#include "robomongo/ssh/libssh2_config.h"

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
#include <stdarg.h>
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

/*
 * Initialises Sockets and Libssh
 * Returns 0 if initialization succeeds
 */
int rbm_ssh_init() {
    int err;
#ifdef WIN32
    WSADATA wsadata;
#endif

#ifdef WIN32
    err = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (err != 0) {
        log_error("WSAStartup failed with error: %d", err);
        return 1;
    }
#endif

    err = libssh2_init (0);
    if (err != 0) {
        log_error("libssh2 initialization failed (%d)", err);
        return 1;
    }

    return 0;
}

/*
 * Cleanups Sockets and Libssh
 */
void rbm_ssh_cleanup() {
    libssh2_exit();

#ifdef WIN32
    WSACleanup();
#endif
}


/*
 * Returns socket if succeed, otherwise -1 on error
 */
static rbm_socket_t socket_connect(rbm_ssh_session* session, char *ip, int port) {
    rbm_socket_t sock;
    struct sockaddr_in sin;

    /* Connect to SSH server */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == rbm_socket_invalid) {
        ssh_log_error(session, "Failed to open socket");
        return -1;
    }

    sin.sin_family = AF_INET;
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(ip))) {
        ssh_log_error(session, "Call to inet_addr failed");
        return -1;
    }

    sin.sin_port = htons(port);
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
        ssh_log_error(session, "Failed to connect to %s:%d", ip, port);
        return -1;
    }

    return sock;
}

/*
 * Returns socket (binded and in listen state) if succeed, otherwise (rbm_socket_invalid) on error
 */
rbm_socket_t socket_listen(rbm_ssh_session *rsession, char *ip, int *port) {
    rbm_socket_t listensock;
    struct sockaddr_in sin;
    int sockopt;
    socklen_t sinlen;

    listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listensock == rbm_socket_invalid) {
        ssh_log_error(rsession, "Failed to open socket");
        return rbm_socket_invalid;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(0); // Bind to any available port (htons is not needed, but still it's here)
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(ip))) {
        ssh_log_error(rsession, "inet_addr");
        return rbm_socket_invalid;
    }

    sockopt = 1;
    setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    sinlen = sizeof(sin);
    if (-1 == bind(listensock, (struct sockaddr *)&sin, sinlen)) {
        ssh_log_error(rsession, "Cannot bind to port %d", port);
        return rbm_socket_invalid;
    }

    if (-1 == listen(listensock, 2)) {
        ssh_log_error(rsession, "Failed to listen opened socket");
        return rbm_socket_invalid;
    }

    if (getsockname(listensock, (struct sockaddr *)&sin, &sinlen) == -1) {
        ssh_log_error(rsession, "Failed to get socket address");
        return rbm_socket_invalid;
    }

    *port = ntohs(sin.sin_port);
    return listensock;

}

/*
 * Returns 0 if error.
 */
LIBSSH2_SESSION *ssh_connect(rbm_ssh_session *rsession, rbm_socket_t sock, enum rbm_ssh_auth_type type, char *username, char *password,
                             char *publickeypath, char *privatekeypath, char *passphrase) {
    int rc, i, auth = RBM_SSH_AUTH_TYPE_NONE;
    LIBSSH2_SESSION *session;
    const char *fingerprint;
    char *userauthlist;

    ssh_log_debug(rsession, "ssh_connect: username: %s", username);
    ssh_log_debug(rsession, "ssh_connect: password: %s", password);
    ssh_log_debug(rsession, "ssh_connect: privatekeyfile: %s", privatekeypath);
    ssh_log_debug(rsession, "ssh_connect: publickeyfile: %s", publickeypath);

    /* Create a session instance */
    session = libssh2_session_init();
    if (!session) {
        ssh_log_error(rsession, "Could not initialize SSH session");
        return 0;
    }

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        ssh_log_error(rsession, "Error when starting up SSH session: %d\n", rc);
        return 0;
    }

    /* At this point we havn't yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */
    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

    /*
    fprintf(stderr, "Fingerprint: ");
    for(i = 0; i < 20; i++)
        fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
    fprintf(stderr, "\n");
    */

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(session, username, strlen(username));
    ssh_log_debug(rsession, "Authentication methods: %s", userauthlist);
    if (strstr(userauthlist, "password"))
        auth |= RBM_SSH_AUTH_TYPE_PASSWORD;
    if (strstr(userauthlist, "publickey"))
        auth |= RBM_SSH_AUTH_TYPE_PUBLICKEY;

    // If authentication by password is available
    // and was chosen by the user, then use it
    if (auth & RBM_SSH_AUTH_TYPE_PASSWORD && type == RBM_SSH_AUTH_TYPE_PASSWORD) {
        if (libssh2_userauth_password(session, username, password)) {
            ssh_log_error(rsession, "Authentication by password failed");
            return 0;
        }
        ssh_log_debug(rsession, "Authentication by password succeeded.");
        // If authentication by key is available
        // and was chosen by the user, then use it
    } else if (auth & RBM_SSH_AUTH_TYPE_PUBLICKEY && type == RBM_SSH_AUTH_TYPE_PUBLICKEY) {
        rc = libssh2_userauth_publickey_fromfile(session, username, publickeypath, privatekeypath, passphrase);
        if (rc) {
            ssh_log_error(rsession, "Authentication by key (%s) failed (Error %d)", privatekeypath, rc);
            return 0;
        }
        ssh_log_debug(rsession, "Authentication by key succeeded.");
    } else {
        ssh_log_error(rsession, "No supported authentication methods found");
        return 0;
    }

    return session;
}


// Returns NULL on error, or valid rbm_ssh_channel otherwise.
rbm_ssh_channel *ssh_channel_create(rbm_ssh_session* session, rbm_socket_t socket, LIBSSH2_CHANNEL *lchannel) {
    const int bufsize = 16384;
    rbm_ssh_channel **channels;

    rbm_ssh_channel *channel = malloc(sizeof(rbm_ssh_channel));
    channel->session = session;
    channel->channel = lchannel;
    channel->socket = socket;
    channel->inbuf = malloc(sizeof(char) * bufsize);
    channel->outbuf = malloc(sizeof(char) * bufsize);
    channel->bufmaxsize = bufsize;

    channels = (rbm_ssh_channel**) realloc (session->channels, (session->channelssize + 1) * sizeof(rbm_ssh_channel*)); // acts like malloc when (session->channels == NULL)
    if (!channels) {
        ssh_log_error(session, "Not enough memory (call to realloc)");
        return NULL;
    }

    channels[session->channelssize] = channel;

    session->channels = channels;
    ++session->channelssize;
    return channel;
}

void ssh_channel_close(rbm_ssh_channel* channel) {
    int i;
    rbm_ssh_session* session;
    rbm_ssh_channel** channels;

    session = channel->session;

    for (i = 0; i < session->channelssize; i++) {
        if (session->channels[i] != channel) {
            continue;
        }

        if (session->channelssize == 1) {
            channels = NULL;
        } else {
            channels = (rbm_ssh_channel**) malloc((session->channelssize - 1) * sizeof(rbm_ssh_channel*));
            memcpy(channels, session->channels, i * sizeof(rbm_ssh_channel*));

            if (i + 1 < session->channelssize) {
                memcpy(channels + i, session->channels + i + 1, (session->channelssize - i - 1) * sizeof(rbm_ssh_channel*));
            }
        }

        free(session->channels);
        session->channels = channels;
        --session->channelssize;

        // 1. Free libssh2 channel
        if (channel->channel) {
            libssh2_channel_free(channel->channel);
            channel->channel = NULL;
        }

        // 2. Free input/output buffers
        free(channel->inbuf);
        channel->inbuf = NULL;
        free(channel->outbuf);
        channel->outbuf = NULL;

        // 3. Close socket
        if (channel->socket != rbm_socket_invalid) {
            close(channel->socket);
            channel->socket = rbm_socket_invalid;
        }

        // 4. Free channel struct
        free(channel);

        ssh_log_debug(session, "Channel closed");
        break;
    }
}

rbm_ssh_channel* ssh_channel_find_by_socket(rbm_ssh_session* session, rbm_socket_t socket) {
    for (int i = 0; i < session->channelssize; i++) {
        if (session->channels[i]->socket == socket) {
            return session->channels[i];
        }
    }

    return NULL;
}

// Return -1 on error. 0 otherwise.
static int handle_new_client_connections(rbm_ssh_session *connection, int *fdmax, fd_set *masterset) {
    rbm_socket_t local_socket = connection->localsocket;
    rbm_socket_t ssh_socket = connection->sshsocket;
    LIBSSH2_SESSION* session = connection->sshsession;
    struct rbm_ssh_tunnel_config* config = connection->config;
    rbm_socket_t newfd = rbm_socket_invalid;
    const int ERROR = -1;
    const int SUCCESS = 0;

    ssh_log_debug(connection, "Data on accept socket is available");

    struct sockaddr_in remoteaddr;
    socklen_t slen = sizeof(remoteaddr);

    // handle new connections
    if ((newfd = accept(local_socket, (struct sockaddr *) &remoteaddr, &slen)) == -1) {
        ssh_log_error(connection, "Error on accept()");
        return ERROR;
    } else {
        FD_SET(newfd, masterset); // add to master set

        if (newfd > *fdmax) {       // keep track of the maximum
            *fdmax = newfd;
        }

        ssh_log_debug(connection, "New connection from %s on socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
    }

    LIBSSH2_CHANNEL *channel = NULL;
    int maxattempts = 25; //45;
    int attempts = 0;
    while (attempts < maxattempts) {
        ++attempts;
        channel = libssh2_channel_direct_tcpip_ex(session, config->remotehost, config->remoteport,
                                                  config->localip, config->localport);

        int errsave = errno;
        if (!channel) {
            ssh_log_warn(connection, "Could not open the direct TCP/IP channel (%d)", channel);

            // Error 35: Resource temporarily unavailable.
            if (errsave == 35) {
                usleep(200 * 1000);
                continue;
            }

            break;
        }

        ssh_log_debug(connection, "Channel successfully created!");
        break;
    }

    if (!channel) {
        ssh_log_error(connection, "Failed to create SSH channel");
        return ERROR;
    }

    if (ssh_channel_create(connection, newfd, channel) == NULL) {
        return ERROR;
    }

    return SUCCESS;
}

//  0: success
// -1:
static int handle_ssh_connections(rbm_ssh_session *connection, fd_set *masterset) {
    const int AGAIN = -2;
    const int ERROR = -1;
    const int SUCCESS = 0;
    struct rbm_ssh_tunnel_config* config = connection->config;

    ssh_log_debug(connection, "Data on SSH socket is available");
    ssh_log_debug(connection, "[%d]  <-  Number of channels", connection->channelssize);

    if (connection->channelssize == 0) {
        FD_CLR(connection->localsocket, masterset); // remove from master set
        FD_CLR(connection->sshsocket, masterset);   // remove from master set
        return SUCCESS;
    }

    int s = 0;
    int result = SUCCESS;
    int eagain = 0;
    while (s < connection->channelssize) {
        rbm_ssh_channel *context = connection->channels[s];
        ++s;

        int firstflag = 1;
        while (1) {
            int len;
            len = libssh2_channel_read(context->channel, context->outbuf, context->bufmaxsize);
            if (len == LIBSSH2_ERROR_EAGAIN) {

                if (firstflag) {
                    ++eagain;

                    if (eagain == connection->channelssize) {
                        result = AGAIN;
                        ssh_log_warn(connection, "All channels are in a non ready state (EAGAIN)");
                    }
                }

                // Proceed with the next channel
                break;
            } else if (len < 0) {

                // ETIMEDOUT (60) Connection timed out
                // We need to reconnect
                // if (errno == 60) {
                //    return 2;
                // }
                // Endless cycle:
                // Network is down. libssh2_channel_read: -43. (Error #50)

                result = ERROR;
                ssh_log_error(connection, "libssh2_channel_read: %d", len);
                break;
            }

            firstflag = 0;
            ssh_log_debug(connection, "Received %d bytes from tunnel", len);

            int wr = 0;
            int rc = 0; // result
            while (wr < len) {
                rc = send(context->socket, context->outbuf + wr, len - wr, 0);
                if (rc <= 0) {
                    result = ERROR;
                    ssh_log_error(connection, "Failure to write data to client");
                    break;
                }
                wr += rc;
            }
            if (libssh2_channel_eof(context->channel)) {
                result = SUCCESS;
                ssh_log_debug(connection, "The server at %s:%d disconnected!\n",
                    config->remotehost, config->remoteport);
                break;
            }
        }
    }

    return result;
}

static int handle_client_connections(rbm_ssh_session *connection, rbm_socket_t i, fd_set *masterset) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    int result = SUCCESS;
    ssh_log_debug(connection, "Data on client socket is available");

    rbm_ssh_channel *context = ssh_channel_find_by_socket(connection, i);
    if (!context) {
        close(i); // bye!
        FD_CLR(i, masterset); // remove from master set
        return ERROR;
    }

    // Read data from a client
    int nbytes = recv(context->socket, context->inbuf, context->bufmaxsize, 0);
    if (nbytes <= 0) {
        if (nbytes == 0) {
            // Normal situation
            result = SUCCESS;
            ssh_log_debug(connection, "Client disconnected");
        } else {
            // Got error
            result = ERROR;
            ssh_log_error(connection, "Error when recv()");
        }

        // In both these cases, close and cleanup connection
        close(context->socket); // bye!
        FD_CLR(context->socket, masterset); // remove from master set
        ssh_channel_close(context);

        if (connection->channelssize == 0) {
            FD_CLR(connection->localsocket, masterset); // remove from master set
            FD_CLR(connection->sshsocket, masterset);   // remove from master set
        }

        return result;
    }
    ssh_log_debug(connection, "Received %d bytes from client", nbytes);

    // Write data to ssh tunnel
    const int againmax = 100;
    int again = 0;
    int wr = 0;
    int rc = 0; // result
    while (wr < nbytes) {
        rc = libssh2_channel_write(context->channel, context->inbuf + wr, nbytes - wr);
        if (LIBSSH2_ERROR_EAGAIN == rc) {
            ++again;

            if (again > againmax) {
                ssh_log_warn(connection, "Number of attempts to libssh2_channel_write exсeed max value");
                return ERROR;
            }

            continue;
        }
        if (rc < 0) {
            ssh_log_error(connection, "Failed to write to SSH channel");
            return ERROR;
        }
        wr += rc;
    }
    ssh_log_debug(connection, "Written %d bytes to tunnel", wr);

    return SUCCESS;
}

int rbm_ssh_open_tunnel_ex(rbm_ssh_session *connection) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    rbm_socket_t local_socket = connection->localsocket;
    rbm_socket_t ssh_socket = connection->sshsocket;

    const int maxerrors = 25;   // number of serial errors, when we probably should stop the loop
    int errors = 0;             // counter for serial errors
    int rc = 0;

    int fdmax;       // maximum socket (file descriptor) number
    int isocket;     // index for traversing sockets
    fd_set masterset, readset, clearset;
    FD_ZERO(&masterset);
    FD_ZERO(&clearset);

    // Add the listener to the master set
    FD_SET(local_socket, &masterset);
    FD_SET(ssh_socket, &masterset);

    // Keep track of the biggest file descriptor
    fdmax = local_socket > ssh_socket ? local_socket : ssh_socket;

    while (errors < maxerrors) {

        readset = masterset; // copy set

        // If readset has no descriptors, it means that
        // session is closed and we should stop our work
        if (!memcmp(&readset, &clearset, sizeof(fd_set)))
            break;

        ssh_log_debug(connection, "* Okay, we are ready to select.");
        if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
            ssh_log_error(connection, "Error on select()");
            break;
        }
        ssh_log_debug(connection, "* Selected!");

        // Run through the existing connections looking for data to read
        for(isocket = 0; isocket <= fdmax; isocket++) {

            // Skip connections that are not available for reading
            if (!FD_ISSET(isocket, &readset))
                continue;

            if (isocket == local_socket) {
                rc = handle_new_client_connections(connection, &fdmax, &masterset);
                goto next;
            }

            if (isocket == ssh_socket) {
                rc = handle_ssh_connections(connection, &masterset);
                goto next;
            }

            rc = handle_client_connections(connection, isocket, &masterset);

next:
            // Increment "errors" counter, if we found an error,
            // or zero it, if not.
            errors = (rc == -2) ? errors + 1 : 0;
            if (errors > 0) {
                ssh_log_warn(connection, "*** COLLECTED %d AGAIN ***", errors);
            }

            if (rc == -1) {
                ssh_log_warn(connection, "SSH tunnel shutdown because of error");
                return ERROR;
            }
        }
    }

    if (errors >= maxerrors) {
        ssh_log_warn(connection, "SSH tunnel shutdown because of series of successive EAGAIN errors");
        return ERROR;
    }

    rbm_ssh_session_close(connection);
    return SUCCESS;
}

void rbm_ssh_session_cleanup(rbm_ssh_session *session);

int rbm_ssh_open_tunnel(rbm_ssh_session *connection) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    int rc = 0;

    while (1) {
        rc = rbm_ssh_open_tunnel_ex(connection);
        if (rc == 0)
            break;

        // Cleanup SSH connection we hope that local connection
        // will not break so often
        rbm_ssh_session_cleanup(connection);
        ssh_log_warn(connection, "STARTING AGAIN!!!!!!!!!111");

        if (rbm_ssh_setup(connection) == -1) {
            rbm_ssh_session_close(connection);
            return ERROR;
        }
    }

    return SUCCESS;
}

void rbm_ssh_session_cleanup(rbm_ssh_session *session) {
    if (session == NULL) {
        return;
    }

    // Order is important:
    // 1. close accept socket
    // 2. free libssh2 channels (and client sockets)
    // 3. free libssh2 session
    // 4. close ssh socket

    // 2.
    ssh_log_debug(session, "Closing channels");
    while (session->channelssize > 0) {
        ssh_channel_close(session->channels[0]);
        ssh_log_debug(session, "Channel closed");
    }

    // 3.
    if (session->sshsession) {
        ssh_log_debug(session, "Closing SSH session");
        libssh2_session_disconnect(session->sshsession, "Client disconnecting normally");
        libssh2_session_free(session->sshsession);
        session->sshsession = NULL;
    }

    // 4.
    if (session->sshsocket != rbm_socket_invalid) {
        ssh_log_debug(session, "Closing SSH socket");
        close(session->sshsocket);
        session->sshsocket = rbm_socket_invalid;
    }
}

void rbm_ssh_session_close(rbm_ssh_session *session) {
    if (session->localsocket != rbm_socket_invalid) {
        ssh_log_debug(session, "Closing local accept socket");
        close(session->localsocket);
        session->localsocket = rbm_socket_invalid;
    }

    rbm_ssh_session_cleanup(session);

    ssh_log_debug(session, "SSH tunnel successfully closed.");
    free(session);
}

// Returns 0 on error, valid rbm_ssh_session* if no errors
rbm_ssh_session* rbm_ssh_session_create(struct rbm_ssh_tunnel_config *config) {
    rbm_ssh_session *session = (rbm_ssh_session *) malloc(sizeof(rbm_ssh_session));
    if (!session) {
        return NULL;
    }
    session->localsocket = rbm_socket_invalid;
    session->sshsocket = rbm_socket_invalid;
    session->sshsession = 0;
    session->config = config;
    session->channelssize = 0;
    session->channels = NULL;
    session->lasterror[0] = '\0';

    // Check that loglevel is valid
    if (config->loglevel != RBM_SSH_LOG_TYPE_ERROR &&
        config->loglevel != RBM_SSH_LOG_TYPE_WARN &&
        config->loglevel != RBM_SSH_LOG_TYPE_INFO &&
        config->loglevel != RBM_SSH_LOG_TYPE_DEBUG) {
        log_error("Invalid log level for SSH submodule");
        return NULL;
    }

    return session;
}

// Returns -1 on error, 0 when otherwise
int rbm_ssh_setup(rbm_ssh_session *session) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    rbm_ssh_tunnel_config *config = session->config;

    ssh_log_debug(session, "Connecting to SSH server (%s:%d)...", config->sshserverip, config->sshserverport);

    session->sshsocket = socket_connect(session, config->sshserverip, config->sshserverport);
    if (session->sshsocket == -1) {
        session->sshsocket = rbm_socket_invalid;
        return ERROR; // errors are already logged by socket_connect
    }

    session->sshsession = ssh_connect(session, session->sshsocket, config->authtype, config->username, config->password,
        config->publickeyfile, config->privatekeyfile, config->passphrase);
    if (session->sshsession == 0) {
        return ERROR; // errors are already logged by ssh_connect
    }

    // Must use non-blocking IO hereafter due to the current libssh3 API
    libssh2_session_set_blocking(session->sshsession, 0);

    return SUCCESS;
}

// Returns -1 on error, 0 when otherwise
int rbm_ssh_session_setup(rbm_ssh_session *session) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    rbm_ssh_tunnel_config *config = session->config;

    if (rbm_ssh_setup(session) == -1)
        return ERROR;

    session->localsocket = socket_listen(session, config->localip, (int *) &config->localport);
    if (session->localsocket == -1) {
        session->localsocket = rbm_socket_invalid;
        return ERROR; // errors are already logged by socket_listen
    }

    ssh_log_debug(session, "Waiting for TCP connection on %s:%d...", config->localip, config->localport);

    return SUCCESS;
}


