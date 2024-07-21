hostkey_method_ssh_ecdsa_sig_verify(LIBSSH2_SESSION * session,
                                    const unsigned char *sig,
                                    size_t sig_len,
                                    const unsigned char *m,
                                    size_t m_len, void **abstract)
{
    const unsigned char *r, *s, *p;
    size_t r_len, s_len;
    libssh2_ecdsa_ctx *ctx = (libssh2_ecdsa_ctx *) (*abstract);

    (void) session;

    if(sig_len < 35)
        return -1;

    /* Skip past keyname_len(4) + keyname(19){"ecdsa-sha2-nistp256"} + signature_len(4) */
    p = sig;
    p += 27;

    r_len = _libssh2_ntohu32(p);
    p += 4;
    r = p;
    p += r_len;

    s_len = _libssh2_ntohu32(p);
    p += 4;
    s = p;

    return _libssh2_ecdsa_verify(ctx, r, r_len, s, s_len, m, m_len);
}