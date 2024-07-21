static int aesni_cbc_hmac_sha1_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg,
                                    void *ptr)
{
    EVP_AES_HMAC_SHA1 *key = data(ctx);

    switch (type) {
    case EVP_CTRL_AEAD_SET_MAC_KEY:
        {
            unsigned int i;
            unsigned char hmac_key[64];

            memset(hmac_key, 0, sizeof(hmac_key));

            if (arg > (int)sizeof(hmac_key)) {
                SHA1_Init(&key->head);
                SHA1_Update(&key->head, ptr, arg);
                SHA1_Final(hmac_key, &key->head);
            } else {
                memcpy(hmac_key, ptr, arg);
            }

            for (i = 0; i < sizeof(hmac_key); i++)
                hmac_key[i] ^= 0x36; /* ipad */
            SHA1_Init(&key->head);
            SHA1_Update(&key->head, hmac_key, sizeof(hmac_key));

            for (i = 0; i < sizeof(hmac_key); i++)
                hmac_key[i] ^= 0x36 ^ 0x5c; /* opad */
            SHA1_Init(&key->tail);
            SHA1_Update(&key->tail, hmac_key, sizeof(hmac_key));

            OPENSSL_cleanse(hmac_key, sizeof(hmac_key));

            return 1;
        }
    case EVP_CTRL_AEAD_TLS1_AAD:
        {
            unsigned char *p = ptr;
            unsigned int len = p[arg - 2] << 8 | p[arg - 1];

            if (ctx->encrypt) {
                key->payload_length = len;
                if ((key->aux.tls_ver =
                     p[arg - 4] << 8 | p[arg - 3]) >= TLS1_1_VERSION) {
                    len -= AES_BLOCK_SIZE;
                    p[arg - 2] = len >> 8;
                    p[arg - 1] = len;
                }
                key->md = key->head;
                SHA1_Update(&key->md, p, arg);

                return (int)(((len + SHA_DIGEST_LENGTH +
                               AES_BLOCK_SIZE) & -AES_BLOCK_SIZE)
                             - len);
            } else {
                if (arg > 13)
                    arg = 13;
                memcpy(key->aux.tls_aad, ptr, arg);
                key->payload_length = arg;

                return SHA_DIGEST_LENGTH;
            }
        }
#  if !defined(OPENSSL_NO_MULTIBLOCK) && EVP_CIPH_FLAG_TLS1_1_MULTIBLOCK
    case EVP_CTRL_TLS1_1_MULTIBLOCK_MAX_BUFSIZE:
        return (int)(5 + 16 + ((arg + 20 + 16) & -16));
    case EVP_CTRL_TLS1_1_MULTIBLOCK_AAD:
        {
            EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM *param =
                (EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM *) ptr;
            unsigned int n4x = 1, x4;
            unsigned int frag, last, packlen, inp_len;

            if (arg < (int)sizeof(EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM))
                return -1;

            inp_len = param->inp[11] << 8 | param->inp[12];

            if (ctx->encrypt) {
                if ((param->inp[9] << 8 | param->inp[10]) < TLS1_1_VERSION)
                    return -1;

                if (inp_len) {
                    if (inp_len < 4096)
                        return 0; /* too short */

                    if (inp_len >= 8192 && OPENSSL_ia32cap_P[2] & (1 << 5))
                        n4x = 2; /* AVX2 */
                } else if ((n4x = param->interleave / 4) && n4x <= 2)
                    inp_len = param->len;
                else
                    return -1;

                key->md = key->head;
                SHA1_Update(&key->md, param->inp, 13);

                x4 = 4 * n4x;
                n4x += 1;

                frag = inp_len >> n4x;
                last = inp_len + frag - (frag << n4x);
                if (last > frag && ((last + 13 + 9) % 64 < (x4 - 1))) {
                    frag++;
                    last -= x4 - 1;
                }

                packlen = 5 + 16 + ((frag + 20 + 16) & -16);
                packlen = (packlen << n4x) - packlen;
                packlen += 5 + 16 + ((last + 20 + 16) & -16);

                param->interleave = x4;

                return (int)packlen;
            } else
                return -1;      /* not yet */
        }
    case EVP_CTRL_TLS1_1_MULTIBLOCK_ENCRYPT:
        {
            EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM *param =
                (EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM *) ptr;

            return (int)tls1_1_multi_block_encrypt(key, param->out,
                                                   param->inp, param->len,
                                                   param->interleave / 4);
        }
    case EVP_CTRL_TLS1_1_MULTIBLOCK_DECRYPT:
#  endif
    default:
        return -1;
    }
}