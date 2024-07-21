static int ssl_parse_certificate_verify( ssl_context *ssl )
{
    int ret;
    size_t n = 0, n1, n2;
    unsigned char hash[48];
    int hash_id;
    unsigned int hashlen;

    SSL_DEBUG_MSG( 2, ( "=> parse certificate verify" ) );

    if( ssl->session_negotiate->peer_cert == NULL )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip parse certificate verify" ) );
        ssl->state++;
        return( 0 );
    }

    ssl->handshake->calc_verify( ssl, hash );

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    ssl->state++;

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate verify message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY );
    }

    if( ssl->in_msg[0] != SSL_HS_CERTIFICATE_VERIFY )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate verify message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY );
    }

    if( ssl->minor_ver == SSL_MINOR_VERSION_3 )
    {
        /*
         * As server we know we either have SSL_HASH_SHA384 or
         * SSL_HASH_SHA256
         */
        if( ssl->in_msg[4] != ssl->handshake->verify_sig_alg ||
            ssl->in_msg[5] != SSL_SIG_RSA )
        {
            SSL_DEBUG_MSG( 1, ( "peer not adhering to requested sig_alg for verify message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY );
        }

        if( ssl->handshake->verify_sig_alg == SSL_HASH_SHA384 )
        {
            hashlen = 48;
            hash_id = SIG_RSA_SHA384;
        }
        else
        {
            hashlen = 32;
            hash_id = SIG_RSA_SHA256;
        }

        n += 2;
    }
    else
    {
        hashlen = 36;
        hash_id = SIG_RSA_RAW;
    }

    n1 = ssl->session_negotiate->peer_cert->rsa.len;
    n2 = ( ssl->in_msg[4 + n] << 8 ) | ssl->in_msg[5 + n];

    if( n + n1 + 6 != ssl->in_hslen || n1 != n2 )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate verify message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY );
    }

    ret = rsa_pkcs1_verify( &ssl->session_negotiate->peer_cert->rsa, RSA_PUBLIC,
                            hash_id, hashlen, hash, ssl->in_msg + 6 + n );
    if( ret != 0 )
    {
        SSL_DEBUG_RET( 1, "rsa_pkcs1_verify", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= parse certificate verify" ) );

    return( 0 );
}