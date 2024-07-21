njs_iterator_to_array_handler(njs_vm_t *vm, njs_iterator_args_t *args,
    njs_value_t *value, int64_t index)
{
    njs_array_t  *array;

    array = args->data;
    array->start[index] = *value;

    return NJS_OK;
}