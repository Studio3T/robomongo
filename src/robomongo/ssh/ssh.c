#include "ssh.h"

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

#include "log.h"

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
 * errsave: use 0, if you are not logging errors (i.e. errno == 0).
 */
static void ssh_log_v(rbm_ssh_session* session, enum rbm_ssh_log_type type, const char *format, va_list args, int errsave) {
    const int bufsize = 2000;
    char buf[bufsize];
    vsnprintf(buf, bufsize, format, args);

    if (type == RBM_SSH_LOG_TYPE_ERROR) {
        if (errsave) {
            sprintf(session->lasterror, "%s. %s. (Error #%d)", strerror(errsave), buf, errsave);
        } else {
            sprintf(session->lasterror, "%s", buf);
        }

        fprintf(stderr, "%s\n", session->lasterror);
        session->config->logcallback(session, session->lasterror, type);
        return;
    }

    if (type != RBM_SSH_LOG_TYPE_WARN &&
        type != RBM_SSH_LOG_TYPE_INFO &&
        type != RBM_SSH_LOG_TYPE_DEBUG)
        return;

    if (type > session->config->loglevel)
        return;

    printf("%s\n", buf);
    session->config->logcallback(session, buf, type);
}

static void ssh_log_msg(rbm_ssh_session* session, const char *format, ...) {
    const int type = RBM_SSH_LOG_TYPE_INFO;

    // For performance reasons, return as quick as possible,
    // if this level of logging is not enabled
    if (type > session->config->loglevel)
        return;

    va_list args;
    va_start(args, format);
    ssh_log_v(session, type, format, args, 0);
    va_end(args);
}

// When you faced with an error that you are planning to overcome or handle,
// log it as a warning. If you do not have plan how to proceed further, log
// as an error.
static void ssh_log_warn(rbm_ssh_session* session, const char *format, ...) {
    int errsave = errno;
    const int type = RBM_SSH_LOG_TYPE_WARN;

    // For performance reasons, return as quick as possible,
    // if this level of logging is not enabled
    if (type > session->config->loglevel)
        return;

    va_list args;
    va_start(args, format);
    ssh_log_v(session, type, format, args, errsave);
    va_end(args);
}

static void ssh_log_debug(rbm_ssh_session* session, const char *format, ...) {
    const int type = RBM_SSH_LOG_TYPE_DEBUG;

    // For performance reasons, return as quick as possible,
    // if this level of logging is not enabled
    if (type > session->config->loglevel)
        return;

    va_list args;
    va_start(args, format);
    ssh_log_v(session, type, format, args, 0);
    va_end(args);
}

static void ssh_log_error(rbm_ssh_session* session, const char *format, ...) {
    int errsave = errno;
    va_list args;
    va_start(args, format);
    ssh_log_v(session, RBM_SSH_LOG_TYPE_ERROR, format, args, errsave);
    va_end(args);
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
    int maxattempts = 45;
    int attempts = 0;
    while (attempts < maxattempts) {
        ++attempts;
        channel = libssh2_channel_direct_tcpip_ex(session, config->remotehost, config->remoteport,
                                                  config->localip, config->localport);
        if (!channel) {
            ssh_log_warn(connection, "Could not open the direct TCP/IP channel");
            usleep(200 * 1000);
            continue;
        }

        ssh_log_debug(connection, "Channel successfully created!");
        break;
    }

    if (!channel) {
        ssh_log_error(connection, "Failed to create SSH channel in %d attempts", maxattempts);
        return ERROR;
    }

    if (ssh_channel_create(connection, newfd, channel) == NULL) {
        return ERROR;
    }

    return SUCCESS;
}

//  0: success
// -1:
static int handle_ssh_connections(rbm_ssh_session *connection) {
    const int SUCCESS = 0;
    struct rbm_ssh_tunnel_config* config = connection->config;

    ssh_log_debug(connection, "Data on SSH socket is available");
    ssh_log_debug(connection, "[%d]  <-  Number of channels", connection->channelssize);

    if (connection->channelssize == 0) {
        rbm_ssh_session_close(connection);
        return SUCCESS;
    }

    int s = 0;
    while (s < connection->channelssize) {
        rbm_ssh_channel *context = connection->channels[s];
        ++s;

        while (1) {
            int len;
            len = libssh2_channel_read(context->channel, context->outbuf, context->bufmaxsize);
            if (LIBSSH2_ERROR_EAGAIN == len)
                break;
            else if (len < 0) {

                // ETIMEDOUT (60) Connection timed out
                // We need to reconnect
                if (errno == 60) {
                    return 2;
                }

                ssh_log_error(connection, "libssh2_channel_read: %d", (int)len);
                break;
            }

            ssh_log_debug(connection, "Received %d bytes from tunnel", len);

            int wr = 0;
            int rc = 0; // result
            while (wr < len) {
                rc = send(context->socket, context->outbuf + wr, len - wr, 0);
                if (rc <= 0) {
                    ssh_log_error(connection, "Failure to write data to client");
                    break;
                }
                wr += rc;
            }
            if (libssh2_channel_eof(context->channel)) {
                ssh_log_debug(connection, "The server at %s:%d disconnected!\n",
                            config->remotehost, config->remoteport);
                break;
            }
        }
    }

    return SUCCESS;
}

