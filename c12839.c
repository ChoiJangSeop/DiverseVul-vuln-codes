sixel_allocator_calloc(
    sixel_allocator_t   /* in */ *allocator,  /* allocator object */
    size_t              /* in */ nelm,        /* number of elements */
    size_t              /* in */ elsize)      /* size of element */
{
    /* precondition */
    assert(allocator);
    assert(allocator->fn_calloc);

    return allocator->fn_calloc(nelm, elsize);
}