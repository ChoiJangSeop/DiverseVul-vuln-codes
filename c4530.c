GC_allochblk(size_t sz, int kind, unsigned flags/* IGNORE_OFF_PAGE or 0 */)
{
    word blocks;
    int start_list;
    struct hblk *result;
    int may_split;
    int split_limit; /* Highest index of free list whose blocks we      */
                     /* split.                                          */

    GC_ASSERT((sz & (GRANULE_BYTES - 1)) == 0);
    blocks = OBJ_SZ_TO_BLOCKS(sz);
    if ((signed_word)(blocks * HBLKSIZE) < 0) {
      return 0;
    }
    start_list = GC_hblk_fl_from_blocks(blocks);
    /* Try for an exact match first. */
    result = GC_allochblk_nth(sz, kind, flags, start_list, FALSE);
    if (0 != result) return result;

    may_split = TRUE;
    if (GC_use_entire_heap || GC_dont_gc
        || USED_HEAP_SIZE < GC_requested_heapsize
        || GC_incremental || !GC_should_collect()) {
        /* Should use more of the heap, even if it requires splitting. */
        split_limit = N_HBLK_FLS;
    } else if (GC_finalizer_bytes_freed > (GC_heapsize >> 4)) {
          /* If we are deallocating lots of memory from         */
          /* finalizers, fail and collect sooner rather         */
          /* than later.                                        */
          split_limit = 0;
    } else {
          /* If we have enough large blocks left to cover any   */
          /* previous request for large blocks, we go ahead     */
          /* and split.  Assuming a steady state, that should   */
          /* be safe.  It means that we can use the full        */
          /* heap if we allocate only small objects.            */
          split_limit = GC_enough_large_bytes_left();
#         ifdef USE_MUNMAP
            if (split_limit > 0)
              may_split = AVOID_SPLIT_REMAPPED;
#         endif
    }
    if (start_list < UNIQUE_THRESHOLD) {
      /* No reason to try start_list again, since all blocks are exact  */
      /* matches.                                                       */
      ++start_list;
    }
    for (; start_list <= split_limit; ++start_list) {
        result = GC_allochblk_nth(sz, kind, flags, start_list, may_split);
        if (0 != result)
            break;
    }
    return result;
}