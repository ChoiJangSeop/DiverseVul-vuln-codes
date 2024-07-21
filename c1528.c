static int ssl_rsa_decrypt( void *ctx, int mode, size_t *olen,
                        const unsigned char *input, unsigned char *output,
                        size_t output_max_len )
{
    return rsa_pkcs1_decrypt( (rsa_context *) ctx, mode, olen, input, output,
                              output_max_len );
}