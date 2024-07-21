njs_promise_perform_race_handler(njs_vm_t *vm, njs_iterator_args_t *args,
    njs_value_t *value, int64_t index)
{
    njs_int_t                    ret;
    njs_value_t                  arguments[2], next;
    njs_promise_capability_t     *capability;
    njs_promise_iterator_args_t  *pargs;

    pargs = (njs_promise_iterator_args_t *) args;

    ret = njs_function_call(vm, pargs->function, pargs->constructor, value,
                            1, &next);
    if (njs_slow_path(ret == NJS_ERROR)) {
        return ret;
    }

    capability = pargs->capability;

    arguments[0] = capability->resolve;
    arguments[1] = capability->reject;

    (void) njs_promise_invoke_then(vm, &next, arguments, 2);

    return NJS_OK;
}