static int get_buffer(size_t *_buf_len, char **_buf)
{
    long pw_max;
    long gr_max;
    size_t buf_len;
    char *buf;

    pw_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    gr_max = sysconf(_SC_GETGR_R_SIZE_MAX);

    if (pw_max == -1 && gr_max == -1) {
        buf_len = 16384;
    } else {
        buf_len = MAX(pw_max, gr_max);
    }

    buf = malloc(sizeof(char) * buf_len);
    if (buf == NULL) {
        return LDAP_OPERATIONS_ERROR;
    }

    *_buf_len = buf_len;
    *_buf = buf;

    return LDAP_SUCCESS;
}