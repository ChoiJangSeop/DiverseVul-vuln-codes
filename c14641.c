njs_array_prototype_sort(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int64_t                i, und, len, nlen, length;
    njs_int_t              ret, fast_path;
    njs_array_t            *array;
    njs_value_t            *this, *comparefn, *start, *strings;
    njs_array_sort_ctx_t   ctx;
    njs_array_sort_slot_t  *p, *end, *slots, *nslots;

    comparefn = njs_arg(args, nargs, 1);

    if (njs_is_defined(comparefn)) {
        if (njs_slow_path(!njs_is_function(comparefn))) {
            njs_type_error(vm, "comparefn must be callable or undefined");
            return NJS_ERROR;
        }

        ctx.function = njs_function(comparefn);

    } else {
        ctx.function = NULL;
    }

    this = njs_argument(args, 0);

    ret = njs_value_to_object(vm, this);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_value_length(vm, this, &length);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (njs_slow_path(length < 2)) {
        vm->retval = *this;
        return NJS_OK;
    }

    slots = NULL;
    ctx.vm = vm;
    ctx.strings.separate = 0;
    ctx.strings.pointer = 0;
    ctx.exception = 0;

    fast_path = njs_is_fast_array(this);

    if (njs_fast_path(fast_path)) {
        array = njs_array(this);
        start = array->start;

        slots = njs_mp_alloc(vm->mem_pool,
                             sizeof(njs_array_sort_slot_t) * length);
        if (njs_slow_path(slots == NULL)) {
                return NJS_ERROR;
        }

        und = 0;
        p = slots;

        for (i = 0; i < length; i++) {
            if (njs_slow_path(!njs_is_valid(&start[i]))) {
                fast_path = 0;
                njs_mp_free(vm->mem_pool, slots);
                slots = NULL;
                goto slow_path;
            }

            if (njs_slow_path(njs_is_undefined(&start[i]))) {
                und++;
                continue;
            }

            p->value = start[i];
            p->pos = i;
            p->str = NULL;
            p++;
        }

        len = p - slots;

    } else {

slow_path:

        und = 0;
        p = NULL;
        end = NULL;

        for (i = 0; i < length; i++) {
            if (p >= end) {
                nlen = njs_min(njs_max((p - slots) * 2, 8), length);
                nslots = njs_mp_alloc(vm->mem_pool,
                                      sizeof(njs_array_sort_slot_t) * nlen);
                if (njs_slow_path(nslots == NULL)) {
                    njs_memory_error(vm);
                    return NJS_ERROR;
                }

                if (slots != NULL) {
                    p = (void *) njs_cpymem(nslots, slots,
                                  sizeof(njs_array_sort_slot_t) * (p - slots));
                    njs_mp_free(vm->mem_pool, slots);

                } else {
                    p = nslots;
                }

                slots = nslots;
                end = slots + nlen;
            }

            ret = njs_value_property_i64(vm, this, i, &p->value);
            if (njs_slow_path(ret == NJS_ERROR)) {
                ret = NJS_ERROR;
                goto exception;
            }

            if (ret == NJS_DECLINED) {
                continue;
            }

            if (njs_is_undefined(&p->value)) {
                und++;
                continue;
            }

            p->pos = i;
            p->str = NULL;
            p++;
        }

        len = p - slots;
    }

    strings = njs_arr_init(vm->mem_pool, &ctx.strings, NULL, len + 1,
                           sizeof(njs_value_t));
    if (njs_slow_path(strings == NULL)) {
        ret = NJS_ERROR;
        goto exception;
    }

    njs_qsort(slots, len, sizeof(njs_array_sort_slot_t), njs_array_compare,
              &ctx);

    if (ctx.exception) {
        ret = NJS_ERROR;
        goto exception;
    }

    if (njs_fast_path(fast_path)) {
        array = njs_array(this);
        start = array->start;

        for (i = 0; i < len; i++) {
            start[i] = slots[i].value;
        }

        for (i = len; und-- > 0; i++) {
            start[i] = njs_value_undefined;
        }

    } else {
        for (i = 0; i < len; i++) {
            if (slots[i].pos != i) {
                ret = njs_value_property_i64_set(vm, this, i, &slots[i].value);
                if (njs_slow_path(ret == NJS_ERROR)) {
                    goto exception;
                }
            }
        }

        for (i = len; und-- > 0; i++) {
            ret = njs_value_property_i64_set(vm, this, i,
                                          njs_value_arg(&njs_value_undefined));
            if (njs_slow_path(ret == NJS_ERROR)) {
                goto exception;
            }
        }

        for (; i < length; i++) {
            ret = njs_value_property_i64_delete(vm, this, i, NULL);
            if (njs_slow_path(ret == NJS_ERROR)) {
                goto exception;
            }
        }
    }

    vm->retval = *this;

    ret = NJS_OK;

exception:

    if (slots != NULL) {
        njs_mp_free(vm->mem_pool, slots);
    }

    njs_arr_destroy(&ctx.strings);

    return ret;
}