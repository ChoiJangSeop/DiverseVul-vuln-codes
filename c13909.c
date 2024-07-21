njs_promise_perform_all_settled_handler(njs_vm_t *vm, njs_iterator_args_t *args,
    njs_value_t *value, int64_t index)
{
    njs_int_t                    ret;
    njs_array_t                  *array;
    njs_value_t                  arguments[2], next;
    njs_function_t               *on_fulfilled, *on_rejected;
    njs_promise_capability_t     *capability;
    njs_promise_all_context_t    *context;
    njs_promise_iterator_args_t  *pargs;

    pargs = (njs_promise_iterator_args_t *) args;

    capability = pargs->capability;

    array = args->data;
    njs_set_undefined(&array->start[index]);

    ret = njs_function_call(vm, pargs->function, pargs->constructor, value,
                            1, &next);
    if (njs_slow_path(ret == NJS_ERROR)) {
        return ret;
    }

    on_fulfilled = njs_promise_create_function(vm,
                                            sizeof(njs_promise_all_context_t));
    if (njs_slow_path(on_fulfilled == NULL)) {
        return NJS_ERROR;
    }

    context = on_fulfilled->context;

    context->already_called = 0;
    context->index = (uint32_t) index;
    context->values = pargs->args.data;
    context->capability = capability;
    context->remaining_elements = pargs->remaining;

    on_rejected = njs_promise_create_function(vm, 0);
    if (njs_slow_path(on_rejected == NULL)) {
        return NJS_ERROR;
    }

    on_fulfilled->u.native = njs_promise_all_settled_element_functions;
    on_rejected->u.native = njs_promise_all_settled_element_functions;
    on_rejected->magic8 = 1; /* rejected. */

    on_fulfilled->args_count = 1;
    on_rejected->args_count = 1;

    on_rejected->context = context;

    (*pargs->remaining)++;

    njs_set_function(&arguments[0], on_fulfilled);
    njs_set_function(&arguments[1], on_rejected);

    ret = njs_promise_invoke_then(vm, &next, arguments, 2);
    if (njs_slow_path(ret == NJS_ERROR)) {
        return ret;
    }

    return NJS_OK;
}