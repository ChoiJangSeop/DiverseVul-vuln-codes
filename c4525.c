STATIC ptr_t GC_alloc_large_and_clear(size_t lb, int k, unsigned flags)
{
    ptr_t result;
    word n_blocks = OBJ_SZ_TO_BLOCKS(lb);

    GC_ASSERT(I_HOLD_LOCK());
    result = GC_alloc_large(lb, k, flags);
    if (result != NULL
          && (GC_debugging_started || GC_obj_kinds[k].ok_init)) {
        /* Clear the whole block, in case of GC_realloc call. */
        BZERO(result, n_blocks * HBLKSIZE);
    }
    return result;
}