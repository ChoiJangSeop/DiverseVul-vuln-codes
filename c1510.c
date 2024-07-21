int rsa_pkcs1_decrypt( rsa_context *ctx,
                       int mode, size_t *olen,
                       const unsigned char *input,
                       unsigned char *output,
                       size_t output_max_len)
{
    switch( ctx->padding )
    {
        case RSA_PKCS_V15:
            return rsa_rsaes_pkcs1_v15_decrypt( ctx, mode, olen, input, output,
                                                output_max_len );

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsaes_oaep_decrypt( ctx, mode, NULL, 0, olen, input,
                                           output, output_max_len );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}