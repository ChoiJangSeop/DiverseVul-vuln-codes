sixel_allocator_realloc(
    sixel_allocator_t   /* in */ *allocator,  /* allocator object */
    void                /* in */ *p,          /* existing buffer to be re-allocated */
    size_t              /* in */ n)           /* re-allocation size */
{
    /* precondition */
    assert(allocator);
    assert(allocator->fn_realloc);

    return allocator->fn_realloc(p, n);
}