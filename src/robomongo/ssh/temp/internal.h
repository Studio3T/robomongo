#include "robomongo/ssh/ssh.h"

#include <stdarg.h>

int log_error(const char *format, ...);
int log_msg(const char *format, ...);

void ssh_log_v(rbm_ssh_session* session, enum rbm_ssh_log_type type, const char *format, va_list args, int errsave);
void ssh_log_msg(rbm_ssh_session* session, const char *format, ...);
void ssh_log_warn(rbm_ssh_session* session, const char *format, ...);
void ssh_log_debug(rbm_ssh_session* session, const char *format, ...);
void ssh_log_error(rbm_ssh_session* session, const char *format, ...);
