execstack2_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    return do_execstack(i_ctx_p, op->value.boolval, op - 1);
}