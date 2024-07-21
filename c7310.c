hostkey_method_ssh_dss_init(LIBSSH2_SESSION * session,
                            const unsigned char *hostkey_data,
                            size_t hostkey_data_len,
                            void **abstract)
{
    libssh2_dsa_ctx *dsactx;
    const unsigned char *p, *q, *g, *y, *s;
    unsigned long p_len, q_len, g_len, y_len, len;
    int ret;

    (void) hostkey_data_len;

    if(*abstract) {
        hostkey_method_ssh_dss_dtor(session, abstract);
        *abstract = NULL;
    }

    s = hostkey_data;
    len = _libssh2_ntohu32(s);
    s += 4;
    if(len != 7 || strncmp((char *) s, "ssh-dss", 7) != 0) {
        return -1;
    }
    s += 7;

    p_len = _libssh2_ntohu32(s);
    s += 4;
    p = s;
    s += p_len;
    q_len = _libssh2_ntohu32(s);
    s += 4;
    q = s;
    s += q_len;
    g_len = _libssh2_ntohu32(s);
    s += 4;
    g = s;
    s += g_len;
    y_len = _libssh2_ntohu32(s);
    s += 4;
    y = s;
    /* s += y_len; */

    ret = _libssh2_dsa_new(&dsactx, p, p_len, q, q_len,
                           g, g_len, y, y_len, NULL, 0);
    if(ret) {
        return -1;
    }

    *abstract = dsactx;

    return 0;
}