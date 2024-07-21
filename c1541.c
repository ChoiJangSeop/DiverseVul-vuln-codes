static int x509parse_verify_top(
                x509_cert *child, x509_cert *trust_ca,
                x509_crl *ca_crl, int path_cnt, int *flags,
                int (*f_vrfy)(void *, x509_cert *, int, int *),
                void *p_vrfy )
{
    int hash_id, ret;
    int ca_flags = 0, check_path_cnt = path_cnt + 1;
    unsigned char hash[64];

    if( x509parse_time_expired( &child->valid_to ) )
        *flags |= BADCERT_EXPIRED;

    /*
     * Child is the top of the chain. Check against the trust_ca list.
     */
    *flags |= BADCERT_NOT_TRUSTED;

    while( trust_ca != NULL )
    {
        if( trust_ca->version == 0 ||
            child->issuer_raw.len != trust_ca->subject_raw.len ||
            memcmp( child->issuer_raw.p, trust_ca->subject_raw.p,
                    child->issuer_raw.len ) != 0 )
        {
            trust_ca = trust_ca->next;
            continue;
        }

        /*
         * Reduce path_len to check against if top of the chain is
         * the same as the trusted CA
         */
        if( child->subject_raw.len == trust_ca->subject_raw.len &&
            memcmp( child->subject_raw.p, trust_ca->subject_raw.p,
                            child->issuer_raw.len ) == 0 ) 
        {
            check_path_cnt--;
        }

        if( trust_ca->max_pathlen > 0 &&
            trust_ca->max_pathlen < check_path_cnt )
        {
            trust_ca = trust_ca->next;
            continue;
        }

        hash_id = child->sig_alg;

        x509_hash( child->tbs.p, child->tbs.len, hash_id, hash );

        if( rsa_pkcs1_verify( &trust_ca->rsa, RSA_PUBLIC, hash_id,
                    0, hash, child->sig.p ) != 0 )
        {
            trust_ca = trust_ca->next;
            continue;
        }

        /*
         * Top of chain is signed by a trusted CA
         */
        *flags &= ~BADCERT_NOT_TRUSTED;
        break;
    }

    /*
     * If top of chain is not the same as the trusted CA send a verify request
     * to the callback for any issues with validity and CRL presence for the
     * trusted CA certificate.
     */
    if( trust_ca != NULL &&
        ( child->subject_raw.len != trust_ca->subject_raw.len ||
          memcmp( child->subject_raw.p, trust_ca->subject_raw.p,
                            child->issuer_raw.len ) != 0 ) )
    {
        /* Check trusted CA's CRL for then chain's top crt */
        *flags |= x509parse_verifycrl( child, trust_ca, ca_crl );

        if( x509parse_time_expired( &trust_ca->valid_to ) )
            ca_flags |= BADCERT_EXPIRED;

        if( NULL != f_vrfy )
        {
            if( ( ret = f_vrfy( p_vrfy, trust_ca, path_cnt + 1, &ca_flags ) ) != 0 )
                return( ret );
        }
    }

    /* Call callback on top cert */
    if( NULL != f_vrfy )
    {
        if( ( ret = f_vrfy(p_vrfy, child, path_cnt, flags ) ) != 0 )
            return( ret );
    }

    *flags |= ca_flags;

    return( 0 );
}