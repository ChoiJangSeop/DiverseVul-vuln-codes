njs_typed_array_set_value(njs_vm_t *vm, njs_typed_array_t *array,
    uint32_t index, njs_value_t *setval)
{
    double              num;
    njs_int_t           ret;
    njs_array_buffer_t  *buffer;

    ret = njs_value_to_number(vm, setval, &num);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    buffer = njs_typed_array_writable(vm, array);
    if (njs_slow_path(buffer == NULL)) {
        return NJS_ERROR;
    }

    njs_typed_array_prop_set(vm, array, index, num);

    njs_set_number(setval, num);

    return NJS_OK;
}