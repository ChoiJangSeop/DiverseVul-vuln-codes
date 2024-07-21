njs_string_prototype_replace(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char             *r;
    size_t             length, search_length, ret_length, size;
    int64_t            pos;
    njs_int_t          ret;
    njs_value_t        *this, *search, *replace;
    njs_value_t        search_lvalue, replace_lvalue, replacer, retval,
                       arguments[3];
    const u_char       *p;
    njs_function_t     *func_replace;
    njs_string_prop_t  string, s, ret_string;

    static const njs_value_t  replace_key =
                                      njs_wellknown_symbol(NJS_SYMBOL_REPLACE);

    this = njs_argument(args, 0);

    if (njs_slow_path(njs_is_null_or_undefined(this))) {
        njs_type_error(vm, "cannot convert \"%s\"to object",
                       njs_type_string(this->type));
        return NJS_ERROR;
    }

    search = njs_lvalue_arg(&search_lvalue, args, nargs, 1);
    replace = njs_lvalue_arg(&replace_lvalue, args, nargs, 2);

    if (!njs_is_null_or_undefined(search)) {
        ret = njs_value_method(vm, search, njs_value_arg(&replace_key),
                               &replacer);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }

        if (njs_is_defined(&replacer)) {
            arguments[0] = *this;
            arguments[1] = *replace;

            return njs_function_call(vm, njs_function(&replacer), search,
                                     arguments, 2, &vm->retval);
        }
    }

    ret = njs_value_to_string(vm, this, this);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_value_to_string(vm, search, search);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    func_replace = njs_is_function(replace) ? njs_function(replace) : NULL;

    if (func_replace == NULL) {
        ret = njs_value_to_string(vm, replace, replace);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    length = njs_string_prop(&string, this);
    search_length = njs_string_prop(&s, search);

    pos = njs_string_index_of(&string, &s, 0);
    if (pos < 0) {
        vm->retval = *this;
        return NJS_OK;
    }

    if (func_replace == NULL) {
        ret = njs_string_get_substitution(vm, search, this, pos, NULL, 0, NULL,
                                          replace, &retval);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }

    } else {
        arguments[0] = *search;
        njs_set_number(&arguments[1], pos);
        arguments[2] = *this;

        ret = njs_function_call(vm, func_replace,
                                njs_value_arg(&njs_value_undefined),
                                arguments, 3, &retval);

        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }

        ret = njs_value_to_string(vm, &retval, &retval);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }
    }

    if (length == string.size) {
        p = string.start + pos;

    } else {
        /* UTF-8 string. */
        p = njs_string_offset(string.start, string.start + string.size, pos);
    }

    ret_length = njs_string_prop(&ret_string, &retval);

    size = string.size + ret_string.size -  s.size;
    length += ret_length - search_length;

    r = njs_string_alloc(vm, &vm->retval, size, length);
    if (njs_slow_path(r == NULL)) {
        return NJS_ERROR;
    }

    r = njs_cpymem(r, string.start, p - string.start);
    r = njs_cpymem(r, ret_string.start, ret_string.size);
    memcpy(r, p + s.size, string.size - s.size - (p - string.start));

    return NJS_OK;
}