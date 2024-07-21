void *ztryrealloc_usable(void *ptr, size_t size, size_t *usable) {
#ifndef HAVE_MALLOC_SIZE
    void *realptr;
#endif
    size_t oldsize;
    void *newptr;

    /* not allocating anything, just redirect to free. */
    if (size == 0 && ptr != NULL) {
        zfree(ptr);
        if (usable) *usable = 0;
        return NULL;
    }
    /* Not freeing anything, just redirect to malloc. */
    if (ptr == NULL)
        return ztrymalloc_usable(size, usable);

#ifdef HAVE_MALLOC_SIZE
    oldsize = zmalloc_size(ptr);
    newptr = realloc(ptr,size);
    if (newptr == NULL) {
        if (usable) *usable = 0;
        return NULL;
    }

    update_zmalloc_stat_free(oldsize);
    size = zmalloc_size(newptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return newptr;
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((size_t*)realptr);
    newptr = realloc(realptr,size+PREFIX_SIZE);
    if (newptr == NULL) {
        if (usable) *usable = 0;
        return NULL;
    }

    *((size_t*)newptr) = size;
    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return (char*)newptr+PREFIX_SIZE;
#endif
}