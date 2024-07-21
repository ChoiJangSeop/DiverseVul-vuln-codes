zcrd1_params(os_ptr op, gs_cie_render * pcrd,
             ref_cie_render_procs * pcprocs, gs_memory_t * mem)
{
    int code;
    int ignore;
    gx_color_lookup_table *const prtl = &pcrd->RenderTable.lookup;
    ref *pRT;

    if ((code = dict_int_param(op, "ColorRenderingType", 1, 1, 0, &ignore)) < 0)
        return code;
    if ((code = zcrd1_proc_params(mem, op, pcprocs)) < 0)
        return code;

    if ((code = dict_matrix3_param(mem, op, "MatrixLMN", &pcrd->MatrixLMN)) < 0)
        return code;
    if ((code = dict_range3_param(mem, op, "RangeLMN", &pcrd->RangeLMN)) < 0)
        return code;
    if ((code = dict_matrix3_param(mem, op, "MatrixABC", &pcrd->MatrixABC)) < 0)
        return code;
    if ((code = dict_range3_param(mem, op, "RangeABC", &pcrd->RangeABC)) < 0)
        return code;
    if ((code = cie_points_param(mem, op, &pcrd->points)) < 0)
        return code;
    if ((code = dict_matrix3_param(mem, op, "MatrixPQR", &pcrd->MatrixPQR)) < 0)
        return code;
    if ((code = dict_range3_param(mem,op, "RangePQR", &pcrd->RangePQR)) < 0)
        return code;

    if (dict_find_string(op, "RenderTable", &pRT) > 0) {
        const ref *prte = pRT->value.const_refs;

        /* Finish unpacking and checking the RenderTable parameter. */
        check_type_only(prte[4], t_integer);
        if (!(prte[4].value.intval == 3 || prte[4].value.intval == 4))
            return_error(gs_error_rangecheck);
        prtl->n = 3;
        prtl->m = prte[4].value.intval;
        if (r_size(pRT) != prtl->m + 5)
            return_error(gs_error_rangecheck);
        code = cie_table_param(pRT, prtl, mem);
        if (code < 0)
            return code;
    } else {
        prtl->table = 0;
    }
    pcrd->EncodeLMN = Encode_default;
    pcrd->EncodeABC = Encode_default;
    pcrd->TransformPQR = TransformPQR_default;
    pcrd->RenderTable.T = RenderTableT_default;
    return 0;
}