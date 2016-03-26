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

static void ssh_log_msg(ssh_session* session, const char *format, ...) {
    const int buf_size = 2000;
    char buf[buf_size];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, buf_size, format, args);

    printf("%s\n", buf);
    session->config->logcallback(session, buf, 0);

    va_end(args);
}

static void ssh_log_error(ssh_session* session, const char *format, ...) {
    int errsave = errno;
    const int buf_size = 2000;
    char buf[buf_size];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, buf_size, format, args);

    if (errsave) {
        sprintf(session->lasterror, "%s. %s. (Error #%d)", strerror(errsave), buf, errsave);
    } else {
        sprintf(session->lasterror, "%s", buf);
    }

    fprintf(stderr, "%s\n", session->lasterror);
    session->config->logcallback(session, session->lasterror, 1);

    va_end(args);
}

/*
 * Returns socket if succeed, otherwise -1 on error
 */
static socket_type socket_connect(ssh_session* session, char *ip, int port) {
    socket_type sock;
    struct sockaddr_in sin;

    /* Connect to SSH server */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == socket_invalid) {
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
 * Returns socket (binded and in listen state) if succeed, otherwise -1 on error
 */
socket_type socket_listen(ssh_session *rsession, char *ip, int *port) {
    socket_type listensock;
    struct sockaddr_in sin;
    int sockopt;
    socklen_t sinlen;

    listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listensock == socket_invalid) {
        ssh_log_error(rsession, "Failed to open socket");
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(0); // Bind to any available port (htons is not needed, but still it's here)
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(ip))) {
        ssh_log_error(rsession, "inet_addr");
        return -1;
    }

    sockopt = 1;
    setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    sinlen = sizeof(sin);
    if (-1 == bind(listensock, (struct sockaddr *)&sin, sinlen)) {
        ssh_log_error(rsession, "Cannot bind to port %d", port);
        return -1;
    }

    if (-1 == listen(listensock, 2)) {
        ssh_log_error(rsession, "Failed to listen opened socket");
        return -1;
    }

    if (getsockname(listensock, (struct sockaddr *)&sin, &sinlen) == -1) {
        ssh_log_error(rsession, "Failed to get socket address");
        return -1;
    }

    *port = ntohs(sin.sin_port);
    return listensock;

}

/*
 * Returns 0 if error.
 */
LIBSSH2_SESSION *ssh_connect(ssh_session *rsession, socket_type sock, enum ssh_auth_type type, char *username, char *password,
                             char *publickeypath, char *privatekeypath, char *passphrase) {
    int rc, i, auth = AUTH_NONE;
    LIBSSH2_SESSION *session;
    const char *fingerprint;
    char *userauthlist;

    ssh_log_msg(rsession, "ssh_connect: username: %s", username);
    ssh_log_msg(rsession, "ssh_connect: password: %s", password);
    ssh_log_msg(rsession, "ssh_connect: privatekeyfile: %s", privatekeypath);
    ssh_log_msg(rsession, "ssh_connect: publickeyfile: %s", publickeypath);

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
    ssh_log_msg(rsession, "Authentication methods: %s", userauthlist);
    if (strstr(userauthlist, "password"))
        auth |= AUTH_PASSWORD;
    if (strstr(userauthlist, "publickey"))
        auth |= AUTH_PUBLICKEY;

    // If authentication by password is available
    // and was chosen by the user, then use it
    if (auth & AUTH_PASSWORD && type == AUTH_PASSWORD) {
        if (libssh2_userauth_password(session, username, password)) {
            ssh_log_error(rsession, "Authentication by password failed");
            return 0;
        }
        ssh_log_msg(rsession, "Authentication by password succeeded.");
        // If authentication by key is available
        // and was chosen by the user, then use it
    } else if (auth & AUTH_PUBLICKEY && type == AUTH_PUBLICKEY) {
        rc = libssh2_userauth_publickey_fromfile(session, username, publickeypath, privatekeypath, passphrase);
        if (rc) {
            ssh_log_error(rsession, "Authentication by key (%s) failed (Error %d)", privatekeypath, rc);
            return 0;
        }
        ssh_log_msg(rsession, "Authentication by key succeeded.");
    } else {
        ssh_log_error(rsession, "No supported authentication methods found");
        return 0;
    }

    return session;
}


ssh_channel *ssh_channel_create(ssh_session* session, socket_type socket, LIBSSH2_CHANNEL *lchannel) {
    const int bufsize = 16384;
    ssh_channel **channels;

    ssh_channel *channel = malloc(sizeof(ssh_channel));
    channel->session = session;
    channel->channel = lchannel;
    channel->socket = socket;
    channel->inbuf = malloc(sizeof(char) * bufsize);
    channel->outbuf = malloc(sizeof(char) * bufsize);
    channel->bufsize = bufsize;

    channels = (ssh_channel**) realloc (session->channels, (session->channelssize + 1) * sizeof(ssh_channel*)); // acts like malloc when (session->channels == NULL)
    if (!channels) {
        ssh_log_error(session, "Not enough memory (call to realloc)");
        return NULL;
    }

    channels[session->channelssize] = channel;

    session->channels = channels;
    ++session->channelssize;
    return channel;
}

void ssh_channel_close(ssh_channel* channel) {
    int i;
    ssh_session* session;
    ssh_channel** channels;

    session = channel->session;

    for (i = 0; i < session->channelssize; i++) {
        if (session->channels[i] != channel) {
            continue;
        }

        if (session->channelssize == 1) {
            channels = NULL;
        } else {
            channels = (ssh_channel**) malloc((session->channelssize - 1) * sizeof(ssh_channel*));
            memcpy(channels, session->channels, i * sizeof(ssh_channel*));

            if (i + 1 < session->channelssize) {
                memcpy(channels + i, session->channels + i + 1, (session->channelssize - i - 1) * sizeof(ssh_channel*));
            }
        }

        free(session->channels);
        session->channels = channels;
        --session->channelssize;

        libssh2_channel_free(channel->channel);
        free(channel->inbuf);
        free(channel->outbuf);

        // free socket and channel
        free(channel);
        ssh_log_msg(session, "done channel_close");
        break;
    }

}

