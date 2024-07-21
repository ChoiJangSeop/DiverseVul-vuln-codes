njs_object_prop_define(njs_vm_t *vm, njs_value_t *object,
    njs_value_t *name, njs_value_t *value, njs_object_prop_define_t type)
{
    uint32_t              length;
    njs_int_t             ret;
    njs_array_t           *array;
    njs_object_prop_t     *prop, *prev;
    njs_property_query_t  pq;

    static const njs_str_t  length_key = njs_str("length");

    if (njs_slow_path(!njs_is_key(name))) {
        ret = njs_value_to_key(vm, name, name);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

again:

    njs_property_query_init(&pq, NJS_PROPERTY_QUERY_SET, 1);

    ret = njs_property_query(vm, &pq, object, name);
    if (njs_slow_path(ret == NJS_ERROR)) {
        return ret;
    }

    prop = njs_object_prop_alloc(vm, name, &njs_value_invalid,
                                 NJS_ATTRIBUTE_UNSET);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    switch (type) {

    case NJS_OBJECT_PROP_DESCRIPTOR:
        if (njs_descriptor_prop(vm, prop, value) != NJS_OK) {
            return NJS_ERROR;
        }

        break;

    case NJS_OBJECT_PROP_GETTER:
        prop->getter = *value;
        njs_set_invalid(&prop->setter);
        prop->enumerable = NJS_ATTRIBUTE_TRUE;
        prop->configurable = NJS_ATTRIBUTE_TRUE;

        break;

    case NJS_OBJECT_PROP_SETTER:
        prop->setter = *value;
        njs_set_invalid(&prop->getter);
        prop->enumerable = NJS_ATTRIBUTE_TRUE;
        prop->configurable = NJS_ATTRIBUTE_TRUE;

        break;
    }

    if (njs_fast_path(ret == NJS_DECLINED)) {

set_prop:

        if (!njs_object(object)->extensible) {
            njs_key_string_get(vm, &pq.key,  &pq.lhq.key);
            njs_type_error(vm, "Cannot add property \"%V\", "
                           "object is not extensible", &pq.lhq.key);
            return NJS_ERROR;
        }

        if (njs_slow_path(njs_is_typed_array(object)
                          && njs_is_string(name)))
        {
            /* Integer-Indexed Exotic Objects [[DefineOwnProperty]]. */
            if (!isnan(njs_string_to_index(name))) {
                njs_type_error(vm, "Invalid typed array index");
                return NJS_ERROR;
            }
        }

        /* 6.2.5.6 CompletePropertyDescriptor */

        if (njs_is_accessor_descriptor(prop)) {
            if (!njs_is_valid(&prop->getter)) {
                njs_set_undefined(&prop->getter);
            }

            if (!njs_is_valid(&prop->setter)) {
                njs_set_undefined(&prop->setter);
            }

        } else {
            if (prop->writable == NJS_ATTRIBUTE_UNSET) {
                prop->writable = 0;
            }

            if (!njs_is_valid(&prop->value)) {
                njs_set_undefined(&prop->value);
            }
        }

        if (prop->enumerable == NJS_ATTRIBUTE_UNSET) {
            prop->enumerable = 0;
        }

        if (prop->configurable == NJS_ATTRIBUTE_UNSET) {
            prop->configurable = 0;
        }

        if (njs_slow_path(pq.lhq.value != NULL)) {
            prev = pq.lhq.value;

            if (njs_slow_path(prev->type == NJS_WHITEOUT)) {
                /* Previously deleted property.  */
                *prev = *prop;
            }

        } else {
            pq.lhq.value = prop;
            pq.lhq.replace = 0;
            pq.lhq.pool = vm->mem_pool;

            ret = njs_lvlhsh_insert(njs_object_hash(object), &pq.lhq);
            if (njs_slow_path(ret != NJS_OK)) {
                njs_internal_error(vm, "lvlhsh insert failed");
                return NJS_ERROR;
            }
        }

        return NJS_OK;
    }

    /* Updating existing prop. */

    prev = pq.lhq.value;

    switch (prev->type) {
    case NJS_PROPERTY:
    case NJS_PROPERTY_HANDLER:
        break;

    case NJS_PROPERTY_REF:
        if (njs_is_accessor_descriptor(prop)
            || prop->configurable == NJS_ATTRIBUTE_FALSE
            || prop->enumerable == NJS_ATTRIBUTE_FALSE
            || prop->writable == NJS_ATTRIBUTE_FALSE)
        {
            array = njs_array(object);
            length = array->length;

            ret = njs_array_convert_to_slow_array(vm, array);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

            ret = njs_array_length_redefine(vm, object, length);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

            goto again;
        }

        if (njs_is_valid(&prop->value)) {
            *prev->value.data.u.value = prop->value;
        } else {
            njs_set_undefined(prev->value.data.u.value);
        }

        return NJS_OK;

    case NJS_PROPERTY_TYPED_ARRAY_REF:
        if (njs_is_accessor_descriptor(prop)) {
            goto exception;
        }

        if (prop->configurable == NJS_ATTRIBUTE_TRUE ||
            prop->enumerable == NJS_ATTRIBUTE_FALSE ||
            prop->writable == NJS_ATTRIBUTE_FALSE)
        {
            goto exception;
        }

        if (njs_is_valid(&prop->value)) {
            return njs_typed_array_set_value(vm, njs_typed_array(&prev->value),
                                             prev->value.data.magic32,
                                             &prop->value);
        }

        return NJS_OK;

    default:
        njs_internal_error(vm, "unexpected property type \"%s\" "
                           "while defining property",
                           njs_prop_type_string(prev->type));

        return NJS_ERROR;
    }

    /* 9.1.6.3 ValidateAndApplyPropertyDescriptor */

    if (!prev->configurable) {

        if (prop->configurable == NJS_ATTRIBUTE_TRUE) {
            goto exception;
        }

        if (prop->enumerable != NJS_ATTRIBUTE_UNSET
            && prev->enumerable != prop->enumerable)
        {
            goto exception;
        }
    }

    if (njs_is_generic_descriptor(prop)) {
        goto done;
    }

    if (njs_is_data_descriptor(prev) != njs_is_data_descriptor(prop)) {
        if (!prev->configurable) {
            goto exception;
        }

        /*
         * 6.b-c Preserve the existing values of the converted property's
         * [[Configurable]] and [[Enumerable]] attributes and set the rest of
         * the property's attributes to their default values.
         */

        if (njs_is_data_descriptor(prev)) {
            njs_set_undefined(&prev->getter);
            njs_set_undefined(&prev->setter);

            njs_set_invalid(&prev->value);
            prev->writable = NJS_ATTRIBUTE_UNSET;

        } else {
            njs_set_undefined(&prev->value);
            prev->writable = NJS_ATTRIBUTE_FALSE;

            njs_set_invalid(&prev->getter);
            njs_set_invalid(&prev->setter);
        }


    } else if (njs_is_data_descriptor(prev)
               && njs_is_data_descriptor(prop))
    {
        if (!prev->configurable && !prev->writable) {
            if (prop->writable == NJS_ATTRIBUTE_TRUE) {
                goto exception;
            }

            if (njs_is_valid(&prop->value)
                && prev->type != NJS_PROPERTY_HANDLER
                && !njs_values_same(&prop->value, &prev->value))
            {
                goto exception;
            }
        }

    } else {
        if (!prev->configurable) {
            if (njs_is_valid(&prop->getter)
                && !njs_values_strict_equal(&prop->getter, &prev->getter))
            {
                goto exception;
            }

            if (njs_is_valid(&prop->setter)
                && !njs_values_strict_equal(&prop->setter, &prev->setter))
            {
                goto exception;
            }
        }
    }

done:

    if (njs_is_valid(&prop->value) || njs_is_accessor_descriptor(prop)) {
        if (prev->type == NJS_PROPERTY_HANDLER) {
            if (prev->writable) {
                ret = prev->value.data.u.prop_handler(vm, prev, object,
                                                      &prop->value,
                                                      &vm->retval);
                if (njs_slow_path(ret == NJS_ERROR)) {
                    return ret;
                }

                if (ret == NJS_DECLINED) {
                    pq.lhq.value = NULL;
                    goto set_prop;
                }
            }

        } else {
            if (njs_slow_path(pq.lhq.key_hash == NJS_LENGTH_HASH)) {
                if (njs_strstr_eq(&pq.lhq.key, &length_key)) {
                    ret = njs_array_length_set(vm, object, prev, &prop->value);
                    if (ret != NJS_DECLINED) {
                        return ret;
                    }
                }
            }

            prev->value = prop->value;
        }
    }

    /*
     * 9. For each field of Desc that is present, set the corresponding
     * attribute of the property named P of object O to the value of the field.
     */

    if (njs_is_valid(&prop->getter)) {
        prev->getter = prop->getter;
    }

    if (njs_is_valid(&prop->setter)) {
        prev->setter = prop->setter;
    }

    if (prop->writable != NJS_ATTRIBUTE_UNSET) {
        prev->writable = prop->writable;
    }

    if (prop->enumerable != NJS_ATTRIBUTE_UNSET) {
        prev->enumerable = prop->enumerable;
    }

    if (prop->configurable != NJS_ATTRIBUTE_UNSET) {
        prev->configurable = prop->configurable;
    }

    return NJS_OK;

exception:

    njs_key_string_get(vm, &pq.key,  &pq.lhq.key);
    njs_type_error(vm, "Cannot redefine property: \"%V\"", &pq.lhq.key);

    return NJS_ERROR;
}