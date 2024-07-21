int rsa_self_test( int verbose )
{
    size_t len;
    rsa_context rsa;
    unsigned char rsa_plaintext[PT_LEN];
    unsigned char rsa_decrypted[PT_LEN];
    unsigned char rsa_ciphertext[KEY_LEN];
#if defined(POLARSSL_SHA1_C)
    unsigned char sha1sum[20];
#endif

    rsa_init( &rsa, RSA_PKCS_V15, 0 );

    rsa.len = KEY_LEN;
    mpi_read_string( &rsa.N , 16, RSA_N  );
    mpi_read_string( &rsa.E , 16, RSA_E  );
    mpi_read_string( &rsa.D , 16, RSA_D  );
    mpi_read_string( &rsa.P , 16, RSA_P  );
    mpi_read_string( &rsa.Q , 16, RSA_Q  );
    mpi_read_string( &rsa.DP, 16, RSA_DP );
    mpi_read_string( &rsa.DQ, 16, RSA_DQ );
    mpi_read_string( &rsa.QP, 16, RSA_QP );

    if( verbose != 0 )
        printf( "  RSA key validation: " );

    if( rsa_check_pubkey(  &rsa ) != 0 ||
        rsa_check_privkey( &rsa ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 encryption : " );

    memcpy( rsa_plaintext, RSA_PT, PT_LEN );

    if( rsa_pkcs1_encrypt( &rsa, &myrand, NULL, RSA_PUBLIC, PT_LEN,
                           rsa_plaintext, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 decryption : " );

    if( rsa_pkcs1_decrypt( &rsa, RSA_PRIVATE, &len,
                           rsa_ciphertext, rsa_decrypted,
                           sizeof(rsa_decrypted) ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( memcmp( rsa_decrypted, rsa_plaintext, len ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

#if defined(POLARSSL_SHA1_C)
    if( verbose != 0 )
        printf( "passed\n  PKCS#1 data sign  : " );

    sha1( rsa_plaintext, PT_LEN, sha1sum );

    if( rsa_pkcs1_sign( &rsa, NULL, NULL, RSA_PRIVATE, SIG_RSA_SHA1, 20,
                        sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 sig. verify: " );

    if( rsa_pkcs1_verify( &rsa, RSA_PUBLIC, SIG_RSA_SHA1, 20,
                          sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n\n" );
#endif /* POLARSSL_SHA1_C */

    rsa_free( &rsa );

    return( 0 );
}