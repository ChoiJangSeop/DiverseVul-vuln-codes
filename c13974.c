njs_array_prototype_concat(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    double       idx;
    int64_t      k, len, length;
    njs_int_t    ret;
    njs_uint_t   i;
    njs_value_t  this, retval, *e;
    njs_array_t  *array, *keys;

    ret = njs_value_to_object(vm, &args[0]);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    /* TODO: ArraySpeciesCreate(). */

    array = njs_array_alloc(vm, 0, 0, NJS_ARRAY_SPARE);
    if (njs_slow_path(array == NULL)) {
        return NJS_ERROR;
    }

    njs_set_array(&this, array);

    len = 0;
    length = 0;

    for (i = 0; i < nargs; i++) {
        e = njs_argument(args, i);

        ret = njs_is_concat_spreadable(vm, e);
        if (njs_slow_path(ret == NJS_ERROR)) {
            return NJS_ERROR;
        }

        if (ret == NJS_OK) {
            ret = njs_object_length(vm, e, &len);
            if (njs_slow_path(ret == NJS_ERROR)) {
                return ret;
            }

            if (njs_slow_path((length + len) > NJS_MAX_LENGTH)) {
                njs_type_error(vm, "Invalid length");
                return NJS_ERROR;
            }

            if (njs_is_fast_array(e) || njs_fast_object(len)) {
                for (k = 0; k < len; k++, length++) {
                    ret = njs_value_property_i64(vm, e, k, &retval);
                    if (njs_slow_path(ret != NJS_OK)) {
                        if (ret == NJS_ERROR) {
                            return NJS_ERROR;
                        }

                        njs_set_invalid(&retval);
                    }

                    ret = njs_array_add(vm, array, &retval);
                    if (njs_slow_path(ret != NJS_OK)) {
                        return NJS_ERROR;
                    }
                }

                continue;
            }

            keys = njs_array_indices(vm, e);
            if (njs_slow_path(keys == NULL)) {
                return NJS_ERROR;
            }

            for (k = 0; k < keys->length; k++) {
                ret = njs_value_property(vm, e, &keys->start[k], &retval);
                if (njs_slow_path(ret == NJS_ERROR)) {
                    return ret;
                }

                if (ret == NJS_OK) {
                    idx = njs_string_to_index(&keys->start[k]) + length;

                    ret = njs_value_property_i64_set(vm, &this, idx, &retval);
                    if (njs_slow_path(ret == NJS_ERROR)) {
                        njs_array_destroy(vm, keys);
                        return ret;
                    }
                }
            }

            njs_array_destroy(vm, keys);

            length += len;

            continue;
        }

        if (njs_slow_path((length + len) >= NJS_MAX_LENGTH)) {
            njs_type_error(vm, "Invalid length");
            return NJS_ERROR;
        }

        ret = njs_value_property_i64_set(vm, &this, length, e);
        if (njs_slow_path(ret == NJS_ERROR)) {
            return ret;
        }

        length++;
    }

    ret = njs_object_length_set(vm, &this, length);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    vm->retval = this;

    return NJS_OK;
}