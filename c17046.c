int tls_choose_sigalg(SSL *s, int fatalerrs)
{
    const SIGALG_LOOKUP *lu = NULL;
    int sig_idx = -1;

    s->s3.tmp.cert = NULL;
    s->s3.tmp.sigalg = NULL;

    if (SSL_IS_TLS13(s)) {
        size_t i;
#ifndef OPENSSL_NO_EC
        int curve = -1;
#endif

        /* Look for a certificate matching shared sigalgs */
        for (i = 0; i < s->shared_sigalgslen; i++) {
            lu = s->shared_sigalgs[i];
            sig_idx = -1;

            /* Skip SHA1, SHA224, DSA and RSA if not PSS */
            if (lu->hash == NID_sha1
                || lu->hash == NID_sha224
                || lu->sig == EVP_PKEY_DSA
                || lu->sig == EVP_PKEY_RSA)
                continue;
            /* Check that we have a cert, and signature_algorithms_cert */
            if (!tls1_lookup_md(lu, NULL) || !has_usable_cert(s, lu, -1))
                continue;
            if (lu->sig == EVP_PKEY_EC) {
#ifndef OPENSSL_NO_EC
                if (curve == -1) {
                    EC_KEY *ec = EVP_PKEY_get0_EC_KEY(s->cert->pkeys[SSL_PKEY_ECC].privatekey);

                    curve = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec));
                }
                if (lu->curve != NID_undef && curve != lu->curve)
                    continue;
#else
                continue;
#endif
            } else if (lu->sig == EVP_PKEY_RSA_PSS) {
                /* validate that key is large enough for the signature algorithm */
                EVP_PKEY *pkey;

                pkey = s->cert->pkeys[lu->sig_idx].privatekey;
                if (!rsa_pss_check_min_key_size(EVP_PKEY_get0(pkey), lu))
                    continue;
            }
            break;
        }
        if (i == s->shared_sigalgslen) {
            if (!fatalerrs)
                return 1;
            SSLfatal(s, SSL_AD_HANDSHAKE_FAILURE, SSL_F_TLS_CHOOSE_SIGALG,
                     SSL_R_NO_SUITABLE_SIGNATURE_ALGORITHM);
            return 0;
        }
    } else {
        /* If ciphersuite doesn't require a cert nothing to do */
        if (!(s->s3.tmp.new_cipher->algorithm_auth & SSL_aCERT))
            return 1;
        if (!s->server && !ssl_has_cert(s, s->cert->key - s->cert->pkeys))
                return 1;

        if (SSL_USE_SIGALGS(s)) {
            size_t i;
            if (s->s3.tmp.peer_sigalgs != NULL) {
#ifndef OPENSSL_NO_EC
                int curve;

                /* For Suite B need to match signature algorithm to curve */
                if (tls1_suiteb(s)) {
                    EC_KEY *ec = EVP_PKEY_get0_EC_KEY(s->cert->pkeys[SSL_PKEY_ECC].privatekey);
                    curve = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec));
                } else {
                    curve = -1;
                }
#endif

                /*
                 * Find highest preference signature algorithm matching
                 * cert type
                 */
                for (i = 0; i < s->shared_sigalgslen; i++) {
                    lu = s->shared_sigalgs[i];

                    if (s->server) {
                        if ((sig_idx = tls12_get_cert_sigalg_idx(s, lu)) == -1)
                            continue;
                    } else {
                        int cc_idx = s->cert->key - s->cert->pkeys;

                        sig_idx = lu->sig_idx;
                        if (cc_idx != sig_idx)
                            continue;
                    }
                    /* Check that we have a cert, and sig_algs_cert */
                    if (!has_usable_cert(s, lu, sig_idx))
                        continue;
                    if (lu->sig == EVP_PKEY_RSA_PSS) {
                        /* validate that key is large enough for the signature algorithm */
                        EVP_PKEY *pkey = s->cert->pkeys[sig_idx].privatekey;

                        if (!rsa_pss_check_min_key_size(EVP_PKEY_get0(pkey), lu))
                            continue;
                    }
#ifndef OPENSSL_NO_EC
                    if (curve == -1 || lu->curve == curve)
#endif
                        break;
                }
                if (i == s->shared_sigalgslen) {
                    if (!fatalerrs)
                        return 1;
                    SSLfatal(s, SSL_AD_HANDSHAKE_FAILURE,
                             SSL_F_TLS_CHOOSE_SIGALG,
                             SSL_R_NO_SUITABLE_SIGNATURE_ALGORITHM);
                    return 0;
                }
            } else {
                /*
                 * If we have no sigalg use defaults
                 */
                const uint16_t *sent_sigs;
                size_t sent_sigslen;

                if ((lu = tls1_get_legacy_sigalg(s, -1)) == NULL) {
                    if (!fatalerrs)
                        return 1;
                    SSLfatal(s, SSL_AD_INTERNAL_ERROR, SSL_F_TLS_CHOOSE_SIGALG,
                             ERR_R_INTERNAL_ERROR);
                    return 0;
                }

                /* Check signature matches a type we sent */
                sent_sigslen = tls12_get_psigalgs(s, 1, &sent_sigs);
                for (i = 0; i < sent_sigslen; i++, sent_sigs++) {
                    if (lu->sigalg == *sent_sigs
                            && has_usable_cert(s, lu, lu->sig_idx))
                        break;
                }
                if (i == sent_sigslen) {
                    if (!fatalerrs)
                        return 1;
                    SSLfatal(s, SSL_AD_ILLEGAL_PARAMETER,
                             SSL_F_TLS_CHOOSE_SIGALG,
                             SSL_R_WRONG_SIGNATURE_TYPE);
                    return 0;
                }
            }
        } else {
            if ((lu = tls1_get_legacy_sigalg(s, -1)) == NULL) {
                if (!fatalerrs)
                    return 1;
                SSLfatal(s, SSL_AD_INTERNAL_ERROR, SSL_F_TLS_CHOOSE_SIGALG,
                         ERR_R_INTERNAL_ERROR);
                return 0;
            }
        }
    }
    if (sig_idx == -1)
        sig_idx = lu->sig_idx;
    s->s3.tmp.cert = &s->cert->pkeys[sig_idx];
    s->cert->key = s->s3.tmp.cert;
    s->s3.tmp.sigalg = lu;
    return 1;
}