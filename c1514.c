int main( int argc, char *argv[] )
{
    ((void) argc);
    ((void) argv);

    printf("POLARSSL_BIGNUM_C and/or POLARSSL_RSA_C and/or "
           "POLARSSL_SHA1_C and/or POLARSSL_X509_PARSE_C and/or "
           "POLARSSL_FS_IO not defined.\n");
    return( 0 );
}