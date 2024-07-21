hostkey_method_ssh_ecdsa_init(LIBSSH2_SESSION * session,
                          const unsigned char *hostkey_data,
                          size_t hostkey_data_len,
                          void **abstract)
{
    libssh2_ecdsa_ctx *ecdsactx = NULL;
    const unsigned char *s, *k;
    size_t len, key_len, n_len;
    libssh2_curve_type type;

    if(abstract != NULL && *abstract) {
        hostkey_method_ssh_ecdsa_dtor(session, abstract);
        *abstract = NULL;
    }

    if(hostkey_data_len < 23)
        return -1;

    s = hostkey_data;
    len = _libssh2_ntohu32(s);
    s += 4;

    if(len != 19)
        return -1;

    if(strncmp((char *) s, "ecdsa-sha2-nistp256", 19) == 0) {
        type = LIBSSH2_EC_CURVE_NISTP256;
    }
    else if(strncmp((char *) s, "ecdsa-sha2-nistp384", 19) == 0) {
        type = LIBSSH2_EC_CURVE_NISTP384;
    }
    else if(strncmp((char *) s, "ecdsa-sha2-nistp521", 19) == 0) {
        type = LIBSSH2_EC_CURVE_NISTP521;
    }
    else {
        return -1;
    }
    s += 19;

    /* Domain length */
    n_len = _libssh2_ntohu32(s);
    s += 4;

    if(n_len != 8)
        return -1;

    if(type == LIBSSH2_EC_CURVE_NISTP256 && strncmp((char *)s, "nistp256", 8) != 0) {
        return -1;
    }
    else if(type == LIBSSH2_EC_CURVE_NISTP384 && strncmp((char *)s, "nistp384", 8) != 0) {
        return -1;
    }
    else if(type == LIBSSH2_EC_CURVE_NISTP521 && strncmp((char *)s, "nistp521", 8) != 0) {
        return -1;
    }

    s += 8;

    /* public key */
    key_len = _libssh2_ntohu32(s);
    s += 4;

    k = s;

    if(_libssh2_ecdsa_curve_name_with_octal_new(&ecdsactx, k, key_len, type) )
        return -1;

    if(abstract != NULL)
        *abstract = ecdsactx;

    return 0;
}