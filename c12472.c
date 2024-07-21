static void bits2int(uECC_word_t *native,
                     const uint8_t *bits,
                     unsigned bits_size,
                     uECC_Curve curve) {
    unsigned num_n_bytes = BITS_TO_BYTES(curve->num_n_bits);
    unsigned num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    int shift;
    uECC_word_t carry;
    uECC_word_t *ptr;

    if (bits_size > num_n_bytes) {
        bits_size = num_n_bytes;
    }

    uECC_vli_clear(native, num_n_words);
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) native, bits, bits_size);
#else
    uECC_vli_bytesToNative(native, bits, bits_size);
#endif    
    if (bits_size * 8 <= (unsigned)curve->num_n_bits) {
        return;
    }
    shift = bits_size * 8 - curve->num_n_bits;
    carry = 0;
    ptr = native + num_n_words;
    while (ptr-- > native) {
        uECC_word_t temp = *ptr;
        *ptr = (temp >> shift) | carry;
        carry = temp << (uECC_WORD_BITS - shift);
    }

    /* Reduce mod curve_n */
    if (uECC_vli_cmp_unsafe(curve->n, native, num_n_words) != 1) {
        uECC_vli_sub(native, native, curve->n, num_n_words);
    }
}