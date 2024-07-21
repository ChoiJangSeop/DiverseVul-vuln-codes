GC_INNER ptr_t GC_alloc_large(size_t lb, int k, unsigned flags)
{
    struct hblk * h;
    word n_blocks;
    ptr_t result;
    GC_bool retry = FALSE;

    GC_ASSERT(I_HOLD_LOCK());
    lb = ROUNDUP_GRANULE_SIZE(lb);
    n_blocks = OBJ_SZ_TO_BLOCKS(lb);
    if (!EXPECT(GC_is_initialized, TRUE)) {
      DCL_LOCK_STATE;
      UNLOCK(); /* just to unset GC_lock_holder */
      GC_init();
      LOCK();
    }
    /* Do our share of marking work */
        if (GC_incremental && !GC_dont_gc)
            GC_collect_a_little_inner((int)n_blocks);
    h = GC_allochblk(lb, k, flags);
#   ifdef USE_MUNMAP
        if (0 == h) {
            GC_merge_unmapped();
            h = GC_allochblk(lb, k, flags);
        }
#   endif
    while (0 == h && GC_collect_or_expand(n_blocks, flags != 0, retry)) {
        h = GC_allochblk(lb, k, flags);
        retry = TRUE;
    }
    if (h == 0) {
        result = 0;
    } else {
        size_t total_bytes = n_blocks * HBLKSIZE;
        if (n_blocks > 1) {
            GC_large_allocd_bytes += total_bytes;
            if (GC_large_allocd_bytes > GC_max_large_allocd_bytes)
                GC_max_large_allocd_bytes = GC_large_allocd_bytes;
        }
        /* FIXME: Do we need some way to reset GC_max_large_allocd_bytes? */
        result = h -> hb_body;
    }
    return result;
}