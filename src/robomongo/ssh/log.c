#include "robomongo/ssh/private.h"

#include <errno.h>
#include <stdio.h>

enum {BUF_SIZE = 1024 };

int log_error(const char *format, ...)
{
    int errsave = errno;
    char buf[BUF_SIZE];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, BUF_SIZE, format, args);

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
    char buf[BUF_SIZE];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, BUF_SIZE, format, args);

    printf("%s\n", buf);

    va_end(args);
    return 1;
}

/*
 * errsave: use 0, if you are not logging errors (i.e. errno == 0).
 */
void ssh_log_v(struct rbm_session* session, enum rbm_ssh_log_type type, const char *format, va_list args, int errsave) {
    char buf[BUF_SIZE];
    vsnprintf(buf, BUF_SIZE, format, args);

    if (type == RBM_SSH_LOG_TYPE_ERROR ||
        type == RBM_SSH_LOG_TYPE_WARN) {
        if (errsave) {
            sprintf(session->lasterror, "%s. %s. (Error #%d)", strerror(errsave), buf, errsave);
        } else {
            sprintf(session->lasterror, "%s", buf);
        }

        fprintf(stderr, "%s\n", session->lasterror);
        if (session->config->logcontext)
            session->config->logcallback(session->config->logcontext, session->lasterror, type);
        return;
    }

    if (type != RBM_SSH_LOG_TYPE_INFO &&
        type != RBM_SSH_LOG_TYPE_DEBUG)
        return;

    if (type > session->config->loglevel)
        return;

    printf("%s\n", buf);
    if (session->config->logcontext)
        session->config->logcallback(session->config->logcontext, buf, type);
}

void ssh_log_msg(struct rbm_session* session, const char *format, ...) {
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
void ssh_log_warn(struct rbm_session* session, const char *format, ...) {
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

void ssh_log_debug(struct rbm_session* session, const char *format, ...) {
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

void ssh_log_error(struct rbm_session* session, const char *format, ...) {
    int errsave = errno;
    va_list args;
    va_start(args, format);
    ssh_log_v(session, RBM_SSH_LOG_TYPE_ERROR, format, args, errsave);
    va_end(args);
}
