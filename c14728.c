njs_promise_perform_then(njs_vm_t *vm, njs_value_t *value,
    njs_value_t *fulfilled, njs_value_t *rejected,
    njs_promise_capability_t *capability)
{
    njs_int_t               ret;
    njs_value_t             arguments[2];
    njs_promise_t           *promise;
    njs_function_t          *function;
    njs_promise_data_t      *data;
    njs_promise_reaction_t  *fulfilled_reaction, *rejected_reaction;

    if (!njs_is_function(fulfilled)) {
        fulfilled = njs_value_arg(&njs_value_undefined);
    }

    if (!njs_is_function(rejected)) {
        rejected = njs_value_arg(&njs_value_undefined);
    }

    promise = njs_promise(value);
    data = njs_data(&promise->value);

    fulfilled_reaction = njs_mp_alloc(vm->mem_pool,
                                      sizeof(njs_promise_reaction_t));
    if (njs_slow_path(fulfilled_reaction == NULL)) {
        njs_memory_error(vm);
        return NJS_ERROR;
    }

    fulfilled_reaction->capability = capability;
    fulfilled_reaction->handler = *fulfilled;
    fulfilled_reaction->type = NJS_PROMISE_FULFILL;

    rejected_reaction = njs_mp_alloc(vm->mem_pool,
                                     sizeof(njs_promise_reaction_t));
    if (njs_slow_path(rejected_reaction == NULL)) {
        njs_memory_error(vm);
        return NJS_ERROR;
    }

    rejected_reaction->capability = capability;
    rejected_reaction->handler = *rejected;
    rejected_reaction->type = NJS_PROMISE_REJECTED;

    if (data->state == NJS_PROMISE_PENDING) {
        njs_queue_insert_tail(&data->fulfill_queue, &fulfilled_reaction->link);
        njs_queue_insert_tail(&data->reject_queue, &rejected_reaction->link);

    } else {
        function = njs_promise_create_function(vm,
                                               sizeof(njs_promise_context_t));
        function->u.native = njs_promise_reaction_job;

        if (data->state == NJS_PROMISE_REJECTED) {
            njs_set_data(&arguments[0], rejected_reaction, 0);

            ret = njs_promise_host_rejection_tracker(vm, promise,
                                                     NJS_PROMISE_HANDLE);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

        } else {
            njs_set_data(&arguments[0], fulfilled_reaction, 0);
        }

        arguments[1] = data->result;

        ret = njs_promise_add_event(vm, function, arguments, 2);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    data->is_handled = 1;

    if (capability == NULL) {
        njs_vm_retval_set(vm, &njs_value_undefined);

    } else {
        njs_vm_retval_set(vm, &capability->promise);
    }

    return NJS_OK;
}