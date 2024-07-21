njs_array_convert_to_slow_array(njs_vm_t *vm, njs_array_t *array)
{
    uint32_t           i, length;
    njs_value_t        index, value;
    njs_object_prop_t  *prop;

    njs_set_array(&value, array);
    array->object.fast_array = 0;

    length = array->length;

    for (i = 0; i < length; i++) {
        if (njs_is_valid(&array->start[i])) {
            njs_uint32_to_string(&index, i);
            prop = njs_object_property_add(vm, &value, &index, 0);
            if (njs_slow_path(prop == NULL)) {
                return NJS_ERROR;
            }

            prop->value = array->start[i];
        }
    }

    /* GC: release value. */

    njs_mp_free(vm->mem_pool, array->start);
    array->start = NULL;

    return NJS_OK;
}