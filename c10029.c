NOEXPORT int dh_init(SERVICE_OPTIONS *section) {
    DH *dh=NULL;
    int i, n;
    char description[128];
    STACK_OF(SSL_CIPHER) *ciphers;

    section->option.dh_temp_params=0; /* disable by default */

    /* check if DH is actually enabled for this section */
    ciphers=SSL_CTX_get_ciphers(section->ctx);
    if(!ciphers)
        return 1; /* ERROR (unlikely) */
    n=sk_SSL_CIPHER_num(ciphers);
    for(i=0; i<n; ++i) {
        *description='\0';
        SSL_CIPHER_description(sk_SSL_CIPHER_value(ciphers, i),
            description, sizeof description);
        /* s_log(LOG_INFO, "Ciphersuite: %s", description); */
        if(strstr(description, " Kx=DH")) {
            s_log(LOG_INFO, "DH initialization needed for %s",
                SSL_CIPHER_get_name(sk_SSL_CIPHER_value(ciphers, i)));
            break;
        }
    }
    if(i==n) { /* no DH ciphers found */
        s_log(LOG_INFO, "DH initialization not needed");
        return 0; /* OK */
    }

    s_log(LOG_DEBUG, "DH initialization");
#ifndef OPENSSL_NO_ENGINE
    if(!section->engine) /* cert is a file and not an identifier */
#endif
        dh=dh_read(section->cert);
    if(dh) {
        SSL_CTX_set_tmp_dh(section->ctx, dh);
        s_log(LOG_INFO, "%d-bit DH parameters loaded", 8*DH_size(dh));
        DH_free(dh);
        return 0; /* OK */
    }
    CRYPTO_THREAD_read_lock(stunnel_locks[LOCK_DH]);
    SSL_CTX_set_tmp_dh(section->ctx, dh_params);
    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_DH]);
    dh_temp_params=1; /* generate temporary DH parameters in cron */
    section->option.dh_temp_params=1; /* update this section in cron */
    s_log(LOG_INFO, "Using dynamic DH parameters");
    return 0; /* OK */
}