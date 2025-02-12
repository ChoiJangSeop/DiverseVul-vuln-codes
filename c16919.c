static int dsa_sign_setup(DSA *dsa, BN_CTX *ctx_in, BIGNUM **kinvp,
                          BIGNUM **rp)
{
    BN_CTX *ctx;
    BIGNUM k, kq, *K, *kinv = NULL, *r = NULL;
    BIGNUM l, m;
    int ret = 0;
    int q_bits;

    if (!dsa->p || !dsa->q || !dsa->g) {
        DSAerr(DSA_F_DSA_SIGN_SETUP, DSA_R_MISSING_PARAMETERS);
        return 0;
    }

    BN_init(&k);
    BN_init(&kq);
    BN_init(&l);
    BN_init(&m);

    if (ctx_in == NULL) {
        if ((ctx = BN_CTX_new()) == NULL)
            goto err;
    } else
        ctx = ctx_in;

    if ((r = BN_new()) == NULL)
        goto err;

    /* Preallocate space */
    q_bits = BN_num_bits(dsa->q);
    if (!BN_set_bit(&k, q_bits)
        || !BN_set_bit(&l, q_bits)
        || !BN_set_bit(&m, q_bits))
        goto err;

    /* Get random k */
    do
        if (!BN_rand_range(&k, dsa->q))
            goto err;
    while (BN_is_zero(&k));

    if ((dsa->flags & DSA_FLAG_NO_EXP_CONSTTIME) == 0) {
        BN_set_flags(&k, BN_FLG_CONSTTIME);
    }


    if (dsa->flags & DSA_FLAG_CACHE_MONT_P) {
        if (!BN_MONT_CTX_set_locked(&dsa->method_mont_p,
                                    CRYPTO_LOCK_DSA, dsa->p, ctx))
            goto err;
    }

    /* Compute r = (g^k mod p) mod q */

    if ((dsa->flags & DSA_FLAG_NO_EXP_CONSTTIME) == 0) {
        /*
         * We do not want timing information to leak the length of k, so we
         * compute G^k using an equivalent scalar of fixed bit-length.
         *
         * We unconditionally perform both of these additions to prevent a
         * small timing information leakage.  We then choose the sum that is
         * one bit longer than the modulus.
         *
         * TODO: revisit the BN_copy aiming for a memory access agnostic
         * conditional copy.
         */
        if (!BN_add(&l, &k, dsa->q)
            || !BN_add(&m, &l, dsa->q)
            || !BN_copy(&kq, BN_num_bits(&l) > q_bits ? &l : &m))
            goto err;

        BN_set_flags(&kq, BN_FLG_CONSTTIME);

        K = &kq;
    } else {
        K = &k;
    }

    DSA_BN_MOD_EXP(goto err, dsa, r, dsa->g, K, dsa->p, ctx,
                   dsa->method_mont_p);
    if (!BN_mod(r, r, dsa->q, ctx))
        goto err;

    /* Compute  part of 's = inv(k) (m + xr) mod q' */
    if ((kinv = BN_mod_inverse(NULL, &k, dsa->q, ctx)) == NULL)
        goto err;

    if (*kinvp != NULL)
        BN_clear_free(*kinvp);
    *kinvp = kinv;
    kinv = NULL;
    if (*rp != NULL)
        BN_clear_free(*rp);
    *rp = r;
    ret = 1;
 err:
    if (!ret) {
        DSAerr(DSA_F_DSA_SIGN_SETUP, ERR_R_BN_LIB);
        if (r != NULL)
            BN_clear_free(r);
    }
    if (ctx_in == NULL)
        BN_CTX_free(ctx);
    BN_clear_free(&k);
    BN_clear_free(&kq);
    BN_clear_free(&l);
    BN_clear_free(&m);
    return ret;
}