static int has_usable_cert(SSL *s, const SIGALG_LOOKUP *sig, int idx)
{
    const SIGALG_LOOKUP *lu;
    int mdnid, pknid, supported;
    size_t i;

    /* TLS 1.2 callers can override lu->sig_idx, but not TLS 1.3 callers. */
    if (idx == -1)
        idx = sig->sig_idx;
    if (!ssl_has_cert(s, idx))
        return 0;
    if (s->s3.tmp.peer_cert_sigalgs != NULL) {
        for (i = 0; i < s->s3.tmp.peer_cert_sigalgslen; i++) {
            lu = tls1_lookup_sigalg(s->s3.tmp.peer_cert_sigalgs[i]);
            if (lu == NULL
                || !X509_get_signature_info(s->cert->pkeys[idx].x509, &mdnid,
                                            &pknid, NULL, NULL)
                /*
                 * TODO this does not differentiate between the
                 * rsa_pss_pss_* and rsa_pss_rsae_* schemes since we do not
                 * have a chain here that lets us look at the key OID in the
                 * signing certificate.
                 */
                || mdnid != lu->hash
                || pknid != lu->sig)
                continue;

            ERR_set_mark();
            supported = EVP_PKEY_supports_digest_nid(s->cert->pkeys[idx].privatekey,
                                                     mdnid);
            if (supported == 0)
                continue;
            else if (supported < 0)
            {
                /* If it didn't report a mandatory NID, for whatever reasons,
                 * just clear the error and allow all hashes to be used. */
                ERR_pop_to_mark();
            }
            return 1;
        }
        return 0;
    }
    supported = EVP_PKEY_supports_digest_nid(s->cert->pkeys[idx].privatekey,
                                             sig->hash);
    if (supported == 0)
        return 0;
    else if (supported < 0)
        ERR_clear_error();

    return 1;
}