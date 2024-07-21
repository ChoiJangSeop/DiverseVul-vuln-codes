static int ssl_parse_client_key_exchange( ssl_context *ssl )
{
    int ret;
    size_t i, n = 0;

    SSL_DEBUG_MSG( 2, ( "=> parse client key exchange" ) );

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE );
    }

    if( ssl->in_msg[0] != SSL_HS_CLIENT_KEY_EXCHANGE )
    {
        SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE );
    }

    if( ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_DES_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_128_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_256_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_128_CBC_SHA256 ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_256_CBC_SHA256 ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256 ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256 ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 ||
        ssl->session_negotiate->ciphersuite == TLS_DHE_RSA_WITH_AES_256_GCM_SHA384 )
    {
#if !defined(POLARSSL_DHM_C)
        SSL_DEBUG_MSG( 1, ( "support for dhm is not available" ) );
        return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
#else
        /*
         * Receive G^Y mod P, premaster = (G^Y)^X mod P
         */
        n = ( ssl->in_msg[4] << 8 ) | ssl->in_msg[5];

        if( n < 1 || n > ssl->handshake->dhm_ctx.len ||
            n + 6 != ssl->in_hslen )
        {
            SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE );
        }

        if( ( ret = dhm_read_public( &ssl->handshake->dhm_ctx,
                                      ssl->in_msg + 6, n ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "dhm_read_public", ret );
            return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_DHM_RP );
        }

        SSL_DEBUG_MPI( 3, "DHM: GY", &ssl->handshake->dhm_ctx.GY );

        ssl->handshake->pmslen = ssl->handshake->dhm_ctx.len;

        if( ( ret = dhm_calc_secret( &ssl->handshake->dhm_ctx,
                                      ssl->handshake->premaster,
                                     &ssl->handshake->pmslen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "dhm_calc_secret", ret );
            return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_DHM_CS );
        }

        SSL_DEBUG_MPI( 3, "DHM: K ", &ssl->handshake->dhm_ctx.K  );
#endif
    }
    else
    {
        if( ssl->rsa_key == NULL )
        {
            SSL_DEBUG_MSG( 1, ( "got no private key" ) );
            return( POLARSSL_ERR_SSL_PRIVATE_KEY_REQUIRED );
        }

        /*
         * Decrypt the premaster using own private RSA key
         */
        i = 4;
        if( ssl->rsa_key )
            n = ssl->rsa_key_len( ssl->rsa_key );
        ssl->handshake->pmslen = 48;

        if( ssl->minor_ver != SSL_MINOR_VERSION_0 )
        {
            i += 2;
            if( ssl->in_msg[4] != ( ( n >> 8 ) & 0xFF ) ||
                ssl->in_msg[5] != ( ( n      ) & 0xFF ) )
            {
                SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );
                return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE );
            }
        }

        if( ssl->in_hslen != i + n )
        {
            SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE );
        }

        if( ssl->rsa_key ) {
            ret = ssl->rsa_decrypt( ssl->rsa_key, RSA_PRIVATE,
                                   &ssl->handshake->pmslen,
                                    ssl->in_msg + i,
                                    ssl->handshake->premaster,
                                    sizeof(ssl->handshake->premaster) );
        }

        if( ret != 0 || ssl->handshake->pmslen != 48 ||
            ssl->handshake->premaster[0] != ssl->max_major_ver ||
            ssl->handshake->premaster[1] != ssl->max_minor_ver )
        {
            SSL_DEBUG_MSG( 1, ( "bad client key exchange message" ) );

            /*
             * Protection against Bleichenbacher's attack:
             * invalid PKCS#1 v1.5 padding must not cause
             * the connection to end immediately; instead,
             * send a bad_record_mac later in the handshake.
             */
            ssl->handshake->pmslen = 48;

            ret = ssl->f_rng( ssl->p_rng, ssl->handshake->premaster,
                              ssl->handshake->pmslen );
            if( ret != 0 )
                return( ret );
        }
    }

    if( ( ret = ssl_derive_keys( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_derive_keys", ret );
        return( ret );
    }

    ssl->state++;

    SSL_DEBUG_MSG( 2, ( "<= parse client key exchange" ) );

    return( 0 );
}