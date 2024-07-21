hostkey_method_ssh_dss_sig_verify(LIBSSH2_SESSION * session,
                                  const unsigned char *sig,
                                  size_t sig_len,
                                  const unsigned char *m,
                                  size_t m_len, void **abstract)
{
    libssh2_dsa_ctx *dsactx = (libssh2_dsa_ctx *) (*abstract);

    /* Skip past keyname_len(4) + keyname(7){"ssh-dss"} + signature_len(4) */
    sig += 15;
    sig_len -= 15;
    if(sig_len != 40) {
        return _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                              "Invalid DSS signature length");
    }
    return _libssh2_dsa_sha1_verify(dsactx, sig, m, m_len);
}