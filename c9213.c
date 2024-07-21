usm_malloc_usmStateReference(void)
{
    struct usmStateReference *retval = (struct usmStateReference *)
        calloc(1, sizeof(struct usmStateReference));

    return retval;
}                               /* end usm_malloc_usmStateReference() */