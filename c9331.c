static void h2_push_diary_append(h2_push_diary *diary, h2_push_diary_entry *e)
{
    h2_push_diary_entry *ne;
    
    if (diary->entries->nelts < diary->N) {
        /* append a new diary entry at the end */
        APR_ARRAY_PUSH(diary->entries, h2_push_diary_entry) = *e;
        ne = &APR_ARRAY_IDX(diary->entries, diary->entries->nelts-1, h2_push_diary_entry);
    }
    else {
        /* replace content with new digest. keeps memory usage constant once diary is full */
        ne = move_to_last(diary, 0);
        *ne = *e;
    }
    /* Intentional no APLOGNO */
    ap_log_perror(APLOG_MARK, GCSLOG_LEVEL, 0, diary->entries->pool,
                  "push_diary_append: %"APR_UINT64_T_HEX_FMT, ne->hash);
}