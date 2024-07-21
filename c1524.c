static int x509parse_verify_child(
                x509_cert *child, x509_cert *parent, x509_cert *trust_ca,
                x509_crl *ca_crl, int path_cnt, int *flags,
                int (*f_vrfy)(void *, x509_cert *, int, int *),
                void *p_vrfy )
{
    int hash_id, ret;
    int parent_flags = 0;
    unsigned char hash[64];
    x509_cert *grandparent;

    if( x509parse_time_expired( &child->valid_to ) )
        *flags |= BADCERT_EXPIRED;

    hash_id = child->sig_alg;

    x509_hash( child->tbs.p, child->tbs.len, hash_id, hash );

    if( rsa_pkcs1_verify( &parent->rsa, RSA_PUBLIC, hash_id, 0, hash,
                           child->sig.p ) != 0 )
        *flags |= BADCERT_NOT_TRUSTED;
        
    /* Check trusted CA's CRL for the given crt */
    *flags |= x509parse_verifycrl(child, parent, ca_crl);

    grandparent = parent->next;

    while( grandparent != NULL )
    {
        if( grandparent->version == 0 ||
            grandparent->ca_istrue == 0 ||
            parent->issuer_raw.len != grandparent->subject_raw.len ||
            memcmp( parent->issuer_raw.p, grandparent->subject_raw.p,
                    parent->issuer_raw.len ) != 0 )
        {
            grandparent = grandparent->next;
            continue;
        }
        break;
    }

    if( grandparent != NULL )
    {
        /*
         * Part of the chain
         */
        ret = x509parse_verify_child( parent, grandparent, trust_ca, ca_crl, path_cnt + 1, &parent_flags, f_vrfy, p_vrfy );
        if( ret != 0 )
            return( ret );
    }
    else
    {
        ret = x509parse_verify_top( parent, trust_ca, ca_crl, path_cnt + 1, &parent_flags, f_vrfy, p_vrfy );
        if( ret != 0 )
            return( ret );
    }

    /* child is verified to be a child of the parent, call verify callback */
    if( NULL != f_vrfy )
        if( ( ret = f_vrfy( p_vrfy, child, path_cnt, flags ) ) != 0 )
            return( ret );

    *flags |= parent_flags;

    return( 0 );
}