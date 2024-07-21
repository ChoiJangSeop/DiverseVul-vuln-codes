int rsa_pkcs1_verify( rsa_context *ctx,
                      int mode,
                      int hash_id,
                      unsigned int hashlen,
                      const unsigned char *hash,
                      unsigned char *sig )
{
    switch( ctx->padding )
    {
        case RSA_PKCS_V15:
            return rsa_rsassa_pkcs1_v15_verify( ctx, mode, hash_id,
                                                hashlen, hash, sig );

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsassa_pss_verify( ctx, mode, hash_id,
                                          hashlen, hash, sig );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}