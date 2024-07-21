njs_typed_array_alloc(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_bool_t zeroing, njs_object_type_t type)
{
    double              num;
    int64_t             i, length;
    uint32_t            element_size;
    uint64_t            size, offset;
    njs_int_t           ret;
    njs_value_t         *value, prop;
    njs_array_t         *src_array;
    njs_typed_array_t   *array, *src_tarray;
    njs_array_buffer_t  *buffer;

    size = 0;
    length = 0;
    offset = 0;

    buffer = NULL;
    src_array = NULL;
    src_tarray = NULL;

    element_size = njs_typed_array_element_size(type);

    value = njs_arg(args, nargs, 0);

    if (njs_is_array_buffer(value)) {
        buffer = njs_array_buffer(value);

        ret = njs_value_to_index(vm, njs_arg(args, nargs, 1), &offset);
        if (njs_slow_path(ret != NJS_OK)) {
            return NULL;
        }

        if (njs_slow_path((offset % element_size) != 0)) {
            njs_range_error(vm, "start offset must be multiple of %uD",
                            element_size);
            return NULL;
        }

        if (njs_is_defined(njs_arg(args, nargs, 2))) {
            ret = njs_value_to_index(vm, njs_argument(args, 2), &size);
            if (njs_slow_path(ret != NJS_OK)) {
                return NULL;
            }
        }

        if (njs_slow_path(njs_is_detached_buffer(buffer))) {
            njs_type_error(vm, "detached buffer");
            return NULL;
        }

        if (njs_is_defined(njs_arg(args, nargs, 2))) {
            ret = njs_value_to_index(vm, njs_argument(args, 2), &size);
            if (njs_slow_path(ret != NJS_OK)) {
                return NULL;
            }

            size *= element_size;

            if (njs_slow_path((offset + size) > buffer->size)) {
                njs_range_error(vm, "Invalid typed array length: %uL", size);
                return NULL;
            }

        } else {
            if (njs_slow_path((buffer->size % element_size) != 0)) {
                njs_range_error(vm, "byteLength of buffer must be "
                                "multiple of %uD", element_size);
                return NULL;
            }

            if (offset > buffer->size) {
                njs_range_error(vm, "byteOffset %uL is outside the bound of "
                                "the buffer", offset);
                return NULL;
            }

            size = buffer->size - offset;
        }

    } else if (njs_is_typed_array(value)) {
        src_tarray = njs_typed_array(value);
        if (njs_slow_path(njs_is_detached_buffer(src_tarray->buffer))) {
            njs_type_error(vm, "detached buffer");
            return NULL;
        }

        size = (uint64_t) njs_typed_array_length(src_tarray) * element_size;

    } else if (njs_is_object(value)) {
        if (njs_is_fast_array(value)) {
            src_array = njs_array(value);
            length = src_array->length;

        } else {
            ret = njs_object_length(vm, value, &length);
            if (njs_slow_path(ret == NJS_ERROR)) {
                return NULL;
            }
        }

        size = length * element_size;

    } else {
        ret = njs_value_to_index(vm, value, &size);
        if (njs_slow_path(ret != NJS_OK)) {
            return NULL;
        }

        size *= element_size;
    }

    if (buffer == NULL) {
        buffer = njs_array_buffer_alloc(vm, size, zeroing);
        if (njs_slow_path(buffer == NULL)) {
            return NULL;
        }
    }

    array = njs_mp_zalloc(vm->mem_pool, sizeof(njs_typed_array_t));
    if (njs_slow_path(array == NULL)) {
        goto memory_error;
    }

    array->buffer = buffer;
    array->offset = offset / element_size;
    array->byte_length = size;
    array->type = type;

    if (src_tarray != NULL) {
        if (type != src_tarray->type) {
            length = njs_typed_array_length(src_tarray);
            for (i = 0; i < length; i++) {
                njs_typed_array_prop_set(vm, array, i,
                                         njs_typed_array_prop(src_tarray, i));
            }

        } else {
            memcpy(&buffer->u.u8[0], &src_tarray->buffer->u.u8[0], size);
        }

    } else if (src_array != NULL) {
        for (i = 0; i < length; i++) {
            ret = njs_value_to_number(vm, &src_array->start[i], &num);
            if (njs_slow_path(ret == NJS_ERROR)) {
                return NULL;
            }

            if (ret == NJS_OK) {
                njs_typed_array_prop_set(vm, array, i, num);
            }
        }

    } else if (!njs_is_array_buffer(value) && njs_is_object(value)) {
        for (i = 0; i < length; i++) {
            ret = njs_value_property_i64(vm, value, i, &prop);
            if (njs_slow_path(ret == NJS_ERROR)) {
                return NULL;
            }

            num = NAN;

            if (ret == NJS_OK) {
                ret = njs_value_to_number(vm, &prop, &num);
                if (njs_slow_path(ret == NJS_ERROR)) {
                    return NULL;
                }
            }

            njs_typed_array_prop_set(vm, array, i, num);
        }
    }

    njs_lvlhsh_init(&array->object.hash);
    njs_lvlhsh_init(&array->object.shared_hash);
    array->object.__proto__ = &vm->prototypes[type].object;
    array->object.type = NJS_TYPED_ARRAY;
    array->object.extensible = 1;
    array->object.fast_array = 1;

    return array;

memory_error:

    njs_memory_error(vm);

    return NULL;
}