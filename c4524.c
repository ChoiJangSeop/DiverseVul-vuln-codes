ptr_t GC_unix_get_mem(word bytes)
{
# if defined(MMAP_SUPPORTED)
    /* By default, we try both sbrk and mmap, in that order.    */
    static GC_bool sbrk_failed = FALSE;
    ptr_t result = 0;

    if (!sbrk_failed) result = GC_unix_sbrk_get_mem(bytes);
    if (0 == result) {
        sbrk_failed = TRUE;
        result = GC_unix_mmap_get_mem(bytes);
    }
    if (0 == result) {
        /* Try sbrk again, in case sbrk memory became available.        */
        result = GC_unix_sbrk_get_mem(bytes);
    }
    return result;
# else /* !MMAP_SUPPORTED */
    return GC_unix_sbrk_get_mem(bytes);
# endif
}