int main( int argc, char *argv[] )
{
    int keysize;
    unsigned long i, j, tsc;
    unsigned char tmp[64];
#if defined(POLARSSL_ARC4_C)
    arc4_context arc4;
#endif
#if defined(POLARSSL_DES_C)
    des3_context des3;
    des_context des;
#endif
#if defined(POLARSSL_AES_C)
    aes_context aes;
#if defined(POLARSSL_GCM_C)
    gcm_context gcm;
#endif
#endif
#if defined(POLARSSL_BLOWFISH_C)
    blowfish_context blowfish;
#endif
#if defined(POLARSSL_CAMELLIA_C)
    camellia_context camellia;
#endif
#if defined(POLARSSL_RSA_C) && defined(POLARSSL_BIGNUM_C) &&    \
    defined(POLARSSL_GENPRIME)
    rsa_context rsa;
#endif
#if defined(POLARSSL_HAVEGE_C)
    havege_state hs;
#endif
#if defined(POLARSSL_CTR_DRBG_C)
    ctr_drbg_context    ctr_drbg;
#endif
    ((void) argc);
    ((void) argv);

    memset( buf, 0xAA, sizeof( buf ) );

    printf( "\n" );

#if defined(POLARSSL_MD4_C)
    printf( HEADER_FORMAT, "MD4" );
    fflush( stdout );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        md4( buf, BUFSIZE, tmp );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        md4( buf, BUFSIZE, tmp );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_MD5_C)
    printf( HEADER_FORMAT, "MD5" );
    fflush( stdout );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        md5( buf, BUFSIZE, tmp );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        md5( buf, BUFSIZE, tmp );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_SHA1_C)
    printf( HEADER_FORMAT, "SHA-1" );
    fflush( stdout );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        sha1( buf, BUFSIZE, tmp );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        sha1( buf, BUFSIZE, tmp );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_SHA2_C)
    printf( HEADER_FORMAT, "SHA-256" );
    fflush( stdout );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        sha2( buf, BUFSIZE, tmp, 0 );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        sha2( buf, BUFSIZE, tmp, 0 );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_SHA4_C)
    printf( HEADER_FORMAT, "SHA-512" );
    fflush( stdout );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        sha4( buf, BUFSIZE, tmp, 0 );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        sha4( buf, BUFSIZE, tmp, 0 );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_ARC4_C)
    printf( HEADER_FORMAT, "ARC4" );
    fflush( stdout );

    arc4_setup( &arc4, tmp, 32 );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        arc4_crypt( &arc4, BUFSIZE, buf, buf );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        arc4_crypt( &arc4, BUFSIZE, buf, buf );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_DES_C)
    printf( HEADER_FORMAT, "3DES" );
    fflush( stdout );

    des3_set3key_enc( &des3, tmp );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        des3_crypt_cbc( &des3, DES_ENCRYPT, BUFSIZE, tmp, buf, buf );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        des3_crypt_cbc( &des3, DES_ENCRYPT, BUFSIZE, tmp, buf, buf );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );

    printf( HEADER_FORMAT, "DES" );
    fflush( stdout );

    des_setkey_enc( &des, tmp );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        des_crypt_cbc( &des, DES_ENCRYPT, BUFSIZE, tmp, buf, buf );

    tsc = hardclock();
    for( j = 0; j < 1024; j++ )
        des_crypt_cbc( &des, DES_ENCRYPT, BUFSIZE, tmp, buf, buf );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_AES_C)
    for( keysize = 128; keysize <= 256; keysize += 64 )
    {
        printf( "  AES-CBC-%d     :  ", keysize );
        fflush( stdout );

        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );
        aes_setkey_enc( &aes, tmp, keysize );

        set_alarm( 1 );

        for( i = 1; ! alarmed; i++ )
            aes_crypt_cbc( &aes, AES_ENCRYPT, BUFSIZE, tmp, buf, buf );

        tsc = hardclock();
        for( j = 0; j < 4096; j++ )
            aes_crypt_cbc( &aes, AES_ENCRYPT, BUFSIZE, tmp, buf, buf );

        printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                        ( hardclock() - tsc ) / ( j * BUFSIZE ) );
    }
#if defined(POLARSSL_GCM_C)
    for( keysize = 128; keysize <= 256; keysize += 64 )
    {
        printf( "  AES-GCM-%d     :  ", keysize );
        fflush( stdout );

        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );
        gcm_init( &gcm, tmp, keysize );

        set_alarm( 1 );

        for( i = 1; ! alarmed; i++ )
            gcm_crypt_and_tag( &gcm, GCM_ENCRYPT, BUFSIZE, tmp, 12, NULL, 0, buf, buf, 16, tmp );

        tsc = hardclock();
        for( j = 0; j < 4096; j++ )
            gcm_crypt_and_tag( &gcm, GCM_ENCRYPT, BUFSIZE, tmp, 12, NULL, 0, buf, buf, 16, tmp );

        printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                        ( hardclock() - tsc ) / ( j * BUFSIZE ) );
    }
