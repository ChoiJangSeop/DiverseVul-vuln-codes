int rsa_rsaes_pkcs1_v15_encrypt( rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t ilen,
                                 const unsigned char *input,
                                 unsigned char *output )
{
    size_t nb_pad, olen;
    int ret;
    unsigned char *p = output;

    if( ctx->padding != RSA_PKCS_V15 || f_rng == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;

    if( olen < ilen + 11 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    nb_pad = olen - 3 - ilen;

    *p++ = 0;
    if( mode == RSA_PUBLIC )
    {
        *p++ = RSA_CRYPT;

        while( nb_pad-- > 0 )
        {
            int rng_dl = 100;

            do {
                ret = f_rng( p_rng, p, 1 );
            } while( *p == 0 && --rng_dl && ret == 0 );

            // Check if RNG failed to generate data
            //
            if( rng_dl == 0 || ret != 0)
                return POLARSSL_ERR_RSA_RNG_FAILED + ret;

            p++;
        }
    }
    else
    {
        *p++ = RSA_SIGN;

        while( nb_pad-- > 0 )
            *p++ = 0xFF;
    }

    *p++ = 0;
    memcpy( p, input, ilen );

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, output, output )
            : rsa_private( ctx, output, output ) );
}