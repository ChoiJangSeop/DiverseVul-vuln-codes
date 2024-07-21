njs_vm_start(njs_vm_t *vm)
{
    njs_int_t  ret;

    ret = njs_module_load(vm);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_vmcode_interpreter(vm, vm->start);

    return (ret == NJS_ERROR) ? NJS_ERROR : NJS_OK;
}