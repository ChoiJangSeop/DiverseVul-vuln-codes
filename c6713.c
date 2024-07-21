copy_stack(i_ctx_t *i_ctx_p, const ref_stack_t * pstack, int skip, ref * arr)
{
    uint size = ref_stack_count(pstack) - skip;
    uint save_space = ialloc_space(idmemory);
    int code;

    if (size > 65535)
        size = 65535;
    ialloc_set_space(idmemory, avm_local);
    code = ialloc_ref_array(arr, a_all, size, "copy_stack");
    if (code >= 0)
        code = ref_stack_store(pstack, arr, size, 0, 1, true, idmemory,
                               "copy_stack");
    ialloc_set_space(idmemory, save_space);
    return code;
}