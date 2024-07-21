int rsa_rsaes_oaep_decrypt( rsa_context *ctx,
                            int mode, 
                            const unsigned char *label, size_t label_len,
                            size_t *olen,
                            const unsigned char *input,
                            unsigned char *output,
                            size_t output_max_len )
{
    int ret;
    size_t ilen;
    unsigned char *p;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];
    unsigned char lhash[POLARSSL_MD_MAX_SIZE];
    unsigned int hlen;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( ctx->padding != RSA_PKCS_V21 )
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
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    md_info = md_info_from_type( ctx->hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    hlen = md_get_size( md_info );

    md_init_ctx( &md_ctx, md_info );

    // Generate lHash
    //
    md( md_info, label, label_len, lhash );

    // seed: Apply seedMask to maskedSeed
    //
    mgf_mask( buf + 1, hlen, buf + hlen + 1, ilen - hlen - 1,
               &md_ctx );

    // DB: Apply dbMask to maskedDB
    //
    mgf_mask( buf + hlen + 1, ilen - hlen - 1, buf + 1, hlen,
               &md_ctx );

    p += hlen;
    md_free_ctx( &md_ctx );

    // Check validity
    //
    if( memcmp( lhash, p, hlen ) != 0 )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    p += hlen;

    while( *p == 0 && p < buf + ilen )
        p++;

    if( p == buf + ilen )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if( *p++ != 0x01 )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if (ilen - (p - buf) > output_max_len)
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}