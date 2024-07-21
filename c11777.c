compile_nested_function(exarg_T *eap, cctx_T *cctx, garray_T *lines_to_free)
{
    int		is_global = *eap->arg == 'g' && eap->arg[1] == ':';
    char_u	*name_start = eap->arg;
    char_u	*name_end = to_name_end(eap->arg, TRUE);
    int		off;
    char_u	*func_name;
    char_u	*lambda_name;
    ufunc_T	*ufunc;
    int		r = FAIL;
    compiletype_T   compile_type;
    isn_T	*funcref_isn = NULL;

    if (eap->forceit)
    {
	emsg(_(e_cannot_use_bang_with_nested_def));
	return NULL;
    }

    if (*name_start == '/')
    {
	name_end = skip_regexp(name_start + 1, '/', TRUE);
	if (*name_end == '/')
	    ++name_end;
	set_nextcmd(eap, name_end);
    }
    if (name_end == name_start || *skipwhite(name_end) != '(')
    {
	if (!ends_excmd2(name_start, name_end))
	{
	    if (*skipwhite(name_end) == '.')
		semsg(_(e_cannot_define_dict_func_in_vim9_script_str),
								     eap->cmd);
	    else
		semsg(_(e_invalid_command_str), eap->cmd);
	    return NULL;
	}

	// "def" or "def Name": list functions
	if (generate_DEF(cctx, name_start, name_end - name_start) == FAIL)
	    return NULL;
	return eap->nextcmd == NULL ? (char_u *)"" : eap->nextcmd;
    }

    // Only g:Func() can use a namespace.
    if (name_start[1] == ':' && !is_global)
    {
	semsg(_(e_namespace_not_supported_str), name_start);
	return NULL;
    }
    if (cctx->ctx_skip != SKIP_YES
	    && check_defined(name_start, name_end - name_start, cctx,
							  NULL, FALSE) == FAIL)
	return NULL;
    if (!ASCII_ISUPPER(is_global ? name_start[2] : name_start[0]))
    {
	semsg(_(e_function_name_must_start_with_capital_str), name_start);
	return NULL;
    }

    eap->arg = name_end;
    fill_exarg_from_cctx(eap, cctx);

    eap->forceit = FALSE;
    // We use the special <Lamba>99 name, but it's not really a lambda.
    lambda_name = vim_strsave(get_lambda_name());
    if (lambda_name == NULL)
	return NULL;

    // This may free the current line, make a copy of the name.
    off = is_global ? 2 : 0;
    func_name = vim_strnsave(name_start + off, name_end - name_start - off);
    if (func_name == NULL)
    {
	r = FAIL;
	goto theend;
    }

    ufunc = define_function(eap, lambda_name, lines_to_free);
    if (ufunc == NULL)
    {
	r = eap->skip ? OK : FAIL;
	goto theend;
    }
    if (eap->nextcmd != NULL)
    {
	semsg(_(e_text_found_after_str_str),
	      eap->cmdidx == CMD_def ? "enddef" : "endfunction", eap->nextcmd);
	r = FAIL;
	func_ptr_unref(ufunc);
	goto theend;
    }

    // copy over the block scope IDs before compiling
    if (!is_global && cctx->ctx_ufunc->uf_block_depth > 0)
    {
	int block_depth = cctx->ctx_ufunc->uf_block_depth;

	ufunc->uf_block_ids = ALLOC_MULT(int, block_depth);
	if (ufunc->uf_block_ids != NULL)
	{
	    mch_memmove(ufunc->uf_block_ids, cctx->ctx_ufunc->uf_block_ids,
						    sizeof(int) * block_depth);
	    ufunc->uf_block_depth = block_depth;
	}
    }

    // Define the funcref before compiling, so that it is found by any
    // recursive call.
    if (is_global)
    {
	r = generate_NEWFUNC(cctx, lambda_name, func_name);
	func_name = NULL;
	lambda_name = NULL;
    }
    else
    {
	// Define a local variable for the function reference.
	lvar_T	*lvar = reserve_local(cctx, func_name, name_end - name_start,
						    TRUE, ufunc->uf_func_type);

	if (lvar == NULL)
	    goto theend;
	if (generate_FUNCREF(cctx, ufunc, &funcref_isn) == FAIL)
	    goto theend;
	r = generate_STORE(cctx, ISN_STORE, lvar->lv_idx, NULL);
    }

    compile_type = get_compile_type(ufunc);
#ifdef FEAT_PROFILE
    // If the outer function is profiled, also compile the nested function for
    // profiling.
    if (cctx->ctx_compile_type == CT_PROFILE)
	compile_type = CT_PROFILE;
#endif
    if (func_needs_compiling(ufunc, compile_type)
	    && compile_def_function(ufunc, TRUE, compile_type, cctx) == FAIL)
    {
	func_ptr_unref(ufunc);
	goto theend;
    }

#ifdef FEAT_PROFILE
    // When the outer function is compiled for profiling, the nested function
    // may be called without profiling.  Compile it here in the right context.
    if (compile_type == CT_PROFILE && func_needs_compiling(ufunc, CT_NONE))
	compile_def_function(ufunc, FALSE, CT_NONE, cctx);
#endif

    // If a FUNCREF instruction was generated, set the index after compiling.
    if (funcref_isn != NULL && ufunc->uf_def_status == UF_COMPILED)
	funcref_isn->isn_arg.funcref.fr_dfunc_idx = ufunc->uf_dfunc_idx;

theend:
    vim_free(lambda_name);
    vim_free(func_name);
    return r == FAIL ? NULL : (char_u *)"";
}