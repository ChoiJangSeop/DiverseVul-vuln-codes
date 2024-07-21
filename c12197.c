njs_vmcode_await(njs_vm_t *vm, njs_vmcode_await_t *await)
{
    size_t              size;
    njs_int_t           ret;
    njs_frame_t         *frame;
    njs_value_t         ctor, val, on_fulfilled, on_rejected, *value;
    njs_promise_t       *promise;
    njs_function_t      *fulfilled, *rejected;
    njs_async_ctx_t     *ctx;
    njs_native_frame_t  *active;

    active = &vm->active_frame->native;

    value = njs_scope_valid_value(vm, await->retval);
    if (njs_slow_path(value == NULL)) {
        return NJS_ERROR;
    }

    njs_set_function(&ctor, &vm->constructors[NJS_OBJ_TYPE_PROMISE]);

    promise = njs_promise_resolve(vm, &ctor, value);
    if (njs_slow_path(promise == NULL)) {
        return NJS_ERROR;
    }

    ctx = active->function->await;

    if (ctx == NULL) {
        ctx = njs_mp_alloc(vm->mem_pool, sizeof(njs_async_ctx_t));
        if (njs_slow_path(ctx == NULL)) {
            njs_memory_error(vm);
            return NJS_ERROR;
        }

        size = njs_function_frame_size(active);

        fulfilled = njs_promise_create_function(vm, size);
        if (njs_slow_path(fulfilled == NULL)) {
            return NJS_ERROR;
        }

        ctx->await = fulfilled->context;
        ctx->capability = active->function->context;

        active->function->context = NULL;

        ret = njs_function_frame_save(vm, ctx->await, NULL);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

    } else {
        fulfilled = njs_promise_create_function(vm, 0);
        if (njs_slow_path(fulfilled == NULL)) {
            return NJS_ERROR;
        }
    }

    ctx->pc = (u_char *) await + sizeof(njs_vmcode_await_t);
    ctx->index = await->retval;

    frame = (njs_frame_t *) active;

    if (frame->exception.catch != NULL) {
        ctx->await->native.pc = frame->exception.catch;

    } else {
        ctx->await->native.pc = ctx->pc;
    }

    fulfilled->context = ctx;
    fulfilled->args_count = 1;
    fulfilled->u.native = njs_await_fulfilled;

    rejected = njs_promise_create_function(vm, 0);
    if (njs_slow_path(rejected == NULL)) {
        return NJS_ERROR;
    }

    rejected->context = ctx;
    rejected->args_count = 1;
    rejected->u.native = njs_await_rejected;

    njs_set_promise(&val, promise);
    njs_set_function(&on_fulfilled, fulfilled);
    njs_set_function(&on_rejected, rejected);

    ret = njs_promise_perform_then(vm, &val, &on_fulfilled, &on_rejected, NULL);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    (void) njs_vmcode_return(vm, NULL, &vm->retval);

    return NJS_AGAIN;
}