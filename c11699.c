get_lambda_tv(
	char_u	    **arg,
	typval_T    *rettv,
	int	    types_optional,
	evalarg_T   *evalarg)
{
    int		evaluate = evalarg != NULL
				      && (evalarg->eval_flags & EVAL_EVALUATE);
    garray_T	newargs;
    garray_T	newlines;
    garray_T	*pnewargs;
    garray_T	argtypes;
    garray_T	default_args;
    ufunc_T	*fp = NULL;
    partial_T   *pt = NULL;
    int		varargs;
    char_u	*ret_type = NULL;
    int		ret;
    char_u	*s;
    char_u	*start, *end;
    int		*old_eval_lavars = eval_lavars_used;
    int		eval_lavars = FALSE;
    char_u	*tofree1 = NULL;
    char_u	*tofree2 = NULL;
    int		equal_arrow = **arg == '(';
    int		white_error = FALSE;
    int		called_emsg_start = called_emsg;
    int		vim9script = in_vim9script();
    long	start_lnum = SOURCING_LNUM;

    if (equal_arrow && !vim9script)
	return NOTDONE;

    ga_init(&newargs);
    ga_init(&newlines);

    // First, check if this is really a lambda expression. "->" or "=>" must
    // be found after the arguments.
    s = *arg + 1;
    ret = get_function_args(&s, equal_arrow ? ')' : '-', NULL,
	    types_optional ? &argtypes : NULL, types_optional, evalarg,
					NULL, &default_args, TRUE, NULL, NULL);
    if (ret == FAIL || skip_arrow(s, equal_arrow, &ret_type, NULL) == NULL)
    {
	if (types_optional)
	    ga_clear_strings(&argtypes);
	return called_emsg == called_emsg_start ? NOTDONE : FAIL;
    }

    // Parse the arguments for real.
    if (evaluate)
	pnewargs = &newargs;
    else
	pnewargs = NULL;
    *arg += 1;
    ret = get_function_args(arg, equal_arrow ? ')' : '-', pnewargs,
	    types_optional ? &argtypes : NULL, types_optional, evalarg,
					    &varargs, &default_args,
					    FALSE, NULL, NULL);
    if (ret == FAIL
		  || (s = skip_arrow(*arg, equal_arrow, &ret_type,
		equal_arrow || vim9script ? &white_error : NULL)) == NULL)
    {
	if (types_optional)
	    ga_clear_strings(&argtypes);
	ga_clear_strings(&newargs);
	return white_error ? FAIL : NOTDONE;
    }
    *arg = s;

    // Skipping over linebreaks may make "ret_type" invalid, make a copy.
    if (ret_type != NULL)
    {
	ret_type = vim_strsave(ret_type);
	tofree2 = ret_type;
    }

    // Set up a flag for checking local variables and arguments.
    if (evaluate)
	eval_lavars_used = &eval_lavars;

    *arg = skipwhite_and_linebreak(*arg, evalarg);

    // Recognize "{" as the start of a function body.
    if (equal_arrow && **arg == '{')
    {
	if (evalarg == NULL)
	    // cannot happen?
	    goto theend;
	SOURCING_LNUM = start_lnum;  // used for where lambda is defined
	if (lambda_function_body(arg, rettv, evalarg, pnewargs,
			   types_optional ? &argtypes : NULL, varargs,
			   &default_args, ret_type) == FAIL)
	    goto errret;
	goto theend;
    }
    if (default_args.ga_len > 0)
    {
	emsg(_(e_cannot_use_default_values_in_lambda));
	goto errret;
    }

    // Get the start and the end of the expression.
    start = *arg;
    ret = skip_expr_concatenate(arg, &start, &end, evalarg);
    if (ret == FAIL)
	goto errret;
    if (evalarg != NULL)
    {
	// avoid that the expression gets freed when another line break follows
	tofree1 = evalarg->eval_tofree;
	evalarg->eval_tofree = NULL;
    }

    if (!equal_arrow)
    {
	*arg = skipwhite_and_linebreak(*arg, evalarg);
	if (**arg != '}')
	{
	    semsg(_(e_expected_right_curly_str), *arg);
	    goto errret;
	}
	++*arg;
    }

    if (evaluate)
    {
	int	    len;
	int	    flags = FC_LAMBDA;
	char_u	    *p;
	char_u	    *line_end;
	char_u	    *name = get_lambda_name();

	fp = alloc_clear(offsetof(ufunc_T, uf_name) + STRLEN(name) + 1);
	if (fp == NULL)
	    goto errret;
	fp->uf_def_status = UF_NOT_COMPILED;
	pt = ALLOC_CLEAR_ONE(partial_T);
	if (pt == NULL)
	    goto errret;

	ga_init2(&newlines, sizeof(char_u *), 1);
	if (ga_grow(&newlines, 1) == FAIL)
	    goto errret;

	// If there are line breaks, we need to split up the string.
	line_end = vim_strchr(start, '\n');
	if (line_end == NULL || line_end > end)
	    line_end = end;

	// Add "return " before the expression (or the first line).
	len = 7 + (int)(line_end - start) + 1;
	p = alloc(len);
	if (p == NULL)
	    goto errret;
	((char_u **)(newlines.ga_data))[newlines.ga_len++] = p;
	STRCPY(p, "return ");
	vim_strncpy(p + 7, start, line_end - start);

	if (line_end != end)
	{
	    // Add more lines, split by line breaks.  Thus is used when a
	    // lambda with { cmds } is encountered.
	    while (*line_end == '\n')
	    {
		if (ga_grow(&newlines, 1) == FAIL)
		    goto errret;
		start = line_end + 1;
		line_end = vim_strchr(start, '\n');
		if (line_end == NULL)
		    line_end = end;
		((char_u **)(newlines.ga_data))[newlines.ga_len++] =
					 vim_strnsave(start, line_end - start);
	    }
	}

	if (strstr((char *)p + 7, "a:") == NULL)
	    // No a: variables are used for sure.
	    flags |= FC_NOARGS;

	fp->uf_refcount = 1;
	set_ufunc_name(fp, name);
	fp->uf_args = newargs;
	ga_init(&fp->uf_def_args);
	if (types_optional)
	{
	    if (parse_argument_types(fp, &argtypes,
						vim9script && varargs) == FAIL)
		goto errret;
	    if (ret_type != NULL)
	    {
		fp->uf_ret_type = parse_type(&ret_type,
						      &fp->uf_type_list, TRUE);
		if (fp->uf_ret_type == NULL)
		    goto errret;
	    }
	    else
		fp->uf_ret_type = &t_unknown;
	}

	fp->uf_lines = newlines;
	if (current_funccal != NULL && eval_lavars)
	{
	    flags |= FC_CLOSURE;
	    if (register_closure(fp) == FAIL)
		goto errret;
	}

#ifdef FEAT_PROFILE
	if (prof_def_func())
	    func_do_profile(fp);
#endif
	if (sandbox)
	    flags |= FC_SANDBOX;
	// In legacy script a lambda can be called with more args than
	// uf_args.ga_len.  In Vim9 script "...name" has to be used.
	fp->uf_varargs = !vim9script || varargs;
	fp->uf_flags = flags;
	fp->uf_calls = 0;
	fp->uf_script_ctx = current_sctx;
	// Use the line number of the arguments.
	fp->uf_script_ctx.sc_lnum += start_lnum;

	function_using_block_scopes(fp, evalarg->eval_cstack);

	pt->pt_func = fp;
	pt->pt_refcount = 1;
	rettv->vval.v_partial = pt;
	rettv->v_type = VAR_PARTIAL;

	hash_add(&func_hashtab, UF2HIKEY(fp));
    }

theend:
    eval_lavars_used = old_eval_lavars;
    if (evalarg != NULL && evalarg->eval_tofree == NULL)
	evalarg->eval_tofree = tofree1;
    else
	vim_free(tofree1);
    vim_free(tofree2);
    if (types_optional)
	ga_clear_strings(&argtypes);

    return OK;

errret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
    ga_clear_strings(&default_args);
    if (types_optional)
    {
	ga_clear_strings(&argtypes);
	if (fp != NULL)
	    vim_free(fp->uf_arg_types);
    }
    vim_free(fp);
    vim_free(pt);
    if (evalarg != NULL && evalarg->eval_tofree == NULL)
	evalarg->eval_tofree = tofree1;
    else
	vim_free(tofree1);
    vim_free(tofree2);
    eval_lavars_used = old_eval_lavars;
    return FAIL;
}