int64_t address_space_cache_init(MemoryRegionCache *cache,
                                 AddressSpace *as,
                                 hwaddr addr,
                                 hwaddr len,
                                 bool is_write)
{
    AddressSpaceDispatch *d;
    hwaddr l;
    MemoryRegion *mr;

    assert(len > 0);

    l = len;
    cache->fv = address_space_get_flatview(as);
    d = flatview_to_dispatch(cache->fv);
    cache->mrs = *address_space_translate_internal(d, addr, &cache->xlat, &l, true);

    mr = cache->mrs.mr;
    memory_region_ref(mr);
    if (memory_access_is_direct(mr, is_write)) {
        /* We don't care about the memory attributes here as we're only
         * doing this if we found actual RAM, which behaves the same
         * regardless of attributes; so UNSPECIFIED is fine.
         */
        l = flatview_extend_translation(cache->fv, addr, len, mr,
                                        cache->xlat, l, is_write,
                                        MEMTXATTRS_UNSPECIFIED);
        cache->ptr = qemu_ram_ptr_length(mr->ram_block, cache->xlat, &l, true);
    } else {
        cache->ptr = NULL;
    }

    cache->len = l;
    cache->is_write = is_write;
    return l;
}