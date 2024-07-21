njs_function_prototype_apply(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int64_t         i, length;
    njs_int_t       ret;
    njs_frame_t     *frame;
    njs_value_t     *this, *arr_like;
    njs_array_t     *arr;
    njs_function_t  *func;

    if (!njs_is_function(njs_argument(args, 0))) {
        njs_type_error(vm, "\"this\" argument is not a function");
        return NJS_ERROR;
    }

    func = njs_function(njs_argument(args, 0));
    this = njs_arg(args, nargs, 1);
    arr_like = njs_arg(args, nargs, 2);

    if (njs_is_null_or_undefined(arr_like)) {
        length = 0;

        goto activate;

    } else if (njs_is_array(arr_like)) {
        arr = arr_like->data.u.array;

        args = arr->start;
        length = arr->length;

        goto activate;

    } else if (njs_slow_path(!njs_is_object(arr_like))) {
        njs_type_error(vm, "second argument is not an array-like object");
        return NJS_ERROR;
    }

    ret = njs_object_length(vm, arr_like, &length);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    arr = njs_array_alloc(vm, 1, length, NJS_ARRAY_SPARE);
    if (njs_slow_path(arr == NULL)) {
        return NJS_ERROR;
    }

    args = arr->start;

    for (i = 0; i < length; i++) {
        ret = njs_value_property_i64(vm, arr_like, i, &args[i]);
        if (njs_slow_path(ret == NJS_ERROR)) {
            return ret;
        }
    }

activate:

    /* Skip the "apply" method frame. */
    vm->top_frame->skip = 1;

    frame = (njs_frame_t *) vm->top_frame;

    ret = njs_function_frame(vm, func, this, args, length, 0);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_function_frame_invoke(vm, frame->native.retval);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return NJS_DECLINED;
}