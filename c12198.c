njs_function_frame_invoke(njs_vm_t *vm, njs_value_t *retval)
{
    njs_native_frame_t  *frame;

    frame = vm->top_frame;
    frame->retval = retval;

    if (njs_function_object_type(vm, frame->function)
        == NJS_OBJ_TYPE_ASYNC_FUNCTION)
    {
        return njs_async_function_frame_invoke(vm, retval);
    }

    if (frame->native) {
        return njs_function_native_call(vm);

    } else {
        return njs_function_lambda_call(vm);
    }
}