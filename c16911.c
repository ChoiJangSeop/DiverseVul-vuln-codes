static ECDSA_SIG *ecdsa_do_sign(const unsigned char *dgst, int dgst_len,
                                const BIGNUM *in_kinv, const BIGNUM *in_r,
                                EC_KEY *eckey)
{
    int ok = 0, i;
    BIGNUM *kinv = NULL, *s, *m = NULL, *tmp = NULL, *order = NULL;
    const BIGNUM *ckinv;
    BN_CTX *ctx = NULL;
    const EC_GROUP *group;
    ECDSA_SIG *ret;
    ECDSA_DATA *ecdsa;
    const BIGNUM *priv_key;

    ecdsa = ecdsa_check(eckey);
    group = EC_KEY_get0_group(eckey);
    priv_key = EC_KEY_get0_private_key(eckey);

    if (group == NULL || priv_key == NULL || ecdsa == NULL) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_PASSED_NULL_PARAMETER);
        return NULL;
    }

    ret = ECDSA_SIG_new();
    if (!ret) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_MALLOC_FAILURE);
        return NULL;
    }
    s = ret->s;

    if ((ctx = BN_CTX_new()) == NULL || (order = BN_new()) == NULL ||
        (tmp = BN_new()) == NULL || (m = BN_new()) == NULL) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    if (!EC_GROUP_get_order(group, order, ctx)) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_EC_LIB);
        goto err;
    }
    i = BN_num_bits(order);
    /*
     * Need to truncate digest if it is too long: first truncate whole bytes.
     */
    if (8 * dgst_len > i)
        dgst_len = (i + 7) / 8;
    if (!BN_bin2bn(dgst, dgst_len, m)) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_BN_LIB);
        goto err;
    }
    /* If still too long truncate remaining bits with a shift */
    if ((8 * dgst_len > i) && !BN_rshift(m, m, 8 - (i & 0x7))) {
        ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_BN_LIB);
        goto err;
    }
    do {
        if (in_kinv == NULL || in_r == NULL) {
            if (!ECDSA_sign_setup(eckey, ctx, &kinv, &ret->r)) {
                ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_ECDSA_LIB);
                goto err;
            }
            ckinv = kinv;
        } else {
            ckinv = in_kinv;
            if (BN_copy(ret->r, in_r) == NULL) {
                ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_MALLOC_FAILURE);
                goto err;
            }
        }

        if (!BN_mod_mul(tmp, priv_key, ret->r, order, ctx)) {
            ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_BN_LIB);
            goto err;
        }
        if (!BN_mod_add_quick(s, tmp, m, order)) {
            ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_BN_LIB);
            goto err;
        }
        if (!BN_mod_mul(s, s, ckinv, order, ctx)) {
            ECDSAerr(ECDSA_F_ECDSA_DO_SIGN, ERR_R_BN_LIB);
            goto err;
        }
        if (BN_is_zero(s)) {
            /*
             * if kinv and r have been supplied by the caller don't to
             * generate new kinv and r values
             */
            if (in_kinv != NULL && in_r != NULL) {
                ECDSAerr(ECDSA_F_ECDSA_DO_SIGN,
                         ECDSA_R_NEED_NEW_SETUP_VALUES);
                goto err;
            }
        } else
            /* s != 0 => we have a valid signature */
            break;
    }
    while (1);

    ok = 1;
 err:
    if (!ok) {
        ECDSA_SIG_free(ret);
        ret = NULL;
    }
    if (ctx)
        BN_CTX_free(ctx);
    if (m)
        BN_clear_free(m);
    if (tmp)
        BN_clear_free(tmp);
    if (order)
        BN_free(order);
    if (kinv)
        BN_clear_free(kinv);
    return ret;
}