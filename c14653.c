static int wc_ecc_make_pub_ex(ecc_key* key, ecc_curve_spec* curveIn,
        ecc_point* pubOut)
{
    int err = MP_OKAY;
#ifndef WOLFSSL_ATECC508A
#ifndef WOLFSSL_SP_MATH
    ecc_point* base = NULL;
#endif
    ecc_point* pub;
    DECLARE_CURVE_SPECS(curve, ECC_CURVE_FIELD_COUNT);
#endif /* !WOLFSSL_ATECC508A */

    if (key == NULL) {
        return BAD_FUNC_ARG;
    }

#ifndef WOLFSSL_ATECC508A

    /* if ecc_point passed in then use it as output for public key point */
    if (pubOut != NULL) {
        pub = pubOut;
    }
    else {
        /* caching public key making it a ECC_PRIVATEKEY instead of
           ECC_PRIVATEKEY_ONLY */
        pub = &key->pubkey;
        key->type = ECC_PRIVATEKEY_ONLY;
    }

    /* avoid loading the curve unless it is not passed in */
    if (curveIn != NULL) {
        curve = curveIn;
    }
    else {
        /* load curve info */
        if (err == MP_OKAY) {
            ALLOC_CURVE_SPECS(ECC_CURVE_FIELD_COUNT);
            err = wc_ecc_curve_load(key->dp, &curve, ECC_CURVE_FIELD_ALL);
        }
    }

    if (err == MP_OKAY) {
    #ifndef ALT_ECC_SIZE
        err = mp_init_multi(pub->x, pub->y, pub->z, NULL, NULL, NULL);
    #else
        pub->x = (mp_int*)&pub->xyz[0];
        pub->y = (mp_int*)&pub->xyz[1];
        pub->z = (mp_int*)&pub->xyz[2];
        alt_fp_init(pub->x);
        alt_fp_init(pub->y);
        alt_fp_init(pub->z);
    #endif
    }


#ifdef WOLFSSL_HAVE_SP_ECC
#ifndef WOLFSSL_SP_NO_256
    if (key->idx != ECC_CUSTOM_IDX && ecc_sets[key->idx].id == ECC_SECP256R1) {
        if (err == MP_OKAY)
            err = sp_ecc_mulmod_base_256(&key->k, pub, 1, key->heap);
    }
    else
#endif
#ifdef WOLFSSL_SP_384
    if (key->idx != ECC_CUSTOM_IDX && ecc_sets[key->idx].id == ECC_SECP384R1) {
        if (err == MP_OKAY)
            err = sp_ecc_mulmod_base_384(&key->k, pub, 1, key->heap);
    }
    else
#endif
#endif
#ifdef WOLFSSL_SP_MATH
        err = WC_KEY_SIZE_E;
#else
    {
        if (err == MP_OKAY) {
            base = wc_ecc_new_point_h(key->heap);
            if (base == NULL)
                err = MEMORY_E;
        }
        /* read in the x/y for this key */
        if (err == MP_OKAY)
            err = mp_copy(curve->Gx, base->x);
        if (err == MP_OKAY)
            err = mp_copy(curve->Gy, base->y);
        if (err == MP_OKAY)
            err = mp_set(base->z, 1);

        /* make the public key */
        if (err == MP_OKAY) {
            err = wc_ecc_mulmod_ex(&key->k, base, pub, curve->Af, curve->prime,
                                                                  1, key->heap);
            if (err == MP_MEM) {
               err = MEMORY_E;
            }
        }

        wc_ecc_del_point_h(base, key->heap);
    }
#endif

#ifdef WOLFSSL_VALIDATE_ECC_KEYGEN
    /* validate the public key, order * pubkey = point at infinity */
    if (err == MP_OKAY)
        err = ecc_check_pubkey_order(key, pub, curve->Af, curve->prime,
                curve->order);
#endif /* WOLFSSL_VALIDATE_KEYGEN */

    if (err != MP_OKAY) {
        /* clean up if failed */
    #ifndef ALT_ECC_SIZE
        mp_clear(pub->x);
        mp_clear(pub->y);
        mp_clear(pub->z);
    #endif
    }

    /* free up local curve */
    if (curveIn == NULL) {
        wc_ecc_curve_free(curve);
        FREE_CURVE_SPECS();
    }

#else
    (void)curveIn;
    err = NOT_COMPILED_IN;
#endif /* WOLFSSL_ATECC508A */

    /* change key state if public part is cached */
    if (key->type == ECC_PRIVATEKEY_ONLY && pubOut == NULL) {
        key->type = ECC_PRIVATEKEY;
    }

    return err;
}