/* Copyright (C) 2016 Alexander Lamaison
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the
 *   following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names
 *   of any other contributors may be used to endorse or
 *   promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include "session_fixture.h"
#include "libssh2_config.h"
#include "openssh_fixture.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

LIBSSH2_SESSION *connected_session = NULL;
int connected_socket = -1;

static int connect_to_server()
{
    int rc;
    connected_socket = open_socket_to_openssh_server();
    if(connected_socket <= 0) {
        return -1;
    }

    rc = libssh2_session_handshake(connected_session, connected_socket);
    if(rc != 0) {
        print_last_session_error("libssh2_session_handshake");
        return -1;
    }

    return 0;
}

void setup_fixture_workdir()
{
    char *wd = getenv("FIXTURE_WORKDIR");
#ifdef FIXTURE_WORKDIR
    if(!wd) {
        wd = FIXTURE_WORKDIR;
    }
#endif
    if(!wd) {
#ifdef WIN32
        char wd_buf[_MAX_PATH];
#else
        char wd_buf[MAXPATHLEN];
#endif
        getcwd(wd_buf, sizeof(wd_buf));
        wd = wd_buf;
    }

    chdir(wd);
}

LIBSSH2_SESSION *start_session_fixture()
{
    int rc;

    setup_fixture_workdir();

    rc = start_openssh_fixture();
    if(rc != 0) {
        return NULL;
    }
    rc = libssh2_init(0);
    if(rc != 0) {
        fprintf(stderr, "libssh2_init failed (%d)\n", rc);
        return NULL;
    }

    connected_session = libssh2_session_init_ex(NULL, NULL, NULL, NULL);
    libssh2_session_set_blocking(connected_session, 1);
    if(connected_session == NULL) {
        fprintf(stderr, "libssh2_session_init_ex failed\n");
        return NULL;
    }

    rc = connect_to_server();
    if(rc != 0) {
        return NULL;
    }

    return connected_session;
}

void print_last_session_error(const char *function)
{
    if(connected_session) {
        char *message;
        int rc =
            libssh2_session_last_error(connected_session, &message, NULL, 0);
        fprintf(stderr, "%s failed (%d): %s\n", function, rc, message);
    }
    else {
        fprintf(stderr, "No session");
    }
}

void stop_session_fixture()
{
    if(connected_session) {
        libssh2_session_disconnect(connected_session, "test ended");
        libssh2_session_free(connected_session);
        shutdown(connected_socket, 2);
        connected_session = NULL;
    }
    else {
        fprintf(stderr, "Cannot stop session - none started");
    }

    stop_openssh_fixture();
}
