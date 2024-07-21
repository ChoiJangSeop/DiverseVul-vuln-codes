static int pkey_sm2_decrypt(EVP_PKEY_CTX *ctx,
                            unsigned char *out, size_t *outlen,
                            const unsigned char *in, size_t inlen)
{
    EC_KEY *ec = ctx->pkey->pkey.ec;
    SM2_PKEY_CTX *dctx = ctx->data;
    const EVP_MD *md = (dctx->md == NULL) ? EVP_sm3() : dctx->md;

    if (out == NULL) {
        if (!sm2_plaintext_size(ec, md, inlen, outlen))
            return -1;
        else
            return 1;
    }

    return sm2_decrypt(ec, md, in, inlen, out, outlen);
}