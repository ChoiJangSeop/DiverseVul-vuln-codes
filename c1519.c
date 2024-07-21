int main( int argc, char *argv[] )
{
    FILE *f;
    int ret;
    size_t i;
    rsa_context rsa;
    unsigned char hash[20];
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];

    ret = 1;

    if( argc != 2 )
    {
        printf( "usage: rsa_sign <filename>\n" );

#if defined(_WIN32)
        printf( "\n" );
#endif

        goto exit;
    }

    printf( "\n  . Reading private key from rsa_priv.txt" );
    fflush( stdout );

    if( ( f = fopen( "rsa_priv.txt", "rb" ) ) == NULL )
    {
        ret = 1;
        printf( " failed\n  ! Could not open rsa_priv.txt\n" \
                "  ! Please run rsa_genkey first\n\n" );
        goto exit;
    }

    rsa_init( &rsa, RSA_PKCS_V15, 0 );
    
    if( ( ret = mpi_read_file( &rsa.N , 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.E , 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.D , 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.P , 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.Q , 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.DP, 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.DQ, 16, f ) ) != 0 ||
        ( ret = mpi_read_file( &rsa.QP, 16, f ) ) != 0 )
    {
        printf( " failed\n  ! mpi_read_file returned %d\n\n", ret );
        goto exit;
    }

    rsa.len = ( mpi_msb( &rsa.N ) + 7 ) >> 3;

    fclose( f );

    printf( "\n  . Checking the private key" );
    fflush( stdout );
    if( ( ret = rsa_check_privkey( &rsa ) ) != 0 )
    {
        printf( " failed\n  ! rsa_check_privkey failed with -0x%0x\n", -ret );
        goto exit;
    }

    /*
     * Compute the SHA-1 hash of the input file,
     * then calculate the RSA signature of the hash.
     */
    printf( "\n  . Generating the RSA/SHA-1 signature" );
    fflush( stdout );

    if( ( ret = sha1_file( argv[1], hash ) ) != 0 )
    {
        printf( " failed\n  ! Could not open or read %s\n\n", argv[1] );
        goto exit;
    }

    if( ( ret = rsa_pkcs1_sign( &rsa, NULL, NULL, RSA_PRIVATE, SIG_RSA_SHA1,
                                20, hash, buf ) ) != 0 )
    {
        printf( " failed\n  ! rsa_pkcs1_sign returned -0x%0x\n\n", -ret );
        goto exit;
    }

    /*
     * Write the signature into <filename>-sig.txt
     */
    memcpy( argv[1] + strlen( argv[1] ), ".sig", 5 );

    if( ( f = fopen( argv[1], "wb+" ) ) == NULL )
    {
        ret = 1;
        printf( " failed\n  ! Could not create %s\n\n", argv[1] );
        goto exit;
    }

    for( i = 0; i < rsa.len; i++ )
        fprintf( f, "%02X%s", buf[i],
                 ( i + 1 ) % 16 == 0 ? "\r\n" : " " );

    fclose( f );

    printf( "\n  . Done (created \"%s\")\n\n", argv[1] );

exit:

#if defined(_WIN32)
    printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( ret );
}