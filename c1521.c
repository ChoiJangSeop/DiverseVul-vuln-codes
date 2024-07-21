static int x509parse_verifycrl(x509_cert *crt, x509_cert *ca,
        x509_crl *crl_list)
{
    int flags = 0;
    int hash_id;
    unsigned char hash[64];

    if( ca == NULL )
        return( flags );

    /*
     * TODO: What happens if no CRL is present?
     * Suggestion: Revocation state should be unknown if no CRL is present.
     * For backwards compatibility this is not yet implemented.
     */

    while( crl_list != NULL )
    {
        if( crl_list->version == 0 ||
            crl_list->issuer_raw.len != ca->subject_raw.len ||
            memcmp( crl_list->issuer_raw.p, ca->subject_raw.p,
                    crl_list->issuer_raw.len ) != 0 )
        {
            crl_list = crl_list->next;
            continue;
        }

        /*
         * Check if CRL is correctly signed by the trusted CA
         */
        hash_id = crl_list->sig_alg;

        x509_hash( crl_list->tbs.p, crl_list->tbs.len, hash_id, hash );

        if( !rsa_pkcs1_verify( &ca->rsa, RSA_PUBLIC, hash_id,
                              0, hash, crl_list->sig.p ) == 0 )
        {
            /*
             * CRL is not trusted
             */
            flags |= BADCRL_NOT_TRUSTED;
            break;
        }

        /*
         * Check for validity of CRL (Do not drop out)
         */
        if( x509parse_time_expired( &crl_list->next_update ) )
            flags |= BADCRL_EXPIRED;

        /*
         * Check if certificate is revoked
         */
        if( x509parse_revoked(crt, crl_list) )
        {
            flags |= BADCERT_REVOKED;
            break;
        }

        crl_list = crl_list->next;
    }
    return flags;
}