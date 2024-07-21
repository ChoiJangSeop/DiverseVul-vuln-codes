njs_iterator_to_array(njs_vm_t *vm, njs_value_t *iterator)
{
    int64_t              length;
    njs_int_t            ret;
    njs_iterator_args_t  args;

    njs_memzero(&args, sizeof(njs_iterator_args_t));

    ret = njs_object_length(vm, iterator, &length);
    if (njs_slow_path(ret != NJS_OK)) {
        return NULL;
    }

    args.data = njs_array_alloc(vm, 1, length, 0);
    if (njs_slow_path(args.data == NULL)) {
        return NULL;
    }

    args.value = iterator;
    args.to = length;

    ret = njs_object_iterate(vm, &args, njs_iterator_to_array_handler);
    if (njs_slow_path(ret == NJS_ERROR)) {
        njs_mp_free(vm->mem_pool, args.data);
        return NULL;
    }

    return args.data;
}