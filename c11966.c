ossl_cipher_set_key(VALUE self, VALUE key)
{
    EVP_CIPHER_CTX *ctx;

    StringValue(key);
    GetCipher(self, ctx);

    if (RSTRING_LEN(key) < EVP_CIPHER_CTX_key_length(ctx))
        ossl_raise(eCipherError, "key length too short");

    if (EVP_CipherInit_ex(ctx, NULL, NULL, (unsigned char *)RSTRING_PTR(key), NULL, -1) != 1)
        ossl_raise(eCipherError, NULL);

    return key;
}