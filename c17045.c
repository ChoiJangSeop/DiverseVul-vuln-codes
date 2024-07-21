int tls1_check_chain(SSL *s, X509 *x, EVP_PKEY *pk, STACK_OF(X509) *chain,
                     int idx)
{
    int i;
    int rv = 0;
    int check_flags = 0, strict_mode;
    CERT_PKEY *cpk = NULL;
    CERT *c = s->cert;
    uint32_t *pvalid;
    unsigned int suiteb_flags = tls1_suiteb(s);
    /* idx == -1 means checking server chains */
    if (idx != -1) {
        /* idx == -2 means checking client certificate chains */
        if (idx == -2) {
            cpk = c->key;
            idx = (int)(cpk - c->pkeys);
        } else
            cpk = c->pkeys + idx;
        pvalid = s->s3.tmp.valid_flags + idx;
        x = cpk->x509;
        pk = cpk->privatekey;
        chain = cpk->chain;
        strict_mode = c->cert_flags & SSL_CERT_FLAGS_CHECK_TLS_STRICT;
        /* If no cert or key, forget it */
        if (!x || !pk)
            goto end;
    } else {
        size_t certidx;

        if (!x || !pk)
            return 0;

        if (ssl_cert_lookup_by_pkey(pk, &certidx) == NULL)
            return 0;
        idx = certidx;
        pvalid = s->s3.tmp.valid_flags + idx;

        if (c->cert_flags & SSL_CERT_FLAGS_CHECK_TLS_STRICT)
            check_flags = CERT_PKEY_STRICT_FLAGS;
        else
            check_flags = CERT_PKEY_VALID_FLAGS;
        strict_mode = 1;
    }

    if (suiteb_flags) {
        int ok;
        if (check_flags)
            check_flags |= CERT_PKEY_SUITEB;
        ok = X509_chain_check_suiteb(NULL, x, chain, suiteb_flags);
        if (ok == X509_V_OK)
            rv |= CERT_PKEY_SUITEB;
        else if (!check_flags)
            goto end;
    }

    /*
     * Check all signature algorithms are consistent with signature
     * algorithms extension if TLS 1.2 or later and strict mode.
     */
    if (TLS1_get_version(s) >= TLS1_2_VERSION && strict_mode) {
        int default_nid;
        int rsign = 0;
        if (s->s3.tmp.peer_cert_sigalgs != NULL
                || s->s3.tmp.peer_sigalgs != NULL) {
            default_nid = 0;
        /* If no sigalgs extension use defaults from RFC5246 */
        } else {
            switch (idx) {
            case SSL_PKEY_RSA:
                rsign = EVP_PKEY_RSA;
                default_nid = NID_sha1WithRSAEncryption;
                break;

            case SSL_PKEY_DSA_SIGN:
                rsign = EVP_PKEY_DSA;
                default_nid = NID_dsaWithSHA1;
                break;

            case SSL_PKEY_ECC:
                rsign = EVP_PKEY_EC;
                default_nid = NID_ecdsa_with_SHA1;
                break;

            case SSL_PKEY_GOST01:
                rsign = NID_id_GostR3410_2001;
                default_nid = NID_id_GostR3411_94_with_GostR3410_2001;
                break;

            case SSL_PKEY_GOST12_256:
                rsign = NID_id_GostR3410_2012_256;
                default_nid = NID_id_tc26_signwithdigest_gost3410_2012_256;
                break;

            case SSL_PKEY_GOST12_512:
                rsign = NID_id_GostR3410_2012_512;
                default_nid = NID_id_tc26_signwithdigest_gost3410_2012_512;
                break;

            default:
                default_nid = -1;
                break;
            }
        }
        /*
         * If peer sent no signature algorithms extension and we have set
         * preferred signature algorithms check we support sha1.
         */
        if (default_nid > 0 && c->conf_sigalgs) {
            size_t j;
            const uint16_t *p = c->conf_sigalgs;
            for (j = 0; j < c->conf_sigalgslen; j++, p++) {
                const SIGALG_LOOKUP *lu = tls1_lookup_sigalg(*p);

                if (lu != NULL && lu->hash == NID_sha1 && lu->sig == rsign)
                    break;
            }
            if (j == c->conf_sigalgslen) {
                if (check_flags)
                    goto skip_sigs;
                else
                    goto end;
            }
        }
        /* Check signature algorithm of each cert in chain */
        if (!tls1_check_sig_alg(s, x, default_nid)) {
            if (!check_flags)
                goto end;
        } else
            rv |= CERT_PKEY_EE_SIGNATURE;
        rv |= CERT_PKEY_CA_SIGNATURE;
        for (i = 0; i < sk_X509_num(chain); i++) {
            if (!tls1_check_sig_alg(s, sk_X509_value(chain, i), default_nid)) {
                if (check_flags) {
                    rv &= ~CERT_PKEY_CA_SIGNATURE;
                    break;
                } else
                    goto end;
            }
        }
    }
    /* Else not TLS 1.2, so mark EE and CA signing algorithms OK */
    else if (check_flags)
        rv |= CERT_PKEY_EE_SIGNATURE | CERT_PKEY_CA_SIGNATURE;
 skip_sigs:
    /* Check cert parameters are consistent */
    if (tls1_check_cert_param(s, x, 1))
        rv |= CERT_PKEY_EE_PARAM;
    else if (!check_flags)
        goto end;
    if (!s->server)
        rv |= CERT_PKEY_CA_PARAM;
    /* In strict mode check rest of chain too */
    else if (strict_mode) {
        rv |= CERT_PKEY_CA_PARAM;
        for (i = 0; i < sk_X509_num(chain); i++) {
            X509 *ca = sk_X509_value(chain, i);
            if (!tls1_check_cert_param(s, ca, 0)) {
                if (check_flags) {
                    rv &= ~CERT_PKEY_CA_PARAM;
                    break;
                } else
                    goto end;
            }
        }
    }
    if (!s->server && strict_mode) {
        STACK_OF(X509_NAME) *ca_dn;
        int check_type = 0;
        switch (EVP_PKEY_id(pk)) {
        case EVP_PKEY_RSA:
            check_type = TLS_CT_RSA_SIGN;
            break;
        case EVP_PKEY_DSA:
            check_type = TLS_CT_DSS_SIGN;
            break;
        case EVP_PKEY_EC:
            check_type = TLS_CT_ECDSA_SIGN;
            break;
        }
        if (check_type) {
            const uint8_t *ctypes = s->s3.tmp.ctype;
            size_t j;

            for (j = 0; j < s->s3.tmp.ctype_len; j++, ctypes++) {
                if (*ctypes == check_type) {
                    rv |= CERT_PKEY_CERT_TYPE;
                    break;
                }
            }
            if (!(rv & CERT_PKEY_CERT_TYPE) && !check_flags)
                goto end;
        } else {
            rv |= CERT_PKEY_CERT_TYPE;
        }

        ca_dn = s->s3.tmp.peer_ca_names;

        if (!sk_X509_NAME_num(ca_dn))
            rv |= CERT_PKEY_ISSUER_NAME;

        if (!(rv & CERT_PKEY_ISSUER_NAME)) {
            if (ssl_check_ca_name(ca_dn, x))
                rv |= CERT_PKEY_ISSUER_NAME;
        }
        if (!(rv & CERT_PKEY_ISSUER_NAME)) {
            for (i = 0; i < sk_X509_num(chain); i++) {
                X509 *xtmp = sk_X509_value(chain, i);
                if (ssl_check_ca_name(ca_dn, xtmp)) {
                    rv |= CERT_PKEY_ISSUER_NAME;
                    break;
                }
            }
        }
        if (!check_flags && !(rv & CERT_PKEY_ISSUER_NAME))
            goto end;
    } else
        rv |= CERT_PKEY_ISSUER_NAME | CERT_PKEY_CERT_TYPE;

    if (!check_flags || (rv & check_flags) == check_flags)
        rv |= CERT_PKEY_VALID;

 end:

    if (TLS1_get_version(s) >= TLS1_2_VERSION)
        rv |= *pvalid & (CERT_PKEY_EXPLICIT_SIGN | CERT_PKEY_SIGN);
    else
        rv |= CERT_PKEY_SIGN | CERT_PKEY_EXPLICIT_SIGN;

    /*
     * When checking a CERT_PKEY structure all flags are irrelevant if the
     * chain is invalid.
     */
    if (!check_flags) {
        if (rv & CERT_PKEY_VALID) {
            *pvalid = rv;
        } else {
            /* Preserve sign and explicit sign flag, clear rest */
            *pvalid &= CERT_PKEY_EXPLICIT_SIGN | CERT_PKEY_SIGN;
            return 0;
        }
    }
    return rv;
}