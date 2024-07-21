lambda_function_body(
	char_u	    **arg,
	typval_T    *rettv,
	evalarg_T   *evalarg,
	garray_T    *newargs,
	garray_T    *argtypes,
	int	    varargs,
	garray_T    *default_args,
	char_u	    *ret_type)
{
    int		evaluate = (evalarg->eval_flags & EVAL_EVALUATE);
    garray_T	*gap = &evalarg->eval_ga;
    garray_T	*freegap = &evalarg->eval_freega;
    ufunc_T	*ufunc = NULL;
    exarg_T	eap;
    garray_T	newlines;
    char_u	*cmdline = NULL;
    int		ret = FAIL;
    char_u	*line_to_free = NULL;
    partial_T	*pt;
    char_u	*name;
    int		lnum_save = -1;
    linenr_T	sourcing_lnum_top = SOURCING_LNUM;

    if (!ends_excmd2(*arg, skipwhite(*arg + 1)))
    {
	semsg(_(e_trailing_characters_str), *arg + 1);
	return FAIL;
    }

    CLEAR_FIELD(eap);
    eap.cmdidx = CMD_block;
    eap.forceit = FALSE;
    eap.cmdlinep = &cmdline;
    eap.skip = !evaluate;
    if (evalarg->eval_cctx != NULL)
	fill_exarg_from_cctx(&eap, evalarg->eval_cctx);
    else
    {
	eap.getline = evalarg->eval_getline;
	eap.cookie = evalarg->eval_cookie;
    }

    ga_init2(&newlines, (int)sizeof(char_u *), 10);
    if (get_function_body(&eap, &newlines, NULL, &line_to_free) == FAIL)
    {
	if (cmdline != line_to_free)
	    vim_free(cmdline);
	goto erret;
    }

    // When inside a lambda must add the function lines to evalarg.eval_ga.
    evalarg->eval_break_count += newlines.ga_len;
    if (gap->ga_itemsize > 0)
    {
	int	idx;
	char_u	*last;
	size_t  plen;
	char_u  *pnl;

	for (idx = 0; idx < newlines.ga_len; ++idx)
	{
	    char_u  *p = skipwhite(((char_u **)newlines.ga_data)[idx]);

	    if (ga_grow(gap, 1) == FAIL || ga_grow(freegap, 1) == FAIL)
		goto erret;

	    // Going to concatenate the lines after parsing.  For an empty or
	    // comment line use an empty string.
	    // Insert NL characters at the start of each line, the string will
	    // be split again later in .get_lambda_tv().
	    if (*p == NUL || vim9_comment_start(p))
		p = (char_u *)"";
	    plen = STRLEN(p);
	    pnl = vim_strnsave((char_u *)"\n", plen + 1);
	    if (pnl != NULL)
		mch_memmove(pnl + 1, p, plen + 1);
	    ((char_u **)gap->ga_data)[gap->ga_len++] = pnl;
	    ((char_u **)freegap->ga_data)[freegap->ga_len++] = pnl;
	}
	if (ga_grow(gap, 1) == FAIL || ga_grow(freegap, 1) == FAIL)
	    goto erret;
	if (eap.nextcmd != NULL)
	    // more is following after the "}", which was skipped
	    last = cmdline;
	else
	    // nothing is following the "}"
	    last = (char_u *)"}";
	plen = STRLEN(last);
	pnl = vim_strnsave((char_u *)"\n", plen + 1);
	if (pnl != NULL)
	    mch_memmove(pnl + 1, last, plen + 1);
	((char_u **)gap->ga_data)[gap->ga_len++] = pnl;
	((char_u **)freegap->ga_data)[freegap->ga_len++] = pnl;
    }

    if (eap.nextcmd != NULL)
    {
	garray_T *tfgap = &evalarg->eval_tofree_ga;

	// Something comes after the "}".
	*arg = eap.nextcmd;

	// "arg" points into cmdline, need to keep the line and free it later.
	if (ga_grow(tfgap, 1) == OK)
	{
	    ((char_u **)(tfgap->ga_data))[tfgap->ga_len++] = cmdline;
	    evalarg->eval_using_cmdline = TRUE;
	    if (cmdline == line_to_free)
		line_to_free = NULL;
	}
    }
    else
	*arg = (char_u *)"";

    if (!evaluate)
    {
	ret = OK;
	goto erret;
    }

    name = get_lambda_name();
    ufunc = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
    if (ufunc == NULL)
	goto erret;
    set_ufunc_name(ufunc, name);
    if (hash_add(&func_hashtab, UF2HIKEY(ufunc)) == FAIL)
	goto erret;
    ufunc->uf_flags = FC_LAMBDA;
    ufunc->uf_refcount = 1;
    ufunc->uf_args = *newargs;
    newargs->ga_data = NULL;
    ufunc->uf_def_args = *default_args;
    default_args->ga_data = NULL;
    ufunc->uf_func_type = &t_func_any;

    // error messages are for the first function line
    lnum_save = SOURCING_LNUM;
    SOURCING_LNUM = sourcing_lnum_top;

    // parse argument types
    if (parse_argument_types(ufunc, argtypes, varargs) == FAIL)
    {
	SOURCING_LNUM = lnum_save;
	goto erret;
    }

    // parse the return type, if any
    if (parse_return_type(ufunc, ret_type) == FAIL)
	goto erret;

    pt = ALLOC_CLEAR_ONE(partial_T);
    if (pt == NULL)
	goto erret;
    pt->pt_func = ufunc;
    pt->pt_refcount = 1;

    ufunc->uf_lines = newlines;
    newlines.ga_data = NULL;
    if (sandbox)
	ufunc->uf_flags |= FC_SANDBOX;
    if (!ASCII_ISUPPER(*ufunc->uf_name))
	ufunc->uf_flags |= FC_VIM9;
    ufunc->uf_script_ctx = current_sctx;
    ufunc->uf_script_ctx_version = current_sctx.sc_version;
    ufunc->uf_script_ctx.sc_lnum += sourcing_lnum_top;
    set_function_type(ufunc);

    function_using_block_scopes(ufunc, evalarg->eval_cstack);

    rettv->vval.v_partial = pt;
    rettv->v_type = VAR_PARTIAL;
    ufunc = NULL;
    ret = OK;

erret:
    if (lnum_save >= 0)
	SOURCING_LNUM = lnum_save;
    vim_free(line_to_free);
    ga_clear_strings(&newlines);
    if (newargs != NULL)
	ga_clear_strings(newargs);
    ga_clear_strings(default_args);
    if (ufunc != NULL)
    {
	func_clear(ufunc, TRUE);
	func_free(ufunc, TRUE);
    }
    return ret;
}