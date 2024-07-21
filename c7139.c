zput(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    os_ptr op2 = op1 - 1;
    byte *sdata;
    uint ssize;

    switch (r_type(op2)) {
        case t_dictionary:
            if (i_ctx_p->in_superexec == 0)
                check_dict_write(*op2);
            {
                int code = idict_put(op2, op1, op);

                if (code < 0)
                    return code;	/* error */
            }
            break;
        case t_array:
            check_write(*op2);
            check_int_ltu(*op1, r_size(op2));
            store_check_dest(op2, op);
            {
                ref *eltp = op2->value.refs + (uint) op1->value.intval;

                ref_assign_old(op2, eltp, op, "put");
            }
            break;
        case t_mixedarray:	/* packed arrays are read-only */
        case t_shortarray:
            return_error(gs_error_invalidaccess);
        case t_string:
            sdata = op2->value.bytes;
            ssize = r_size(op2);
str:	    check_write(*op2);
            check_int_ltu(*op1, ssize);
            check_int_leu(*op, 0xff);
            sdata[(uint)op1->value.intval] = (byte)op->value.intval;
            break;
        case t_astruct:
            if (gs_object_type(imemory, op2->value.pstruct) != &st_bytes)
                return_error(gs_error_typecheck);
            sdata = r_ptr(op2, byte);
            ssize = gs_object_size(imemory, op2->value.pstruct);
            goto str;
        default:
            return_op_typecheck(op2);
    }
    pop(3);
    return 0;
}