zrsdparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref *pFilter;
    ref *pDecodeParms;
    int Intent = 0;
    bool AsyncRead = false;
    ref empty_array, filter1_array, parms1_array;
    uint i;
    int code = 0;

    if (ref_stack_count(&o_stack) < 1)
        return_error(gs_error_stackunderflow);
    if (!r_has_type(op, t_dictionary) && !r_has_type(op, t_null)) {
        return_error(gs_error_typecheck);
    }

    make_empty_array(&empty_array, a_readonly);
    if (r_has_type(op, t_dictionary)
        && dict_find_string(op, "Filter", &pFilter) > 0) {
        if (!r_is_array(pFilter)) {
            if (!r_has_type(pFilter, t_name))
                return_error(gs_error_typecheck);
            make_array(&filter1_array, a_readonly, 1, pFilter);
            pFilter = &filter1_array;
        }
    } else
        pFilter = &empty_array;
    /* If Filter is undefined, ignore DecodeParms. */
    if (pFilter != &empty_array &&
        dict_find_string(op, "DecodeParms", &pDecodeParms) > 0
        ) {
        if (pFilter == &filter1_array) {
            make_array(&parms1_array, a_readonly, 1, pDecodeParms);
            pDecodeParms = &parms1_array;
        } else if (!r_is_array(pDecodeParms))
            return_error(gs_error_typecheck);
        else if (r_size(pFilter) != r_size(pDecodeParms))
            return_error(gs_error_rangecheck);
    } else
        pDecodeParms = 0;
    for (i = 0; i < r_size(pFilter); ++i) {
        ref f, fname, dp;

        array_get(imemory, pFilter, (long)i, &f);
        if (!r_has_type(&f, t_name))
            return_error(gs_error_typecheck);
        name_string_ref(imemory, &f, &fname);
        if (r_size(&fname) < 6 ||
            memcmp(fname.value.bytes + r_size(&fname) - 6, "Decode", 6)
            )
            return_error(gs_error_rangecheck);
        if (pDecodeParms) {
            array_get(imemory, pDecodeParms, (long)i, &dp);
            if (!(r_has_type(&dp, t_dictionary) || r_has_type(&dp, t_null)))
                return_error(gs_error_typecheck);
        }
    }
    if (r_has_type(op, t_dictionary))
        code = dict_int_param(op, "Intent", 0, 3, 0, &Intent);
    if (code < 0 && code != gs_error_rangecheck) /* out-of-range int is ok, use 0 */
        return code;
    if (r_has_type(op, t_dictionary))
        if ((code = dict_bool_param(op, "AsyncRead", false, &AsyncRead)) < 0)
            return code;
    push(1);
    op[-1] = *pFilter;
    if (pDecodeParms)
        *op = *pDecodeParms;
    else
        make_null(op);
    return 0;
}