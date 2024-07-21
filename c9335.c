static apr_status_t gset_decode_next(gset_decoder *decoder, apr_uint64_t *phash)
{
    apr_uint64_t flex = 0, fixed = 0, delta;
    int i;
    
    /* read 1 bits until we encounter 0, then read log2n(diary-P) bits.
     * On a malformed bit-string, this will not fail, but produce results
     * which are pbly too large. Luckily, the diary will modulo the hash.
     */
    while (1) {
        int bit = gset_decode_next_bit(decoder);
        if (bit == -1) {
            return APR_EINVAL;
        }
        if (!bit) {
            break;
        }
        ++flex;
    }
    
    for (i = 0; i < decoder->log2p; ++i) {
        int bit = gset_decode_next_bit(decoder);
        if (bit == -1) {
            return APR_EINVAL;
        }
        fixed = (fixed << 1) | bit;
    }
    
    delta = (flex << decoder->log2p) | fixed;
    *phash = delta + decoder->last_val;
    decoder->last_val = *phash;
    
    /* Intentional no APLOGNO */
    ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, decoder->pool,
                  "h2_push_diary_digest_dec: val=%"APR_UINT64_T_HEX_FMT", delta=%"
                  APR_UINT64_T_HEX_FMT", flex=%d, fixed=%"APR_UINT64_T_HEX_FMT, 
                  *phash, delta, (int)flex, fixed);
                  
    return APR_SUCCESS;
}