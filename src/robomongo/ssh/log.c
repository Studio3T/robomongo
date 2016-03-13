#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

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
