#include "robomongo/ssh/libssh2_config.h"
#include <libssh2.h>

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

#include "ssh.h"

/*
 * Initialises Sockets and Libssh
 * Returns 0 if initialization succeeds
 */
int ssh_init() {
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
void ssh_cleanup() {
    libssh2_exit();

#ifdef WIN32
    WSACleanup();
#endif
}

/*
 * Returns socket if succeed, otherwise -1 on error
 */
socket_type socket_connect(char *ip, int port) {
    socket_type sock;
    struct sockaddr_in sin;

    /* Connect to SSH server */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == socket_invalid) {
        log_error("Failed to open socket");
        return -1;
    }

    sin.sin_family = AF_INET;
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(ip))) {
        log_error("inet_addr error");
        return -1;
    }

    sin.sin_port = htons(port);
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
        log_error("Failed to connect to %s:%d", ip, port);
        return -1;
    }

    return sock;
}

/*
 * Returns socket (binded and in listen state) if succeed, otherwise -1 on error
 */
socket_type socket_listen(char *ip, int port) {
    socket_type listensock;
    struct sockaddr_in sin;
    int sockopt;
    socklen_t sinlen;

    listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listensock == socket_invalid) {
        log_error("Failed to open socket");
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(ip))) {
        log_error("inet_addr");
        return -1;
    }

    sockopt = 1;
    setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    sinlen = sizeof(sin);
    if (-1 == bind(listensock, (struct sockaddr *)&sin, sinlen)) {
        log_error("Cannot bind to port %d", port);
        return -1;
    }
    if (-1 == listen(listensock, 2)) {
        log_error("Failed to listen opened socket");
        return -1;
    }

    return listensock;

}

void test_connect(char *privatekeypath) {
    log_msg("test_connect: privatekeyfile: %s", privatekeypath);
}

/*
 * Returns 0 if error.
 */
LIBSSH2_SESSION *ssh_connect(socket_type sock, enum ssh_auth_type type, char *username, char *password,
                             char *publickeypath, char *privatekeypath, char *passphrase) {
    int rc, i, auth = AUTH_NONE;
    LIBSSH2_SESSION *session;
    const char *fingerprint;
    char *userauthlist;

    log_msg("ssh_connect: username: %s", username);
    log_msg("ssh_connect: password: %s", password);
    log_msg("ssh_connect: privatekeyfile: %s", privatekeypath);
    log_msg("ssh_connect: publickeyfile: %s", publickeypath);

    /* Create a session instance */
    session = libssh2_session_init();
    if (!session) {
        log_error("Could not initialize SSH session");
        return 0;
    }

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        log_error("Error when starting up SSH session: %d\n", rc);
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
    log_msg("Authentication methods: %s", userauthlist);
    if (strstr(userauthlist, "password"))
        auth |= AUTH_PASSWORD;
    if (strstr(userauthlist, "publickey"))
        auth |= AUTH_PUBLICKEY;

    // If authentication by password is available
    // and was chosen by the user, then use it
    if (auth & AUTH_PASSWORD && type == AUTH_PASSWORD) {
        if (libssh2_userauth_password(session, username, password)) {
            log_error("Authentication by password failed");
            return 0;
        }
        log_msg("Authentication by password succeeded.");
        // If authentication by key is available
        // and was chosen by the user, then use it
    } else if (auth & AUTH_PUBLICKEY && type == AUTH_PUBLICKEY) {
        rc = libssh2_userauth_publickey_fromfile(session, username, publickeypath, privatekeypath, passphrase);
        if (rc) {
            log_error("Authentication by key (%s) failed (Error %d).", privatekeypath, rc);
            return 0;
        }
        log_msg("Authentication by key succeeded.");
    } else {
        log_error("No supported authentication methods found.");
        return 0;
    }

    return session;
}


typedef struct {
    socket_type socket;
    LIBSSH2_CHANNEL *channel;
    char *inbuf;
    char *outbuf;
    int bufsize;
} session_context;

session_context *init_session_context(socket_type socket, LIBSSH2_CHANNEL *channel) {
    const int bufsize = 16384;
    session_context *cnt = malloc(sizeof(session_context));
    cnt->socket = socket;
    cnt->channel = channel;
    cnt->inbuf = malloc(sizeof(char) * bufsize);
    cnt->outbuf = malloc(sizeof(char) * bufsize);
    cnt->bufsize = bufsize;
    return cnt;
}

void free_session_context(session_context *context) {
    free(context->inbuf);
    free(context->outbuf);
    // free socket and channel
    free(context);
}

static session_context *session_contexts[1000];
static int session_context_empty_index = 0;

void add_session_context(session_context *context) {
    session_contexts[session_context_empty_index] = context;
    ++session_context_empty_index;
}

session_context *find_session_context_by_socket(socket_type socket) {
    for (int i = 0; i < session_context_empty_index; i++) {
        if (session_contexts[i]->socket == socket) {
            return session_contexts[i];
        }
    }

    return NULL;
}


