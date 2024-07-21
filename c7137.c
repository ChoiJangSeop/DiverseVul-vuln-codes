zundef(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int code;

    check_type(*op1, t_dictionary);
    if (i_ctx_p->in_superexec == 0)
        check_dict_write(*op1);
    code = idict_undef(op1, op);
    if (code < 0 && code != gs_error_undefined) /* ignore undefined error */
        return code;
    pop(2);
    return 0;
}