static void handle_client_connections(rbm_ssh_session *connection, rbm_socket_t i, fd_set *masterset) {
    ssh_log_debug(connection, "Data on client socket is available");

    rbm_ssh_channel *context = ssh_channel_find_by_socket(connection, i);
    if (!context) {
        close(i); // bye!
        FD_CLR(i, masterset); // remove from master set
        return;
    }

    // Read data from a client
    int nbytes = recv(context->socket, context->inbuf, context->bufmaxsize, 0);
    if (nbytes <= 0) {
        if (nbytes == 0) {
            // Normal situation
            ssh_log_debug(connection, "Client disconnected");
        } else {
            // Got error
            ssh_log_error(connection, "Error when recv()");
        }

        // In both these cases, close and cleanup connection
        close(context->socket); // bye!
        FD_CLR(context->socket, masterset); // remove from master set
        ssh_channel_close(context);
        return;
    }
    ssh_log_debug(connection, "Received %d bytes from client", nbytes);

    // Write data to ssh tunnel
    int wr = 0;
    int rc = 0; // result
    while (wr < nbytes) {
        rc = libssh2_channel_write(context->channel, context->inbuf + wr, nbytes - wr);
        if (LIBSSH2_ERROR_EAGAIN == rc) {
            continue;
        }
        if (rc < 0) {
            ssh_log_error(connection, "Failed to write to SSH channel");
            return;
        }
        wr += rc;
    }
    ssh_log_debug(connection, "Written %d bytes to tunnel", wr);

    return;
}

int rbm_ssh_open_tunnel(rbm_ssh_session *connection) {
    rbm_socket_t local_socket = connection->localsocket;
    rbm_socket_t ssh_socket = connection->sshsocket;

    int fdmax;       // maximum file descriptor number
    int i;
    fd_set masterset, readset;
    FD_ZERO(&masterset);

    // add the listener to the master set
    FD_SET(local_socket, &masterset);
    FD_SET(ssh_socket, &masterset);

    // keep track of the biggest file descriptor
    fdmax = local_socket; // so far, it's this one

    while (1) {
        readset = masterset; // copy it

        ssh_log_debug(connection, "* Okay, we are ready to select.");
        if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
            ssh_log_error(connection, "Error on select()");
            break;
        }
        ssh_log_debug(connection, "* Selected!");

        // Run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {

            // Skip connections that are not available for reading
            if (!FD_ISSET(i, &readset))
                continue;

            if (i == local_socket) {
                handle_new_client_connections(connection, &fdmax, &masterset);
                continue;
            }

            if (i == ssh_socket) {
                handle_ssh_connections(connection);
                continue;
            }

            handle_client_connections(connection, i, &masterset);
        }
    }

    return 0;
}

void rbm_ssh_session_close(rbm_ssh_session *session) {
    if (session == NULL) {
        return;
    }

    // Order is important:
    // 1. close client sockets
    // 2. close accept socket
    // 3. free libssh2 channels
    // 4. free libssh2 session
    // 5. close ssh socket


    // 2.
    if (session->localsocket != rbm_socket_invalid) {
        ssh_log_debug(session, "Closing local accept socket");
        close(session->localsocket);
        session->localsocket = rbm_socket_invalid;
    }

    // 3.
    while (session->channelssize > 0) {
        ssh_channel_close(session->channels[0]);
    }

    // 4.
    if (session->sshsession) {
        ssh_log_debug(session, "Closing SSH session");
        libssh2_session_disconnect(session->sshsession, "Client disconnecting normally");
        libssh2_session_free(session->sshsession);
        session->sshsession = NULL;
    }

    // 5.
    if (session->sshsocket != rbm_socket_invalid) {
        ssh_log_debug(session, "Closing SSH socket");
        close(session->sshsocket);
        session->sshsocket = rbm_socket_invalid;
    }

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
int rbm_ssh_session_setup(rbm_ssh_session *session) {
    const int ERROR = -1;
    const int SUCCESS = 0;
    rbm_ssh_tunnel_config *config = session->config;

    ssh_log_debug(session, "Connecting to SSH server (%s:%d)...", config->sshserverip, config->sshserverport);

    session->sshsocket = socket_connect(session, config->sshserverip, config->sshserverport);
    if (session->sshsocket == -1) {
        return ERROR; // errors are already logged by socket_connect
    }

    session->sshsession = ssh_connect(session, session->sshsocket, config->authtype, config->username, config->password,
        config->publickeyfile, config->privatekeyfile, config->passphrase);
    if (session->sshsession == 0) {
        return ERROR; // errors are already logged by ssh_connect
    }

    session->localsocket = socket_listen(session, config->localip, (int *) &config->localport);
    if (session->localsocket == -1) {
        return ERROR; // errors are already logged by socket_listen
    }

    ssh_log_debug(session, "Waiting for TCP connection on %s:%d...", config->localip, config->localport);

    // Must use non-blocking IO hereafter due to the current libssh3 API
    libssh2_session_set_blocking(session->sshsession, 0);

    return SUCCESS;
}


