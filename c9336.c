apr_status_t h2_push_diary_digest_set(h2_push_diary *diary, const char *authority, 
                                      const char *data, apr_size_t len)
{
    gset_decoder decoder;
    unsigned char log2n, log2p;
    int N, i;
    apr_pool_t *pool = diary->entries->pool;
    h2_push_diary_entry e;
    apr_status_t status = APR_SUCCESS;
    
    if (len < 2) {
        /* at least this should be there */
        return APR_EINVAL;
    }
    log2n = data[0];
    log2p = data[1];
    diary->mask_bits = log2n + log2p;
    if (diary->mask_bits > 64) {
        /* cannot handle */
        return APR_ENOTIMPL;
    }
    
    /* whatever is in the digest, it replaces the diary entries */
    apr_array_clear(diary->entries);
    if (!authority || !strcmp("*", authority)) {
        diary->authority = NULL;
    }
    else if (!diary->authority || strcmp(diary->authority, authority)) {
        diary->authority = apr_pstrdup(diary->entries->pool, authority);
    }

    N = h2_log2inv(log2n + log2p);

    decoder.diary    = diary;
    decoder.pool     = pool;
    decoder.log2p    = log2p;
    decoder.data     = (const unsigned char*)data;
    decoder.datalen  = len;
    decoder.offset   = 1;
    decoder.bit      = 8;
    decoder.last_val = 0;
    
    diary->N = N;
    /* Determine effective N we use for storage */
    if (!N) {
        /* a totally empty cache digest. someone tells us that she has no
         * entries in the cache at all. Use our own preferences for N+mask 
         */
        diary->N = diary->NMax;
        return APR_SUCCESS;
    }
    else if (N > diary->NMax) {
        /* Store not more than diary is configured to hold. We open us up
         * to DOS attacks otherwise. */
        diary->N = diary->NMax;
    }
    
    /* Intentional no APLOGNO */
    ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, pool,
                  "h2_push_diary_digest_set: N=%d, log2n=%d, "
                  "diary->mask_bits=%d, dec.log2p=%d", 
                  (int)diary->N, (int)log2n, diary->mask_bits, 
                  (int)decoder.log2p);
                  
    for (i = 0; i < diary->N; ++i) {
        if (gset_decode_next(&decoder, &e.hash) != APR_SUCCESS) {
            /* the data may have less than N values */
            break;
        }
        h2_push_diary_append(diary, &e);
    }
    
    /* Intentional no APLOGNO */
    ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, pool,
                  "h2_push_diary_digest_set: diary now with %d entries, mask_bits=%d", 
                  (int)diary->entries->nelts, diary->mask_bits);
    return status;
}