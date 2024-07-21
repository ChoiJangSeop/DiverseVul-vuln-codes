apr_status_t h2_push_diary_digest_get(h2_push_diary *diary, apr_pool_t *pool, 
                                      int maxP, const char *authority, 
                                      const char **pdata, apr_size_t *plen)
{
    int nelts, N, i;
    unsigned char log2n, log2pmax;
    gset_encoder encoder;
    apr_uint64_t *hashes;
    apr_size_t hash_count;
    
    nelts = diary->entries->nelts;
    
    if ((apr_uint32_t)nelts > APR_UINT32_MAX) {
        /* should not happen */
        return APR_ENOTIMPL;
    }
    N = ceil_power_of_2(nelts);
    log2n = h2_log2(N);
    
    /* Now log2p is the max number of relevant bits, so that
     * log2p + log2n == mask_bits. We can use a lower log2p
     * and have a shorter set encoding...
     */
    log2pmax = h2_log2(ceil_power_of_2(maxP));
    
    memset(&encoder, 0, sizeof(encoder));
    encoder.diary = diary;
    encoder.log2p = H2MIN(diary->mask_bits - log2n, log2pmax);
    encoder.mask_bits = log2n + encoder.log2p;
    encoder.delta_bits = diary->mask_bits - encoder.mask_bits;
    encoder.fixed_bits = encoder.log2p;
    encoder.fixed_mask = 1;
    encoder.fixed_mask = (encoder.fixed_mask << encoder.fixed_bits) - 1;
    encoder.pool = pool;
    encoder.datalen = 512;
    encoder.data = apr_pcalloc(encoder.pool, encoder.datalen);
    
    encoder.data[0] = log2n;
    encoder.data[1] = encoder.log2p;
    encoder.offset = 1;
    encoder.bit = 8;
    encoder.last = 0;
    
    /* Intentional no APLOGNO */
    ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, pool,
                  "h2_push_diary_digest_get: %d entries, N=%d, log2n=%d, "
                  "mask_bits=%d, enc.mask_bits=%d, delta_bits=%d, enc.log2p=%d, authority=%s", 
                  (int)nelts, (int)N, (int)log2n, diary->mask_bits, 
                  (int)encoder.mask_bits, (int)encoder.delta_bits, 
                  (int)encoder.log2p, authority);
                  
    if (!authority || !diary->authority 
        || !strcmp("*", authority) || !strcmp(diary->authority, authority)) {
        hash_count = diary->entries->nelts;
        hashes = apr_pcalloc(encoder.pool, hash_count);
        for (i = 0; i < hash_count; ++i) {
            hashes[i] = ((&APR_ARRAY_IDX(diary->entries, i, h2_push_diary_entry))->hash 
                         >> encoder.delta_bits);
        }
        
        qsort(hashes, hash_count, sizeof(apr_uint64_t), cmp_puint64);
        for (i = 0; i < hash_count; ++i) {
            if (!i || (hashes[i] != hashes[i-1])) {
                gset_encode_next(&encoder, hashes[i]);
            }
        }
        /* Intentional no APLOGNO */
        ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, pool,
                      "h2_push_diary_digest_get: golomb compressed hashes, %d bytes",
                      (int)encoder.offset + 1);
    }
    *pdata = (const char *)encoder.data;
    *plen = encoder.offset + 1;
    
    return APR_SUCCESS;
}