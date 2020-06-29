#include "session_fixture.h"

#include <libssh2.h>

#include <stdio.h>
#include <stdlib.h>

static const char *USERNAME = "libssh2"; /* configured in Dockerfile */
static const char *KEY_FILE_ED25519_PRIVATE = "key_ed25519";

int read_file(const char *path, char **buf, size_t *len);

int test(LIBSSH2_SESSION *session)
{
    int rc;
    FILE *fp = NULL;
    char *buffer = NULL;
    size_t len = 0;
    const char *userauth_list = NULL;

    userauth_list = libssh2_userauth_list(session, USERNAME, strlen(USERNAME));
    if(userauth_list == NULL) {
        print_last_session_error("libssh2_userauth_list");
        return 1;
    }

    if(strstr(userauth_list, "publickey") == NULL) {
        fprintf(stderr, "'publickey' was expected in userauth list: %s\n",
                userauth_list);
        return 1;
    }

    if(read_file(KEY_FILE_ED25519_PRIVATE, &buffer, &len)) {
        fprintf(stderr, "Reading key file failed.");
        return 1;
    }

    rc = libssh2_userauth_publickey_frommemory(session, USERNAME, strlen(USERNAME),
                                               NULL, 0, buffer, len, NULL);

    free(buffer);

    if(rc != 0) {
        print_last_session_error("libssh2_userauth_publickey_fromfile_ex");
        return 1;
    }

    return 0;
}

int read_file(const char *path, char **out_buffer, size_t *out_len)
{
    int rc;
    FILE *fp = NULL;
    char *buffer = NULL;
    size_t len = 0;

    if(out_buffer == NULL || out_len == NULL || path == NULL) {
        fprintf(stderr, "invalid params.");
        return 1;
    }

    *out_buffer = NULL;
    *out_len = 0;

    fp = fopen(path, "r");

    if(!fp) {
       fprintf(stderr, "File could not be read.");
       return 1;
    }

    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    rewind(fp);

    buffer = calloc(1, len + 1);
    if(!buffer) {
       fclose(fp);
       fprintf(stderr, "Could not alloc memory.");
       return 1;
    }

    if(1 != fread(buffer, len, 1, fp)) {
       fclose(fp);
       free(buffer);
       fprintf(stderr, "Could not read file into memory.");
       return 1;
    }

    fclose(fp);

    *out_buffer = buffer;
    *out_len = len;

    return 0;
}
