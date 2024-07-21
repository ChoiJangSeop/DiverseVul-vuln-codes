STATIC ptr_t GC_unmap_start(ptr_t start, size_t bytes)
{
    ptr_t result = (ptr_t)ROUNDUP_PAGESIZE((word)start);

    if ((word)(result + GC_page_size) > (word)(start + bytes)) return 0;
    return result;
}