int x9_62_test_internal(BIO *out, int nid, const char *r_in, const char *s_in)
{
    int ret = 0;
    const char message[] = "abc";
    unsigned char digest[20];
    unsigned int dgst_len = 0;
    EVP_MD_CTX md_ctx;
    EC_KEY *key = NULL;
    ECDSA_SIG *signature = NULL;
    BIGNUM *r = NULL, *s = NULL;

    EVP_MD_CTX_init(&md_ctx);
    /* get the message digest */
    EVP_DigestInit(&md_ctx, EVP_ecdsa());
    EVP_DigestUpdate(&md_ctx, (const void *)message, 3);
    EVP_DigestFinal(&md_ctx, digest, &dgst_len);

    BIO_printf(out, "testing %s: ", OBJ_nid2sn(nid));
    /* create the key */
    if ((key = EC_KEY_new_by_curve_name(nid)) == NULL)
        goto x962_int_err;
    if (!EC_KEY_generate_key(key))
        goto x962_int_err;
    BIO_printf(out, ".");
    (void)BIO_flush(out);
    /* create the signature */
    signature = ECDSA_do_sign(digest, 20, key);
    if (signature == NULL)
        goto x962_int_err;
    BIO_printf(out, ".");
    (void)BIO_flush(out);
    /* compare the created signature with the expected signature */
    if ((r = BN_new()) == NULL || (s = BN_new()) == NULL)
        goto x962_int_err;
    if (!BN_dec2bn(&r, r_in) || !BN_dec2bn(&s, s_in))
        goto x962_int_err;
    if (BN_cmp(signature->r, r) || BN_cmp(signature->s, s))
        goto x962_int_err;
    BIO_printf(out, ".");
    (void)BIO_flush(out);
    /* verify the signature */
    if (ECDSA_do_verify(digest, 20, signature, key) != 1)
        goto x962_int_err;
    BIO_printf(out, ".");
    (void)BIO_flush(out);

    BIO_printf(out, " ok\n");
    ret = 1;
 x962_int_err:
    if (!ret)
        BIO_printf(out, " failed\n");
    if (key)
        EC_KEY_free(key);
    if (signature)
        ECDSA_SIG_free(signature);
    if (r)
        BN_free(r);
    if (s)
        BN_free(s);
    EVP_MD_CTX_cleanup(&md_ctx);
    return ret;
}