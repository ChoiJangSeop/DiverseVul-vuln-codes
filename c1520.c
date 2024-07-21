int rsa_rsaes_pkcs1_v15_decrypt( rsa_context *ctx,
                                 int mode, size_t *olen,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 size_t output_max_len)
{
    int ret, correct = 1;
    size_t ilen, pad_count = 0;
    unsigned char *p, *q;
    unsigned char bt;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];

    if( ctx->padding != RSA_PKCS_V15 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, input, buf )
          : rsa_private( ctx, input, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( *p++ != 0 )
        correct = 0;

    bt = *p++;
    if( ( bt != RSA_CRYPT && mode == RSA_PRIVATE ) ||
        ( bt != RSA_SIGN && mode == RSA_PUBLIC ) )
    {
        correct = 0;
    }

    if( bt == RSA_CRYPT )
    {
        while( *p != 0 && p < buf + ilen - 1 )
            pad_count += ( *p++ != 0 );

        correct &= ( *p == 0 && p < buf + ilen - 1 );

        q = p;

        // Also pass over all other bytes to reduce timing differences
        //
        while ( q < buf + ilen - 1 )
            pad_count += ( *q++ != 0 );

        // Prevent compiler optimization of pad_count
        //
        correct |= pad_count & 0x100000; /* Always 0 unless 1M bit keys */
        p++;
    }
    else
    {
        while( *p == 0xFF && p < buf + ilen - 1 )
            pad_count += ( *p++ == 0xFF );

        correct &= ( *p == 0 && p < buf + ilen - 1 );

        q = p;

        // Also pass over all other bytes to reduce timing differences
        //
        while ( q < buf + ilen - 1 )
            pad_count += ( *q++ != 0 );

        // Prevent compiler optimization of pad_count
        //
        correct |= pad_count & 0x100000; /* Always 0 unless 1M bit keys */
        p++;
    }

    if( correct == 0 )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if (ilen - (p - buf) > output_max_len)
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}