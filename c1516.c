static int ssl_parse_server_key_exchange( ssl_context *ssl )
{
#if defined(POLARSSL_DHM_C)
    int ret;
    size_t n;
    unsigned char *p, *end;
    unsigned char hash[64];
    md5_context md5;
    sha1_context sha1;
    int hash_id = SIG_RSA_RAW;
    unsigned int hashlen = 0;
#endif

    SSL_DEBUG_MSG( 2, ( "=> parse server key exchange" ) );

    if( ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_DES_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_128_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_256_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_128_CBC_SHA256 &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_256_CBC_SHA256 &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256 &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256 &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 &&
        ssl->session_negotiate->ciphersuite != TLS_DHE_RSA_WITH_AES_256_GCM_SHA384 )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip parse server key exchange" ) );
        ssl->state++;
        return( 0 );
    }

#if !defined(POLARSSL_DHM_C)
    SSL_DEBUG_MSG( 1, ( "support for dhm in not available" ) );
    return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
#else
    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_msg[0] != SSL_HS_SERVER_KEY_EXCHANGE )
    {
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE );
    }

    SSL_DEBUG_BUF( 3,   "server key exchange", ssl->in_msg + 4, ssl->in_hslen - 4 );

    /*
     * Ephemeral DH parameters:
     *
     * struct {
     *     opaque dh_p<1..2^16-1>;
     *     opaque dh_g<1..2^16-1>;
     *     opaque dh_Ys<1..2^16-1>;
     * } ServerDHParams;
     */
    p   = ssl->in_msg + 4;
    end = ssl->in_msg + ssl->in_hslen;

    if( ( ret = dhm_read_params( &ssl->handshake->dhm_ctx, &p, end ) ) != 0 )
    {
        SSL_DEBUG_MSG( 2, ( "DHM Read Params returned -0x%x", -ret ) );
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE ); 
    }

    if( ssl->minor_ver == SSL_MINOR_VERSION_3 )
    {
        if( p[1] != SSL_SIG_RSA )
        {
            SSL_DEBUG_MSG( 2, ( "server used unsupported SignatureAlgorithm %d", p[1] ) );
            SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE ); 
        }

        switch( p[0] )
        {
#if defined(POLARSSL_MD5_C)
            case SSL_HASH_MD5:
                hash_id = SIG_RSA_MD5;
                break;
#endif
#if defined(POLARSSL_SHA1_C)
            case SSL_HASH_SHA1:
                hash_id = SIG_RSA_SHA1;
                break;
#endif
#if defined(POLARSSL_SHA2_C)
            case SSL_HASH_SHA224:
                hash_id = SIG_RSA_SHA224;
                break;
            case SSL_HASH_SHA256:
                hash_id = SIG_RSA_SHA256;
                break;
#endif
#if defined(POLARSSL_SHA4_C)
            case SSL_HASH_SHA384:
                hash_id = SIG_RSA_SHA384;
                break;
            case SSL_HASH_SHA512:
                hash_id = SIG_RSA_SHA512;
                break;
#endif
            default:
                SSL_DEBUG_MSG( 2, ( "Server used unsupported HashAlgorithm %d", p[0] ) );
                SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
                return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE ); 
        }      

        SSL_DEBUG_MSG( 2, ( "Server used SignatureAlgorithm %d", p[1] ) );
        SSL_DEBUG_MSG( 2, ( "Server used HashAlgorithm %d", p[0] ) );
        p += 2;
    }

    n = ( p[0] << 8 ) | p[1];
    p += 2;

    if( end != p + n )
    {
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE );
    }

    if( (unsigned int)( end - p ) !=
        ssl->session_negotiate->peer_cert->rsa.len )
    {
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE );
    }

    if( ssl->handshake->dhm_ctx.len < 64 || ssl->handshake->dhm_ctx.len > 512 )
    {
        SSL_DEBUG_MSG( 1, ( "bad server key exchange message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE );
    }

    SSL_DEBUG_MPI( 3, "DHM: P ", &ssl->handshake->dhm_ctx.P  );
    SSL_DEBUG_MPI( 3, "DHM: G ", &ssl->handshake->dhm_ctx.G  );
    SSL_DEBUG_MPI( 3, "DHM: GY", &ssl->handshake->dhm_ctx.GY );

    if( ssl->minor_ver != SSL_MINOR_VERSION_3 )
    {
        /*
         * digitally-signed struct {
         *     opaque md5_hash[16];
         *     opaque sha_hash[20];
         * };
         *
         * md5_hash
         *     MD5(ClientHello.random + ServerHello.random
         *                            + ServerParams);
         * sha_hash
         *     SHA(ClientHello.random + ServerHello.random
         *                            + ServerParams);
         */
        n = ssl->in_hslen - ( end - p ) - 6;

        md5_starts( &md5 );
        md5_update( &md5, ssl->handshake->randbytes, 64 );
        md5_update( &md5, ssl->in_msg + 4, n );
        md5_finish( &md5, hash );

        sha1_starts( &sha1 );
        sha1_update( &sha1, ssl->handshake->randbytes, 64 );
        sha1_update( &sha1, ssl->in_msg + 4, n );
        sha1_finish( &sha1, hash + 16 );

        hash_id = SIG_RSA_RAW;
        hashlen = 36;
    }
    else
    {
        sha2_context sha2;
#if defined(POLARSSL_SHA4_C)
        sha4_context sha4;
#endif

        n = ssl->in_hslen - ( end - p ) - 8;

        /*
         * digitally-signed struct {
         *     opaque client_random[32];
         *     opaque server_random[32];
         *     ServerDHParams params;
         * };
         */
        switch( hash_id )
        {
#if defined(POLARSSL_MD5_C)
            case SIG_RSA_MD5:
                md5_starts( &md5 );
                md5_update( &md5, ssl->handshake->randbytes, 64 );
                md5_update( &md5, ssl->in_msg + 4, n );
                md5_finish( &md5, hash );
                hashlen = 16;
                break;
#endif
#if defined(POLARSSL_SHA1_C)
            case SIG_RSA_SHA1:
                sha1_starts( &sha1 );
                sha1_update( &sha1, ssl->handshake->randbytes, 64 );
                sha1_update( &sha1, ssl->in_msg + 4, n );
                sha1_finish( &sha1, hash );
                hashlen = 20;
                break;
#endif
#if defined(POLARSSL_SHA2_C)
            case SIG_RSA_SHA224:
                sha2_starts( &sha2, 1 );
                sha2_update( &sha2, ssl->handshake->randbytes, 64 );
                sha2_update( &sha2, ssl->in_msg + 4, n );
                sha2_finish( &sha2, hash );
                hashlen = 28;
                break;
            case SIG_RSA_SHA256:
                sha2_starts( &sha2, 0 );
                sha2_update( &sha2, ssl->handshake->randbytes, 64 );
                sha2_update( &sha2, ssl->in_msg + 4, n );
                sha2_finish( &sha2, hash );
                hashlen = 32;
                break;
#endif
#if defined(POLARSSL_SHA4_C)
            case SIG_RSA_SHA384:
                sha4_starts( &sha4, 1 );
                sha4_update( &sha4, ssl->handshake->randbytes, 64 );
                sha4_update( &sha4, ssl->in_msg + 4, n );
                sha4_finish( &sha4, hash );
                hashlen = 48;
                break;
            case SIG_RSA_SHA512:
                sha4_starts( &sha4, 0 );
                sha4_update( &sha4, ssl->handshake->randbytes, 64 );
                sha4_update( &sha4, ssl->in_msg + 4, n );
                sha4_finish( &sha4, hash );
                hashlen = 64;
                break;
#endif
        }
    }
    
    SSL_DEBUG_BUF( 3, "parameters hash", hash, hashlen );

    if( ( ret = rsa_pkcs1_verify( &ssl->session_negotiate->peer_cert->rsa,
                                  RSA_PUBLIC,
                                  hash_id, hashlen, hash, p ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "rsa_pkcs1_verify", ret );
        return( ret );
    }

    ssl->state++;

    SSL_DEBUG_MSG( 2, ( "<= parse server key exchange" ) );

    return( 0 );
#endif
}