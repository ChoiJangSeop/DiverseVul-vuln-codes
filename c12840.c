sixel_allocator_malloc(
    sixel_allocator_t   /* in */ *allocator,  /* allocator object */
    size_t              /* in */ n)           /* allocation size */
{
    /* precondition */
    assert(allocator);
    assert(allocator->fn_malloc);

    if (n == 0) {
        sixel_helper_set_additional_message(
            "sixel_allocator_malloc: called with n == 0");
        return NULL;
    }
    return allocator->fn_malloc(n);
}