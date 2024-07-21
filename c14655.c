static int wc_ecc_shared_secret_gen_sync(ecc_key* private_key, ecc_point* point,
                               byte* out, word32* outlen, ecc_curve_spec* curve)
{
    int err;
#ifndef WOLFSSL_SP_MATH
    ecc_point* result = NULL;
    word32 x = 0;
#endif
    mp_int* k = &private_key->k;
#ifdef HAVE_ECC_CDH
    mp_int k_lcl;

    /* if cofactor flag has been set */
    if (private_key->flags & WC_ECC_FLAG_COFACTOR) {
        mp_digit cofactor = (mp_digit)private_key->dp->cofactor;
        /* only perform cofactor calc if not equal to 1 */
        if (cofactor != 1) {
            k = &k_lcl;
            if (mp_init(k) != MP_OKAY)
                return MEMORY_E;
            /* multiply cofactor times private key "k" */
            err = mp_mul_d(&private_key->k, cofactor, k);
            if (err != MP_OKAY) {
                mp_clear(k);
                return err;
            }
        }
    }
#endif

#ifdef WOLFSSL_HAVE_SP_ECC
#ifndef WOLFSSL_SP_NO_256
    if (private_key->idx != ECC_CUSTOM_IDX &&
                               ecc_sets[private_key->idx].id == ECC_SECP256R1) {
        err = sp_ecc_secret_gen_256(k, point, out, outlen, private_key->heap);
    }
    else
#endif
#ifdef WOLFSSL_SP_384
    if (private_key->idx != ECC_CUSTOM_IDX &&
                               ecc_sets[private_key->idx].id == ECC_SECP384R1) {
        err = sp_ecc_secret_gen_384(k, point, out, outlen, private_key->heap);
    }
    else
#endif
#endif
#ifdef WOLFSSL_SP_MATH
    {
        err = WC_KEY_SIZE_E;

        (void)curve;
    }
#else
    {
        /* make new point */
        result = wc_ecc_new_point_h(private_key->heap);
        if (result == NULL) {
#ifdef HAVE_ECC_CDH
            if (k == &k_lcl)
                mp_clear(k);
#endif
            return MEMORY_E;
        }

        err = wc_ecc_mulmod_ex(k, point, result, curve->Af, curve->prime, 1,
                                                             private_key->heap);
        if (err == MP_OKAY) {
            x = mp_unsigned_bin_size(curve->prime);
            if (*outlen < x || (int)x < mp_unsigned_bin_size(result->x)) {
                err = BUFFER_E;
            }
        }

        if (err == MP_OKAY) {
            XMEMSET(out, 0, x);
            err = mp_to_unsigned_bin(result->x,out +
                                     (x - mp_unsigned_bin_size(result->x)));
        }
        *outlen = x;

        wc_ecc_del_point_h(result, private_key->heap);
    }
#endif
#ifdef HAVE_ECC_CDH
    if (k == &k_lcl)
        mp_clear(k);
#endif

    return err;
}