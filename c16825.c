static void multiblock_speed(const EVP_CIPHER *evp_cipher)
{
    static int mblengths[] =
        { 8 * 1024, 2 * 8 * 1024, 4 * 8 * 1024, 8 * 8 * 1024, 8 * 16 * 1024 };
    int j, count, num = sizeof(lengths) / sizeof(lengths[0]);
    const char *alg_name;
    unsigned char *inp, *out, no_key[32], no_iv[16];
    EVP_CIPHER_CTX ctx;
    double d = 0.0;

    inp = OPENSSL_malloc(mblengths[num - 1]);
    out = OPENSSL_malloc(mblengths[num - 1] + 1024);
    if (!inp || !out) {
        BIO_printf(bio_err,"Out of memory\n");
        goto end;
    }


    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx, evp_cipher, NULL, no_key, no_iv);
    EVP_CIPHER_CTX_ctrl(&ctx, EVP_CTRL_AEAD_SET_MAC_KEY, sizeof(no_key),
                        no_key);
    alg_name = OBJ_nid2ln(evp_cipher->nid);

    for (j = 0; j < num; j++) {
        print_message(alg_name, 0, mblengths[j]);
        Time_F(START);
        for (count = 0, run = 1; run && count < 0x7fffffff; count++) {
            unsigned char aad[13];
            EVP_CTRL_TLS1_1_MULTIBLOCK_PARAM mb_param;
            size_t len = mblengths[j];
            int packlen;

            memset(aad, 0, 8);  /* avoid uninitialized values */
            aad[8] = 23;        /* SSL3_RT_APPLICATION_DATA */
            aad[9] = 3;         /* version */
            aad[10] = 2;
            aad[11] = 0;        /* length */
            aad[12] = 0;
            mb_param.out = NULL;
            mb_param.inp = aad;
            mb_param.len = len;
            mb_param.interleave = 8;

            packlen = EVP_CIPHER_CTX_ctrl(&ctx,
                                          EVP_CTRL_TLS1_1_MULTIBLOCK_AAD,
                                          sizeof(mb_param), &mb_param);

            if (packlen > 0) {
                mb_param.out = out;
                mb_param.inp = inp;
                mb_param.len = len;
                EVP_CIPHER_CTX_ctrl(&ctx,
                                    EVP_CTRL_TLS1_1_MULTIBLOCK_ENCRYPT,
                                    sizeof(mb_param), &mb_param);
            } else {
                int pad;

                RAND_bytes(out, 16);
                len += 16;
                aad[11] = len >> 8;
                aad[12] = len;
                pad = EVP_CIPHER_CTX_ctrl(&ctx,
                                          EVP_CTRL_AEAD_TLS1_AAD, 13, aad);
                EVP_Cipher(&ctx, out, inp, len + pad);
            }
        }
        d = Time_F(STOP);
        BIO_printf(bio_err,
                   mr ? "+R:%d:%s:%f\n"
                   : "%d %s's in %.2fs\n", count, "evp", d);
        results[D_EVP][j] = ((double)count) / d * mblengths[j];
    }

    if (mr) {
        fprintf(stdout, "+H");
        for (j = 0; j < num; j++)
            fprintf(stdout, ":%d", mblengths[j]);
        fprintf(stdout, "\n");
        fprintf(stdout, "+F:%d:%s", D_EVP, alg_name);
        for (j = 0; j < num; j++)
            fprintf(stdout, ":%.2f", results[D_EVP][j]);
        fprintf(stdout, "\n");
    } else {
        fprintf(stdout,
                "The 'numbers' are in 1000s of bytes per second processed.\n");
        fprintf(stdout, "type                    ");
        for (j = 0; j < num; j++)
            fprintf(stdout, "%7d bytes", mblengths[j]);
        fprintf(stdout, "\n");
        fprintf(stdout, "%-24s", alg_name);

        for (j = 0; j < num; j++) {
            if (results[D_EVP][j] > 10000)
                fprintf(stdout, " %11.2fk", results[D_EVP][j] / 1e3);
            else
                fprintf(stdout, " %11.2f ", results[D_EVP][j]);
        }
        fprintf(stdout, "\n");
    }

end:
    if (inp)
        OPENSSL_free(inp);
    if (out)
        OPENSSL_free(out);
}