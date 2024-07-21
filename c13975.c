njs_array_expand(njs_vm_t *vm, njs_array_t *array, uint32_t prepend,
    uint32_t append)
{
    uint32_t     free_before, free_after;
    uint64_t     size;
    njs_value_t  *start, *old;

    free_before = array->start - array->data;
    free_after = array->size - array->length - free_before;

    if (njs_fast_path(free_before >= prepend && free_after >= append)) {
        return NJS_OK;
    }

    size = (uint64_t) prepend + array->length + append;

    if (size < 16) {
        size *= 2;

    } else {
        size += size / 2;
    }

    if (njs_slow_path(size > (UINT32_MAX / sizeof(njs_value_t)))) {
        goto memory_error;
    }

    start = njs_mp_align(vm->mem_pool, sizeof(njs_value_t),
                         size * sizeof(njs_value_t));
    if (njs_slow_path(start == NULL)) {
        goto memory_error;
    }

    array->size = size;

    old = array->data;
    array->data = start;
    start += prepend;

    if (array->length != 0) {
        memcpy(start, array->start, array->length * sizeof(njs_value_t));
    }

    array->start = start;

    njs_mp_free(vm->mem_pool, old);

    return NJS_OK;

memory_error:

    njs_memory_error(vm);

    return NJS_ERROR;
}