doapr_outch(char **sbuffer,
            char **buffer, size_t *currlen, size_t *maxlen, int c)
{
    /* If we haven't at least one buffer, someone has doe a big booboo */
    assert(*sbuffer != NULL || buffer != NULL);

    /* |currlen| must always be <= |*maxlen| */
    assert(*currlen <= *maxlen);

    if (buffer && *currlen == *maxlen) {
        *maxlen += 1024;
        if (*buffer == NULL) {
            *buffer = OPENSSL_malloc(*maxlen);
            if (!*buffer) {
                /* Panic! Can't really do anything sensible. Just return */
                return;
            }
            if (*currlen > 0) {
                assert(*sbuffer != NULL);
                memcpy(*buffer, *sbuffer, *currlen);
            }
            *sbuffer = NULL;
        } else {
            *buffer = OPENSSL_realloc(*buffer, *maxlen);
            if (!*buffer) {
                /* Panic! Can't really do anything sensible. Just return */
                return;
            }
        }
    }

    if (*currlen < *maxlen) {
        if (*sbuffer)
            (*sbuffer)[(*currlen)++] = (char)c;
        else
            (*buffer)[(*currlen)++] = (char)c;
    }

    return;
}