int ssh_open_tunnel(struct ssh_connection* connection) {
    socket_type local_socket = connection->localsocket;
    socket_type ssh_socket = connection->sshsocket;
    LIBSSH2_SESSION* session = connection->sshsession;
    struct ssh_tunnel_config* config = connection->config;


    int fdmax;       // maximum file descriptor number
    int i;
    fd_set masterset, readset;   // master file descriptor list
    FD_ZERO(&masterset);
    socket_type newfd;

    // add the listener to the master set
    FD_SET(local_socket, &masterset);
    FD_SET(ssh_socket, &masterset);

    // keep track of the biggest file descriptor
    fdmax = local_socket; // so far, it's this one

    while (1) {
        readset = masterset; // copy it
        log_msg("* Okay, we are ready to select.");
        if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
            log_error("Error on select()");
            break;
        }
        log_msg("* Selected!");

        // Run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {

            // Skip connections that are not ready
            if (!FD_ISSET(i, &readset))
                continue;

            if (i == local_socket) {
                log_msg("Data on accept socket is available");

                struct sockaddr_in remoteaddr;
                socklen_t slen = sizeof(remoteaddr);

                // handle new connections
                if ((newfd = accept(local_socket, (struct sockaddr *) &remoteaddr, &slen)) == -1) {
                    log_error("Error on accept()");
                } else {
                    FD_SET(newfd, &masterset); // add to master set

                    if (newfd > fdmax) {       // keep track of the maximum
                        fdmax = newfd;
                    }

                    log_msg("New connection from %s on socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
                }

                LIBSSH2_CHANNEL *channel;
                int maxattempts = 25;
                int attempts = 0;
                while (attempts < maxattempts) {
                    ++attempts;
                    channel = libssh2_channel_direct_tcpip_ex(session, config->remotehost, config->remoteport,
                                                              config->localip, config->localport);
                    if (!channel) {
                        log_error("Could not open the direct TCP/IP channel (channel2)");
                        usleep(100 * 1000);
                        continue;
                    }

                    log_msg("Channel successfully created!");
                    break;
                }

                session_context* context = init_session_context(newfd, channel);
                add_session_context(context);
                continue;
            }

            if (i == ssh_socket) {
                log_msg("Data on SSH socket is available");

                log_msg("%d", session_context_empty_index);

                int s = 0;
                while (s < session_context_empty_index) {
                    session_context *context = session_contexts[s];
                    ++s;

                    while (1) {
                        int len;
                        len = libssh2_channel_read(context->channel, context->outbuf, context->bufsize);
                        if (LIBSSH2_ERROR_EAGAIN == len)
                            break;
                        else if (len < 0) {

                            // ETIMEDOUT (60) Connection timed out
                            // We need to reconnect
                            if (errno == 60) {
                                return 2;
                            }

                            log_error("libssh2_channel_read: %d", (int)len);
                            break;
                        }

                        log_msg("Received %d bytes from tunnel", len);

                        int wr = 0;
                        int rc = 0; // result
                        while (wr < len) {
                            rc = send(context->socket, context->outbuf + wr, len - wr, 0);
                            if (rc <= 0) {
                                log_error("Failure to write data to client");
                                break;
                            }
                            wr += rc;
                        }
                        if (libssh2_channel_eof(context->channel)) {
                            log_msg("The server at %s:%d disconnected!\n",
                                    config->remotehost, config->remoteport);
                            break;
                        }
                    }
                }


                continue;
            }

            log_msg("Data on client socket is available");

            session_context *context = find_session_context_by_socket(i);


            // Read data from a client
            int nbytes = recv(context->socket, context->inbuf, context->bufsize, 0);
            if (nbytes <= 0) {
                if (nbytes == 0) {
                    // Normal situation
                    log_msg("Client disconnected");
                } else {
                    // Got error
                    log_error("Error when recv()");
                }

                // In both these cases, close and cleanup connection
                close(context->socket); // bye!
                FD_CLR(context->socket, &masterset); // remove from master set

                continue;
            }

            log_msg("Received %d bytes from client", nbytes);


            // Write data to ssh tunnel
            int wr = 0;
            int rc = 0; // result
            while (wr < nbytes) {
                rc = libssh2_channel_write(context->channel, context->inbuf + wr, nbytes - wr);
                if (LIBSSH2_ERROR_EAGAIN == rc) {
                    continue;
                }
                if (rc < 0) {
                    log_error("Failed to write to SSH channel");
                    break;

                }
                wr += rc;
            }
            log_msg("Written %d bytes to tunnel", wr);
        }
    }

    // -------------------------------------

    return 0;
}

int ssh_esablish_connection(struct ssh_tunnel_config* config, struct ssh_connection* out) {
    log_msg("ssh_open_tunnel: username: %s", config->username);
    log_msg("ssh_open_tunnel: privatekeyfile: %s", config->privatekeyfile);

    test_connect(config->privatekeyfile);

    log_msg("Connecting to %s...", config->sshserverip);
    socket_type ssh_socket = socket_connect(config->sshserverip, config->sshserverport);
    if (ssh_socket == -1) {
        return 1; // errors are already logged by socket_connect
    }

    LIBSSH2_SESSION *session = ssh_connect(ssh_socket, config->authtype, config->username, config->password,
                                           config->publickeyfile, config->privatekeyfile, config->passphrase);
    if (session == 0) {
        return 1; // errors are already logged by ssh_connect
    }

    socket_type local_socket = socket_listen(config->localip, config->localport);
    if (local_socket == -1) {
        return 1; // errors are already logged by socket_listen
    }

    log_msg("Waiting for TCP connection on %s:%d...", config->localip, config->localport);
    // -------------------------------------

    // Must use non-blocking IO hereafter due to the current libssh3 API
    libssh2_session_set_blocking(session, 0);

    // ------------------------------------

    out->sshsession = session;
    out->localsocket = local_socket;
    out->sshsocket = ssh_socket;
    out->config = config;
    return 0;
}