#endif
#endif

#if defined(POLARSSL_CAMELLIA_C)
    for( keysize = 128; keysize <= 256; keysize += 64 )
    {
        printf( "  CAMELLIA-CBC-%d:  ", keysize );
        fflush( stdout );

        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );
        camellia_setkey_enc( &camellia, tmp, keysize );

        set_alarm( 1 );

        for( i = 1; ! alarmed; i++ )
            camellia_crypt_cbc( &camellia, CAMELLIA_ENCRYPT, BUFSIZE, tmp, buf, buf );

        tsc = hardclock();
        for( j = 0; j < 4096; j++ )
            camellia_crypt_cbc( &camellia, CAMELLIA_ENCRYPT, BUFSIZE, tmp, buf, buf );

        printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                        ( hardclock() - tsc ) / ( j * BUFSIZE ) );
    }
#endif

#if defined(POLARSSL_BLOWFISH_C)
    for( keysize = 128; keysize <= 256; keysize += 64 )
    {
        printf( "  BLOWFISH-CBC-%d:  ", keysize );
        fflush( stdout );

        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );
        blowfish_setkey( &blowfish, tmp, keysize );

        set_alarm( 1 );

        for( i = 1; ! alarmed; i++ )
            blowfish_crypt_cbc( &blowfish, BLOWFISH_ENCRYPT, BUFSIZE, tmp, buf, buf );

        tsc = hardclock();
        for( j = 0; j < 4096; j++ )
            blowfish_crypt_cbc( &blowfish, BLOWFISH_ENCRYPT, BUFSIZE, tmp, buf, buf );

        printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                        ( hardclock() - tsc ) / ( j * BUFSIZE ) );
    }
#endif

#if defined(POLARSSL_HAVEGE_C)
    printf( HEADER_FORMAT, "HAVEGE" );
    fflush( stdout );

    havege_init( &hs );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        havege_random( &hs, buf, BUFSIZE );

    tsc = hardclock();
    for( j = 1; j < 1024; j++ )
        havege_random( &hs, buf, BUFSIZE );

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_CTR_DRBG_C)
    printf( HEADER_FORMAT, "CTR_DRBG (NOPR)" );
    fflush( stdout );

    if( ctr_drbg_init( &ctr_drbg, myrand, NULL, NULL, 0 ) != 0 )
        exit(1);

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
            exit(1);

    tsc = hardclock();
    for( j = 1; j < 1024; j++ )
        if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
            exit(1);

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );

    printf( HEADER_FORMAT, "CTR_DRBG (PR)" );
    fflush( stdout );

    if( ctr_drbg_init( &ctr_drbg, myrand, NULL, NULL, 0 ) != 0 )
        exit(1);

    ctr_drbg_set_prediction_resistance( &ctr_drbg, CTR_DRBG_PR_ON );

    set_alarm( 1 );
    for( i = 1; ! alarmed; i++ )
        if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
            exit(1);

    tsc = hardclock();
    for( j = 1; j < 1024; j++ )
        if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
            exit(1);

    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );
#endif

#if defined(POLARSSL_RSA_C) && defined(POLARSSL_BIGNUM_C) &&    \
    defined(POLARSSL_GENPRIME)
    rsa_init( &rsa, RSA_PKCS_V15, 0 );
    rsa_gen_key( &rsa, myrand, NULL, 1024, 65537 );

    printf( HEADER_FORMAT, "RSA-1024" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_public( &rsa, buf, buf );
    }

    printf( "%9lu  public/s\n", i / 3 );

    printf( HEADER_FORMAT, "RSA-1024" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_private( &rsa, buf, buf );
    }

    printf( "%9lu private/s\n", i / 3 );

    rsa_free( &rsa );

    rsa_init( &rsa, RSA_PKCS_V15, 0 );
    rsa_gen_key( &rsa, myrand, NULL, 2048, 65537 );

    printf( HEADER_FORMAT, "RSA-2048" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_public( &rsa, buf, buf );
    }

    printf( "%9lu  public/s\n", i / 3 );

    printf( HEADER_FORMAT, "RSA-2048" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_private( &rsa, buf, buf );
    }

    printf( "%9lu private/s\n", i / 3 );

    rsa_free( &rsa );

    rsa_init( &rsa, RSA_PKCS_V15, 0 );
    rsa_gen_key( &rsa, myrand, NULL, 4096, 65537 );

    printf( HEADER_FORMAT, "RSA-4096" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_public( &rsa, buf, buf );
    }

    printf( "%9lu  public/s\n", i / 3 );

    printf( HEADER_FORMAT, "RSA-4096" );
    fflush( stdout );
    set_alarm( 3 );

    for( i = 1; ! alarmed; i++ )
    {
        buf[0] = 0;
        rsa_private( &rsa, buf, buf );
    }

    printf( "%9lu private/s\n", i / 3 );

    rsa_free( &rsa );
#endif

    printf( "\n" );

#if defined(_WIN32)
    printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( 0 );
}