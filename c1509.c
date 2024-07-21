int rsa_pkcs1_sign( rsa_context *ctx,
                    int (*f_rng)(void *, unsigned char *, size_t),
                    void *p_rng,
                    int mode,
                    int hash_id,
                    unsigned int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig )
{
    switch( ctx->padding )
    {
        case RSA_PKCS_V15:
            return rsa_rsassa_pkcs1_v15_sign( ctx, mode, hash_id,
                                              hashlen, hash, sig );

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsassa_pss_sign( ctx, f_rng, p_rng, mode, hash_id,
                                        hashlen, hash, sig );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}