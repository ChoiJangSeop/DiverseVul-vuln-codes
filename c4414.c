item *do_item_alloc(char *key, const size_t nkey, const unsigned int flags,
                    const rel_time_t exptime, const int nbytes) {
    int i;
    uint8_t nsuffix;
    item *it = NULL;
    char suffix[40];
    size_t ntotal = item_make_header(nkey + 1, flags, nbytes, suffix, &nsuffix);
    if (settings.use_cas) {
        ntotal += sizeof(uint64_t);
    }

    unsigned int id = slabs_clsid(ntotal);
    if (id == 0)
        return 0;

    /* If no memory is available, attempt a direct LRU juggle/eviction */
    /* This is a race in order to simplify lru_pull_tail; in cases where
     * locked items are on the tail, you want them to fall out and cause
     * occasional OOM's, rather than internally work around them.
     * This also gives one fewer code path for slab alloc/free
     */
    /* TODO: if power_largest, try a lot more times? or a number of times
     * based on how many chunks the new object should take up?
     * or based on the size of an object lru_pull_tail() says it evicted?
     * This is a classical GC problem if "large items" are of too varying of
     * sizes. This is actually okay here since the larger the data, the more
     * bandwidth it takes, the more time we can loop in comparison to serving
     * and replacing small items.
     */
    for (i = 0; i < 10; i++) {
        uint64_t total_bytes;
        /* Try to reclaim memory first */
        if (!settings.lru_maintainer_thread) {
            lru_pull_tail(id, COLD_LRU, 0, 0);
        }
        it = slabs_alloc(ntotal, id, &total_bytes, 0);

        if (settings.expirezero_does_not_evict)
            total_bytes -= noexp_lru_size(id);

        if (it == NULL) {
            if (settings.lru_maintainer_thread) {
                lru_pull_tail(id, HOT_LRU, total_bytes, 0);
                lru_pull_tail(id, WARM_LRU, total_bytes, 0);
                if (lru_pull_tail(id, COLD_LRU, total_bytes, LRU_PULL_EVICT) <= 0)
                    break;
            } else {
                if (lru_pull_tail(id, COLD_LRU, 0, LRU_PULL_EVICT) <= 0)
                    break;
            }
        } else {
            break;
        }
    }

    if (i > 0) {
        pthread_mutex_lock(&lru_locks[id]);
        itemstats[id].direct_reclaims += i;
        pthread_mutex_unlock(&lru_locks[id]);
    }

    if (it == NULL) {
        pthread_mutex_lock(&lru_locks[id]);
        itemstats[id].outofmemory++;
        pthread_mutex_unlock(&lru_locks[id]);
        return NULL;
    }

    assert(it->slabs_clsid == 0);
    //assert(it != heads[id]);

    /* Refcount is seeded to 1 by slabs_alloc() */
    it->next = it->prev = 0;

    /* Items are initially loaded into the HOT_LRU. This is '0' but I want at
     * least a note here. Compiler (hopefully?) optimizes this out.
     */
    if (settings.lru_maintainer_thread) {
        if (exptime == 0 && settings.expirezero_does_not_evict) {
            id |= NOEXP_LRU;
        } else {
            id |= HOT_LRU;
        }
    } else {
        /* There is only COLD in compat-mode */
        id |= COLD_LRU;
    }
    it->slabs_clsid = id;

    DEBUG_REFCNT(it, '*');
    it->it_flags |= settings.use_cas ? ITEM_CAS : 0;
    it->nkey = nkey;
    it->nbytes = nbytes;
    memcpy(ITEM_key(it), key, nkey);
    it->exptime = exptime;
    memcpy(ITEM_suffix(it), suffix, (size_t)nsuffix);
    it->nsuffix = nsuffix;

    /* Need to shuffle the pointer stored in h_next into it->data. */
    if (it->it_flags & ITEM_CHUNKED) {
        item_chunk *chunk = (item_chunk *) ITEM_data(it);

        chunk->next = (item_chunk *) it->h_next;
        chunk->prev = 0;
        chunk->head = it;
        /* Need to chain back into the head's chunk */
        chunk->next->prev = chunk;
        chunk->size = chunk->next->size - ((char *)chunk - (char *)it);
        chunk->used = 0;
        assert(chunk->size > 0);
    }
    it->h_next = 0;

    return it;
}