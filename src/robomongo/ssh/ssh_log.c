#include "robomongo/ssh/internal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

int log_error(const char *format, ...)
{
    int errsave = errno;
    const int buf_size = 1024;
    char buf[buf_size];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, buf_size, format, args);

    if (errsave) {
        fprintf(stderr, "Error (%d): %s. %s\n", errno, strerror(errsave), buf);
    } else {
        fprintf(stderr, "Error: %s\n", buf);
    }

    va_end(args);
    return 1;
}

int log_msg(const char *format, ...)
{
    const int buf_size = 1024;
    char buf[buf_size];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, buf_size, format, args);

    printf("%s\n", buf);

    va_end(args);
    return 1;
}

/*
 * errsave: use 0, if you are not logging errors (i.e. errno == 0).
 */
void ssh_log_v(rbm_ssh_session* session, enum rbm_ssh_log_type type, const char *format, va_list args, int errsave) {
    const size_t bufsize = 2000;
    char buf[bufsize];
    vsnprintf(buf, bufsize, format, args);

    if (type == RBM_SSH_LOG_TYPE_ERROR ||
        type == RBM_SSH_LOG_TYPE_WARN) {
        if (errsave) {
            sprintf(session->lasterror, "%s. %s. (Error #%d)", strerror(errsave), buf, errsave);
        } else {
            sprintf(session->lasterror, "%s", buf);
        }

        fprintf(stderr, "%s\n", session->lasterror);
        session->config->logcallback(session, session->lasterror, type);
        return;
    }

    if (type != RBM_SSH_LOG_TYPE_INFO &&
        type != RBM_SSH_LOG_TYPE_DEBUG)
        return;

    if (type > session->config->loglevel)
        return;

    printf("%s\n", buf);
    session->config->logcallback(session, buf, type);
}

void ssh_log_msg(rbm_ssh_session* session, const char *format, ...) {
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
void ssh_log_warn(rbm_ssh_session* session, const char *format, ...) {
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

void ssh_log_debug(rbm_ssh_session* session, const char *format, ...) {
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

void ssh_log_error(rbm_ssh_session* session, const char *format, ...) {
    int errsave = errno;
    va_list args;
    va_start(args, format);
    ssh_log_v(session, RBM_SSH_LOG_TYPE_ERROR, format, args, errsave);
    va_end(args);
}
