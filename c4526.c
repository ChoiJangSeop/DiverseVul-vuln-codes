GC_INNER ptr_t GC_scratch_alloc(size_t bytes)
{
    ptr_t result = scratch_free_ptr;
    word bytes_to_get;

    bytes = ROUNDUP_GRANULE_SIZE(bytes);
    for (;;) {
        scratch_free_ptr += bytes;
        if ((word)scratch_free_ptr <= (word)GC_scratch_end_ptr) {
            /* Unallocated space of scratch buffer has enough size. */
            return result;
        }

        if (bytes >= MINHINCR * HBLKSIZE) {
            bytes_to_get = ROUNDUP_PAGESIZE_IF_MMAP(bytes);
            result = (ptr_t)GET_MEM(bytes_to_get);
            GC_add_to_our_memory(result, bytes_to_get);
            /* Undo scratch free area pointer update; get memory directly. */
            scratch_free_ptr -= bytes;
            if (result != NULL) {
                /* Update end point of last obtained area (needed only  */
                /* by GC_register_dynamic_libraries for some targets).  */
                GC_scratch_last_end_ptr = result + bytes;
            }
            return result;
        }

        bytes_to_get = ROUNDUP_PAGESIZE_IF_MMAP(MINHINCR * HBLKSIZE);
                                                /* round up for safety */
        result = (ptr_t)GET_MEM(bytes_to_get);
        GC_add_to_our_memory(result, bytes_to_get);
        if (NULL == result) {
            WARN("Out of memory - trying to allocate requested amount"
                 " (%" WARN_PRIdPTR " bytes)...\n", (word)bytes);
            scratch_free_ptr -= bytes; /* Undo free area pointer update */
            bytes_to_get = ROUNDUP_PAGESIZE_IF_MMAP(bytes);
            result = (ptr_t)GET_MEM(bytes_to_get);
            GC_add_to_our_memory(result, bytes_to_get);
            return result;
        }
        /* Update scratch area pointers and retry.      */
        scratch_free_ptr = result;
        GC_scratch_end_ptr = scratch_free_ptr + bytes_to_get;
        GC_scratch_last_end_ptr = GC_scratch_end_ptr;
    }
}