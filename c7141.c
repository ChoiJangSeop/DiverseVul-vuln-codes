zsetmaxlength(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    uint new_size;
    int code;

    check_type(*op1, t_dictionary);
    if (i_ctx_p->in_superexec == 0)
        check_dict_write(*op1);
    check_type(*op, t_integer);
    if (op->value.intval < 0)
        return_error(gs_error_rangecheck);
    new_size = (uint) op->value.intval;
    if (dict_length(op - 1) > new_size)
        return_error(gs_error_dictfull);
    code = idict_resize(op - 1, new_size);
    if (code >= 0)
        pop(2);
    return code;
}