ssh_channel* ssh_channel_find_by_socket(ssh_session* session, socket_type socket) {
    for (int i = 0; i < session->channelssize; i++) {
        if (session->channels[i]->socket == socket) {
            return session->channels[i];
        }
    }

    return NULL;
}

int ssh_open_tunnel(ssh_session* connection) {
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
        ssh_log_msg(connection, "* Okay, we are ready to select.");
        if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
            ssh_log_error(connection, "Error on select()");
            break;
        }
        ssh_log_msg(connection, "* Selected!");

        // Run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {

            // Skip connections that are not ready
            if (!FD_ISSET(i, &readset))
                continue;

            if (i == local_socket) {
                ssh_log_msg(connection, "Data on accept socket is available");

                struct sockaddr_in remoteaddr;
                socklen_t slen = sizeof(remoteaddr);

                // handle new connections
                if ((newfd = accept(local_socket, (struct sockaddr *) &remoteaddr, &slen)) == -1) {
                    ssh_log_error(connection, "Error on accept()");
                } else {
                    FD_SET(newfd, &masterset); // add to master set

                    if (newfd > fdmax) {       // keep track of the maximum
                        fdmax = newfd;
                    }

                    ssh_log_msg(connection, "New connection from %s on socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
                }

                LIBSSH2_CHANNEL *channel;
                int maxattempts = 25;
                int attempts = 0;
                while (attempts < maxattempts) {
                    ++attempts;
                    channel = libssh2_channel_direct_tcpip_ex(session, config->remotehost, config->remoteport,
                                                              config->localip, config->localport);
                    if (!channel) {
                        ssh_log_error(connection, "Could not open the direct TCP/IP channel (channel2)");
                        usleep(200 * 1000);
                        continue;
                    }

                    ssh_log_msg(connection, "Channel successfully created!");
                    break;
                }

                (void) ssh_channel_create(connection, newfd, channel);
                continue;
            }

            if (i == ssh_socket) {
                ssh_log_msg(connection, "Data on SSH socket is available");

                ssh_log_msg(connection, "[%d]  <-  Number of channels", connection->channelssize);

                int s = 0;
                while (s < connection->channelssize) {
                    ssh_channel *context = connection->channels[s];
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

                            ssh_log_error(connection, "libssh2_channel_read: %d", (int)len);
                            break;
                        }

                        ssh_log_msg(connection, "Received %d bytes from tunnel", len);

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
                            ssh_log_msg(connection, "The server at %s:%d disconnected!\n",
                                    config->remotehost, config->remoteport);
                            break;
                        }
                    }
                }


                continue;
            }

            ssh_log_msg(connection, "Data on client socket is available");

            ssh_channel *context = ssh_channel_find_by_socket(connection, i);


            // Read data from a client
            int nbytes = recv(context->socket, context->inbuf, context->bufsize, 0);
            if (nbytes <= 0) {
                if (nbytes == 0) {
                    // Normal situation
                    ssh_log_msg(connection, "Client disconnected");
                } else {
                    // Got error
                    ssh_log_error(connection, "Error when recv()");
                }

                // In both these cases, close and cleanup connection
                close(context->socket); // bye!
                FD_CLR(context->socket, &masterset); // remove from master set
                ssh_channel_close(context);

                continue;
            }

            ssh_log_msg(connection, "Received %d bytes from client", nbytes);


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
                    break;

                }
                wr += rc;
            }
            ssh_log_msg(connection, "Written %d bytes to tunnel", wr);
        }
    }

    // -------------------------------------

    return 0;
}

void ssh_session_close(ssh_session *session) {
    // TODO: perform session cleanup
    free(session);
}

// Returns 0 on error, valid ssh_session* if no errors
ssh_session* ssh_session_create(struct ssh_tunnel_config* config) {
    ssh_session* session = (ssh_session*) malloc(sizeof(ssh_session));
    if (!session) {
        return NULL;
    }
    session->config = config;
    session->channelssize = 0;
    session->channels = NULL;

    ssh_log_msg(session, "ssh_open_tunnel: username: %s", config->username);
    ssh_log_msg(session, "ssh_open_tunnel: privatekeyfile: %s", config->privatekeyfile);
    ssh_log_msg(session, "Connecting to %s...", config->sshserverip);

    socket_type ssh_socket = socket_connect(session, config->sshserverip, config->sshserverport);
    if (ssh_socket == -1) {
        return NULL; // errors are already logged by socket_connect
    }

    LIBSSH2_SESSION *lsession = ssh_connect(session, ssh_socket, config->authtype, config->username, config->password,
                                           config->publickeyfile, config->privatekeyfile, config->passphrase);
    if (lsession == 0) {
        return NULL; // errors are already logged by ssh_connect
    }

    socket_type local_socket = socket_listen(session, config->localip, (int *) &config->localport);
    if (local_socket == -1) {
        return NULL; // errors are already logged by socket_listen
    }

    ssh_log_msg(session, "Waiting for TCP connection on %s:%d...", config->localip, config->localport);
    // -------------------------------------

    // Must use non-blocking IO hereafter due to the current libssh3 API
    libssh2_session_set_blocking(lsession, 0);

    // ------------------------------------

    session->sshsession = lsession;
    session->localsocket = local_socket;
    session->sshsocket = ssh_socket;
    return session;
}


