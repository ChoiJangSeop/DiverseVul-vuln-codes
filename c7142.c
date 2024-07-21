zsuperexec(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr ep;

    check_op(1);
    if (!r_has_attr(op, a_executable))
        return 0;		/* literal object just gets pushed back */
    check_estack(2);
    ep = esp += 3;
    make_mark_estack(ep - 2, es_other, end_superexec); /* error case */
    make_op_estack(ep - 1,  end_superexec); /* normal case */
    ref_assign(ep, op);
    esfile_check_cache();
    pop(1);
    i_ctx_p->in_superexec++;
    return o_push_estack;
}