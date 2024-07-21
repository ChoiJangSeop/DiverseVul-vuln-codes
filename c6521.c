zrestore(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    alloc_save_t *asave;
    bool last;
    vm_save_t *vmsave;
    int code = restore_check_operand(op, &asave, idmemory);

    if (code < 0)
        return code;
    if_debug2m('u', imemory, "[u]vmrestore 0x%lx, id = %lu\n",
               (ulong) alloc_save_client_data(asave),
               (ulong) op->value.saveid);
    if (I_VALIDATE_BEFORE_RESTORE)
        ivalidate_clean_spaces(i_ctx_p);
    /* Check the contents of the stacks. */
    osp--;
    {
        int code;

        if ((code = restore_check_stack(i_ctx_p, &o_stack, asave, false)) < 0 ||
            (code = restore_check_stack(i_ctx_p, &e_stack, asave, true)) < 0 ||
            (code = restore_check_stack(i_ctx_p, &d_stack, asave, false)) < 0
            ) {
            osp++;
            return code;
        }
    }
    /* Reset l_new in all stack entries if the new save level is zero. */
    /* Also do some special fixing on the e-stack. */
    restore_fix_stack(i_ctx_p, &o_stack, asave, false);
    restore_fix_stack(i_ctx_p, &e_stack, asave, true);
    restore_fix_stack(i_ctx_p, &d_stack, asave, false);
    /* Iteratively restore the state of memory, */
    /* also doing a grestoreall at each step. */
    do {
        vmsave = alloc_save_client_data(alloc_save_current(idmemory));
        /* Restore the graphics state. */
        gs_grestoreall_for_restore(igs, vmsave->gsave);
        /*
         * If alloc_save_space decided to do a second save, the vmsave
         * object was allocated one save level less deep than the
         * current level, so ifree_object won't actually free it;
         * however, it points to a gsave object that definitely
         * *has* been freed.  In order not to trip up the garbage
         * collector, we clear the gsave pointer now.
         */
        vmsave->gsave = 0;
        /* Now it's safe to restore the state of memory. */
        code = alloc_restore_step_in(idmemory, asave);
        if (code < 0)
            return code;
        last = code;
    }
    while (!last);
    {
        uint space = icurrent_space;

        ialloc_set_space(idmemory, avm_local);
        ifree_object(vmsave, "zrestore");
        ialloc_set_space(idmemory, space);
    }
    dict_set_top();		/* reload dict stack cache */
    if (I_VALIDATE_AFTER_RESTORE)
        ivalidate_clean_spaces(i_ctx_p);
    /* If the i_ctx_p LockFilePermissions is true, but the userparams */
    /* we just restored is false, we need to make sure that we do not */
    /* cause an 'invalidaccess' in setuserparams. Temporarily set     */
    /* LockFilePermissions false until the gs_lev2.ps can do a        */
    /* setuserparams from the restored userparam dictionary.          */
    i_ctx_p->LockFilePermissions = false;
    return 0;
}