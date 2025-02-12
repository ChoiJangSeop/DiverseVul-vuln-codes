static int rc4_hmac_md5_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg,
                             void *ptr)
{
    EVP_RC4_HMAC_MD5 *key = data(ctx);

    switch (type) {
    case EVP_CTRL_AEAD_SET_MAC_KEY:
        {
            unsigned int i;
            unsigned char hmac_key[64];

            memset(hmac_key, 0, sizeof(hmac_key));

            if (arg > (int)sizeof(hmac_key)) {
                MD5_Init(&key->head);
                MD5_Update(&key->head, ptr, arg);
                MD5_Final(hmac_key, &key->head);
            } else {
                memcpy(hmac_key, ptr, arg);
            }

            for (i = 0; i < sizeof(hmac_key); i++)
                hmac_key[i] ^= 0x36; /* ipad */
            MD5_Init(&key->head);
            MD5_Update(&key->head, hmac_key, sizeof(hmac_key));

            for (i = 0; i < sizeof(hmac_key); i++)
                hmac_key[i] ^= 0x36 ^ 0x5c; /* opad */
            MD5_Init(&key->tail);
            MD5_Update(&key->tail, hmac_key, sizeof(hmac_key));

            return 1;
        }
    case EVP_CTRL_AEAD_TLS1_AAD:
        {
            unsigned char *p = ptr;
            unsigned int len = p[arg - 2] << 8 | p[arg - 1];

            if (!ctx->encrypt) {
                len -= MD5_DIGEST_LENGTH;
                p[arg - 2] = len >> 8;
                p[arg - 1] = len;
            }
            key->payload_length = len;
            key->md = key->head;
            MD5_Update(&key->md, p, arg);

            return MD5_DIGEST_LENGTH;
        }
    default:
        return -1;
    }
}