int EncryptionUtil::encrypt(EncryptionMode mode, const unsigned char* source,
                            uint32_t source_length, const unsigned char* key, uint32_t key_length,
                            const unsigned char* iv, bool padding, unsigned char* encrypt) {
    const EVP_CIPHER* cipher = get_evp_type(mode);
    /* The encrypt key to be used for encryption */
    unsigned char encrypt_key[ENCRYPTION_MAX_KEY_LENGTH / 8];
    create_key(key, key_length, encrypt_key, mode);

    if (cipher == nullptr || (EVP_CIPHER_iv_length(cipher) > 0 && !iv)) {
        return AES_BAD_DATA;
    }
    EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_reset(cipher_ctx);
    int length = 0;
    int ret = do_encrypt(cipher_ctx, cipher, source, source_length, encrypt_key, iv, padding,
                         encrypt, &length);
    EVP_CIPHER_CTX_free(cipher_ctx);
    if (ret == 0) {
        ERR_clear_error();
        return AES_BAD_DATA;
    } else {
        return length;
    }
}