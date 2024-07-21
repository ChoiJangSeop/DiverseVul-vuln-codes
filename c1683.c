item *do_item_get(const char *key, const size_t nkey, const uint32_t hv) {
    //mutex_lock(&cache_lock);
    item *it = assoc_find(key, nkey, hv);
    if (it != NULL) {
        refcount_incr(&it->refcount);
        /* Optimization for slab reassignment. prevents popular items from
         * jamming in busy wait. Can only do this here to satisfy lock order
         * of item_lock, cache_lock, slabs_lock. */
        if (slab_rebalance_signal &&
            ((void *)it >= slab_rebal.slab_start && (void *)it < slab_rebal.slab_end)) {
            do_item_unlink_nolock(it, hv);
            do_item_remove(it);
            it = NULL;
        }
    }
    //mutex_unlock(&cache_lock);
    int was_found = 0;

    if (settings.verbose > 2) {
        if (it == NULL) {
            fprintf(stderr, "> NOT FOUND %s", key);
        } else {
            fprintf(stderr, "> FOUND KEY %s", ITEM_key(it));
            was_found++;
        }
    }

    if (it != NULL) {
        if (settings.oldest_live != 0 && settings.oldest_live <= current_time &&
            it->time <= settings.oldest_live) {
            do_item_unlink(it, hv);
            do_item_remove(it);
            it = NULL;
            if (was_found) {
                fprintf(stderr, " -nuked by flush");
            }
        } else if (it->exptime != 0 && it->exptime <= current_time) {
            do_item_unlink(it, hv);
            do_item_remove(it);
            it = NULL;
            if (was_found) {
                fprintf(stderr, " -nuked by expire");
            }
        } else {
            it->it_flags |= ITEM_FETCHED;
            DEBUG_REFCNT(it, '+');
        }
    }

    if (settings.verbose > 2)
        fprintf(stderr, "\n");

    return it;
}