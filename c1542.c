int main( int argc, char *argv[] )
{
    ((void) argc);
    ((void) argv);

    printf("POLARSSL_AES_C and/or POLARSSL_DHM_C and/or POLARSSL_ENTROPY_C "
           "and/or POLARSSL_NET_C and/or POLARSSL_RSA_C and/or "
           "POLARSSL_SHA1_C and/or POLARSSL_FS_IO and/or "
           "POLARSSL_CTR_DBRG_C not defined.\n");
    return( 0 );
}