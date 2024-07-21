hostkey_method_ssh_ed25519_init(LIBSSH2_SESSION * session,
                                const unsigned char *hostkey_data,
                                size_t hostkey_data_len,
                                void **abstract)
{
    const unsigned char *s;
    unsigned long len, key_len;
    libssh2_ed25519_ctx *ctx = NULL;

    if(*abstract) {
        hostkey_method_ssh_ed25519_dtor(session, abstract);
        *abstract = NULL;
    }

    if(hostkey_data_len < 15) {
        return -1;
    }

    s = hostkey_data;
    len = _libssh2_ntohu32(s);
    s += 4;

    if(len != 11 || strncmp((char *) s, "ssh-ed25519", 11) != 0) {
        return -1;
    }

    s += 11;

    //public key
    key_len = _libssh2_ntohu32(s);
    s += 4;

    if(_libssh2_ed25519_new_public(&ctx, session, s, key_len) != 0) {
        return -1;
    }

    *abstract = ctx;

    return 0;
}