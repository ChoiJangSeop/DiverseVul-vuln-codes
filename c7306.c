hostkey_method_ssh_rsa_init(LIBSSH2_SESSION * session,
                            const unsigned char *hostkey_data,
                            size_t hostkey_data_len,
                            void **abstract)
{
    libssh2_rsa_ctx *rsactx;
    const unsigned char *s, *e, *n;
    unsigned long len, e_len, n_len;
    int ret;

    (void) hostkey_data_len;

    if(*abstract) {
        hostkey_method_ssh_rsa_dtor(session, abstract);
        *abstract = NULL;
    }

    s = hostkey_data;
    len = _libssh2_ntohu32(s);
    s += 4;

    if(len != 7 || strncmp((char *) s, "ssh-rsa", 7) != 0) {
        return -1;
    }
    s += 7;

    e_len = _libssh2_ntohu32(s);
    s += 4;

    e = s;
    s += e_len;
    n_len = _libssh2_ntohu32(s);
    s += 4;
    n = s;

    ret = _libssh2_rsa_new(&rsactx, e, e_len, n, n_len, NULL, 0,
                           NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    if(ret) {
        return -1;
    }

    *abstract = rsactx;

    return 0;
}