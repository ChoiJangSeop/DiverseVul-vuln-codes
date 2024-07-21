int uECC_verify(const uint8_t *public_key,
                const uint8_t *message_hash,
                unsigned hash_size,
                const uint8_t *signature,
                uECC_Curve curve) {
    uECC_word_t u1[uECC_MAX_WORDS], u2[uECC_MAX_WORDS];
    uECC_word_t z[uECC_MAX_WORDS];
    uECC_word_t sum[uECC_MAX_WORDS * 2];
    uECC_word_t rx[uECC_MAX_WORDS];
    uECC_word_t ry[uECC_MAX_WORDS];
    uECC_word_t tx[uECC_MAX_WORDS];
    uECC_word_t ty[uECC_MAX_WORDS];
    uECC_word_t tz[uECC_MAX_WORDS];
    const uECC_word_t *points[4];
    const uECC_word_t *point;
    bitcount_t num_bits;
    bitcount_t i;
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *_public = (uECC_word_t *)public_key;
#else
    uECC_word_t _public[uECC_MAX_WORDS * 2];
#endif    
    uECC_word_t r[uECC_MAX_WORDS], s[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);

    rx[num_n_words - 1] = 0;
    r[num_n_words - 1] = 0;
    s[num_n_words - 1] = 0;

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) r, signature, curve->num_bytes);
    bcopy((uint8_t *) s, signature + curve->num_bytes, curve->num_bytes);
#else
    uECC_vli_bytesToNative(_public, public_key, curve->num_bytes);
    uECC_vli_bytesToNative(
        _public + num_words, public_key + curve->num_bytes, curve->num_bytes);
    uECC_vli_bytesToNative(r, signature, curve->num_bytes);
    uECC_vli_bytesToNative(s, signature + curve->num_bytes, curve->num_bytes);
#endif

    /* r, s must not be 0. */
    if (uECC_vli_isZero(r, num_words) || uECC_vli_isZero(s, num_words)) {
        return 0;
    }

    /* r, s must be < n. */
    if (uECC_vli_cmp_unsafe(curve->n, r, num_n_words) != 1 ||
            uECC_vli_cmp_unsafe(curve->n, s, num_n_words) != 1) {
        return 0;
    }

    /* Calculate u1 and u2. */
    uECC_vli_modInv(z, s, curve->n, num_n_words); /* z = 1/s */
    u1[num_n_words - 1] = 0;
    bits2int(u1, message_hash, hash_size, curve);
    uECC_vli_modMult(u1, u1, z, curve->n, num_n_words); /* u1 = e/s */
    uECC_vli_modMult(u2, r, z, curve->n, num_n_words); /* u2 = r/s */

    /* Calculate sum = G + Q. */
    uECC_vli_set(sum, _public, num_words);
    uECC_vli_set(sum + num_words, _public + num_words, num_words);
    uECC_vli_set(tx, curve->G, num_words);
    uECC_vli_set(ty, curve->G + num_words, num_words);
    uECC_vli_modSub(z, sum, tx, curve->p, num_words); /* z = x2 - x1 */
    XYcZ_add(tx, ty, sum, sum + num_words, curve);
    uECC_vli_modInv(z, z, curve->p, num_words); /* z = 1/z */
    apply_z(sum, sum + num_words, z, curve);

    /* Use Shamir's trick to calculate u1*G + u2*Q */
    points[0] = 0;
    points[1] = curve->G;
    points[2] = _public;
    points[3] = sum;
    num_bits = smax(uECC_vli_numBits(u1, num_n_words),
                    uECC_vli_numBits(u2, num_n_words));

    point = points[(!!uECC_vli_testBit(u1, num_bits - 1)) |
                   ((!!uECC_vli_testBit(u2, num_bits - 1)) << 1)];
    uECC_vli_set(rx, point, num_words);
    uECC_vli_set(ry, point + num_words, num_words);
    uECC_vli_clear(z, num_words);
    z[0] = 1;

    for (i = num_bits - 2; i >= 0; --i) {
        uECC_word_t index;
        curve->double_jacobian(rx, ry, z, curve);

        index = (!!uECC_vli_testBit(u1, i)) | ((!!uECC_vli_testBit(u2, i)) << 1);
        point = points[index];
        if (point) {
            uECC_vli_set(tx, point, num_words);
            uECC_vli_set(ty, point + num_words, num_words);
            apply_z(tx, ty, z, curve);
            uECC_vli_modSub(tz, rx, tx, curve->p, num_words); /* Z = x2 - x1 */
            XYcZ_add(tx, ty, rx, ry, curve);
            uECC_vli_modMult_fast(z, z, tz, curve);
        }
    }

    uECC_vli_modInv(z, z, curve->p, num_words); /* Z = 1/Z */
    apply_z(rx, ry, z, curve);

    /* v = x1 (mod n) */
    if (uECC_vli_cmp_unsafe(curve->n, rx, num_n_words) != 1) {
        uECC_vli_sub(rx, rx, curve->n, num_n_words);
    }

    /* Accept only if v == r. */
    return (int)(uECC_vli_equal(rx, r, num_words));
}