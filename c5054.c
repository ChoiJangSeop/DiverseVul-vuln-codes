ref_param_read_typed(gs_param_list * plist, gs_param_name pkey,
                     gs_param_typed_value * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    ref elt;
    int code = ref_param_read(iplist, pkey, &loc, -1);

    if (code != 0)
        return code;
    switch (r_type(loc.pvalue)) {
        case t_array:
        case t_mixedarray:
        case t_shortarray:
            iparam_check_read(loc);
            if (r_size(loc.pvalue) <= 0) {
                /* 0-length array; can't get type info */
                pvalue->type = gs_param_type_array;
                pvalue->value.d.list = 0;
                pvalue->value.d.size = 0;
                return 0;
            }
            /*
             * We have to guess at the array type.  First we guess based
             * on the type of the first element of the array.  If that
             * fails, we try again with more general types.
             */
            array_get(plist->memory, loc.pvalue, 0, &elt);
            switch (r_type(&elt)) {
                case t_integer:
                    pvalue->type = gs_param_type_int_array;
                    code = ref_param_read_int_array(plist, pkey,
                                                    &pvalue->value.ia);
                    if (code != gs_error_typecheck)
                        return code;
                    /* This might be a float array.  Fall through. */
                    *loc.presult = 0;  /* reset error */
                case t_real:
                    pvalue->type = gs_param_type_float_array;
                    return ref_param_read_float_array(plist, pkey,
                                                      &pvalue->value.fa);
                case t_string:
                    pvalue->type = gs_param_type_string_array;
                    return ref_param_read_string_array(plist, pkey,
                                                       &pvalue->value.sa);
                case t_name:
                    pvalue->type = gs_param_type_name_array;
                    return ref_param_read_string_array(plist, pkey,
                                                       &pvalue->value.na);
                default:
                    break;
            }
            return gs_note_error(gs_error_typecheck);
        case t_boolean:
            pvalue->type = gs_param_type_bool;
            pvalue->value.b = loc.pvalue->value.boolval;
            return 0;
        case t_dictionary:
            code = ref_param_begin_read_collection(plist, pkey,
                            &pvalue->value.d, gs_param_collection_dict_any);
            if (code < 0)
                return code;
            pvalue->type = gs_param_type_dict;

            /* fixup new dict's type & int_keys field if contents have int keys */
            {
                gs_param_enumerator_t enumr;
                gs_param_key_t key;
                ref_type keytype;

                param_init_enumerator(&enumr);
                if (!(*((iparam_list *) plist)->enumerate)
                    ((iparam_list *) pvalue->value.d.list, &enumr, &key, &keytype)
                    && keytype == t_integer) {
                    ((dict_param_list *) pvalue->value.d.list)->int_keys = 1;
                    pvalue->type = gs_param_type_dict_int_keys;
                }
            }
            return 0;
        case t_integer:
            pvalue->type = gs_param_type_long;
            pvalue->value.l = loc.pvalue->value.intval;
            return 0;
        case t_name:
            pvalue->type = gs_param_type_name;
            return ref_param_read_string_value(plist->memory, &loc, &pvalue->value.n);
        case t_null:
            pvalue->type = gs_param_type_null;
            return 0;
        case t_real:
            pvalue->value.f = loc.pvalue->value.realval;
            pvalue->type = gs_param_type_float;
            return 0;
        case t_string:
        case t_astruct:
            pvalue->type = gs_param_type_string;
            return ref_param_read_string_value(plist->memory, &loc, &pvalue->value.s);
        default:
            break;
    }
    return gs_note_error(gs_error_typecheck);
}