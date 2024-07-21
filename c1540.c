int rsa_rsassa_pss_sign( rsa_context *ctx,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
                         int mode,
                         int hash_id,
                         unsigned int hashlen,
                         const unsigned char *hash,
                         unsigned char *sig )
{
    size_t olen;
    unsigned char *p = sig;
    unsigned char salt[POLARSSL_MD_MAX_SIZE];
    unsigned int slen, hlen, offset = 0;
    int ret;
    size_t msb;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( ctx->padding != RSA_PKCS_V21 || f_rng == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;

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
    slen = hlen;

    if( olen < hlen + slen + 2 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    memset( sig, 0, olen );

    msb = mpi_msb( &ctx->N ) - 1;

    // Generate salt of length slen
    //
    if( ( ret = f_rng( p_rng, salt, slen ) ) != 0 )
        return( POLARSSL_ERR_RSA_RNG_FAILED + ret );

    // Note: EMSA-PSS encoding is over the length of N - 1 bits
    //
    msb = mpi_msb( &ctx->N ) - 1;
    p += olen - hlen * 2 - 2;
    *p++ = 0x01;
    memcpy( p, salt, slen );
    p += slen;

    md_init_ctx( &md_ctx, md_info );

    // Generate H = Hash( M' )
    //
    md_starts( &md_ctx );
    md_update( &md_ctx, p, 8 );
    md_update( &md_ctx, hash, hashlen );
    md_update( &md_ctx, salt, slen );
    md_finish( &md_ctx, p );

    // Compensate for boundary condition when applying mask
    //
    if( msb % 8 == 0 )
        offset = 1;

    // maskedDB: Apply dbMask to DB
    //
    mgf_mask( sig + offset, olen - hlen - 1 - offset, p, hlen, &md_ctx );

    md_free_ctx( &md_ctx );

    msb = mpi_msb( &ctx->N ) - 1;
    sig[0] &= 0xFF >> ( olen * 8 - msb );

    p += hlen;
    *p++ = 0xBC;

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, sig, sig )
            : rsa_private( ctx, sig, sig ) );
}