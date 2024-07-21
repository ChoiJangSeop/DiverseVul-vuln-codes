GC_INNER void GC_remove_protection(struct hblk *h, word nblocks,
                                   GC_bool is_ptrfree)
{
    struct hblk * h_trunc;  /* Truncated to page boundary */
    struct hblk * h_end;    /* Page boundary following block end */
    struct hblk * current;

#   if defined(GWW_VDB)
      if (GC_GWW_AVAILABLE()) return;
#   endif
    if (!GC_dirty_maintained) return;
    h_trunc = (struct hblk *)((word)h & ~(GC_page_size-1));
    h_end = (struct hblk *)ROUNDUP_PAGESIZE((word)(h + nblocks));
    if (h_end == h_trunc + 1 &&
        get_pht_entry_from_index(GC_dirty_pages, PHT_HASH(h_trunc))) {
        /* already marked dirty, and hence unprotected. */
        return;
    }
    for (current = h_trunc; (word)current < (word)h_end; ++current) {
        size_t index = PHT_HASH(current);
        if (!is_ptrfree || (word)current < (word)h
            || (word)current >= (word)(h + nblocks)) {
            async_set_pht_entry_from_index(GC_dirty_pages, index);
        }
    }
    UNPROTECT(h_trunc, (ptr_t)h_end - (ptr_t)h_trunc);
}