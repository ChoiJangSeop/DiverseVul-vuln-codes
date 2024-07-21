do_execstack(i_ctx_t *i_ctx_p, bool include_marks, os_ptr op1)
{
    os_ptr op = osp;
    ref *arefs = op1->value.refs;
    uint asize = r_size(op1);
    uint i;
    ref *rq;

    /*
     * Copy elements from the stack to the array,
     * optionally skipping executable nulls.
     * Clear the executable bit in any internal operators, and
     * convert t_structs and t_astructs (which can only appear
     * in connection with stack marks, which means that they will
     * probably be freed when unwinding) to something harmless.
     */
    for (i = 0, rq = arefs + asize; rq != arefs; ++i) {
        const ref *rp = ref_stack_index(&e_stack, (long)i);

        if (r_has_type_attrs(rp, t_null, a_executable) && !include_marks)
            continue;
        --rq;
        ref_assign_old(op1, rq, rp, "execstack");
        switch (r_type(rq)) {
            case t_operator: {
                uint opidx = op_index(rq);

                if (opidx == 0 || op_def_is_internal(op_index_def(opidx)))
                    r_clear_attrs(rq, a_executable);
                break;
            }
            case t_struct:
            case t_astruct: {
                const char *tname = rq->value.pstruct ?
                    gs_struct_type_name_string(
                                gs_object_type(imemory, rq->value.pstruct))
                    : "NULL";

                make_const_string(rq, a_readonly | avm_foreign,
                                  strlen(tname), (const byte *)tname);
                break;
            }
            default:
                ;
        }
    }
    pop(op - op1);
    return 0;
}