int rsa_rsassa_pss_verify( rsa_context *ctx,
                           int mode,
                           int hash_id,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           unsigned char *sig )
{
    int ret;
    size_t siglen;
    unsigned char *p;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];
    unsigned char result[POLARSSL_MD_MAX_SIZE];
    unsigned char zeros[8];
    unsigned int hlen;
    size_t slen, msb;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( ctx->padding != RSA_PKCS_V21 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    siglen = ctx->len;

    if( siglen < 16 || siglen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, sig, buf )
          : rsa_private( ctx, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( buf[siglen - 1] != 0xBC )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    switch( hash_id )
    {
        case SIG_RSA_MD2:
        case SIG_RSA_MD4:
        case SIG_RSA_MD5:
            hashlen = 16;
            break;

        case SIG_RSA_SHA1:
            hashlen = 20;
            break;

        case SIG_RSA_SHA224:
            hashlen = 28;
            break;

        case SIG_RSA_SHA256:
            hashlen = 32;
            break;

        case SIG_RSA_SHA384:
            hashlen = 48;
            break;

        case SIG_RSA_SHA512:
            hashlen = 64;
            break;

        default:
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    md_info = md_info_from_type( ctx->hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    hlen = md_get_size( md_info );
    slen = siglen - hlen - 1;

    memset( zeros, 0, 8 );

    // Note: EMSA-PSS verification is over the length of N - 1 bits
    //
    msb = mpi_msb( &ctx->N ) - 1;

    // Compensate for boundary condition when applying mask
    //
    if( msb % 8 == 0 )
    {
        p++;
        siglen -= 1;
    }
    if( buf[0] >> ( 8 - siglen * 8 + msb ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    md_init_ctx( &md_ctx, md_info );

    mgf_mask( p, siglen - hlen - 1, p + siglen - hlen - 1, hlen, &md_ctx );

    buf[0] &= 0xFF >> ( siglen * 8 - msb );

    while( *p == 0 && p < buf + siglen )
        p++;

    if( p == buf + siglen ||
        *p++ != 0x01 )
    {
        md_free_ctx( &md_ctx );
        return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    slen -= p - buf;

    // Generate H = Hash( M' )
    //
    md_starts( &md_ctx );
    md_update( &md_ctx, zeros, 8 );
    md_update( &md_ctx, hash, hashlen );
    md_update( &md_ctx, p, slen );
    md_finish( &md_ctx, result );

    md_free_ctx( &md_ctx );

    if( memcmp( p + slen, result, hlen ) == 0 )
        return( 0 );
    else
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );
}