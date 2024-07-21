compile_def_function(
	ufunc_T		*ufunc,
	int		check_return_type,
	compiletype_T   compile_type,
	cctx_T		*outer_cctx)
{
    char_u	*line = NULL;
    char_u	*line_to_free = NULL;
    char_u	*p;
    char	*errormsg = NULL;	// error message
    cctx_T	cctx;
    garray_T	*instr;
    int		did_emsg_before = did_emsg;
    int		did_emsg_silent_before = did_emsg_silent;
    int		ret = FAIL;
    sctx_T	save_current_sctx = current_sctx;
    int		save_estack_compiling = estack_compiling;
    int		save_cmod_flags = cmdmod.cmod_flags;
    int		do_estack_push;
    int		new_def_function = FALSE;
#ifdef FEAT_PROFILE
    int		prof_lnum = -1;
#endif
    int		debug_lnum = -1;

    // When using a function that was compiled before: Free old instructions.
    // The index is reused.  Otherwise add a new entry in "def_functions".
    if (ufunc->uf_dfunc_idx > 0)
    {
	dfunc_T *dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	isn_T	*instr_dest = NULL;

	switch (compile_type)
	{
	    case CT_PROFILE:
#ifdef FEAT_PROFILE
			    instr_dest = dfunc->df_instr_prof; break;
#endif
	    case CT_NONE:   instr_dest = dfunc->df_instr; break;
	    case CT_DEBUG:  instr_dest = dfunc->df_instr_debug; break;
	}
	if (instr_dest != NULL)
	    // Was compiled in this mode before: Free old instructions.
	    delete_def_function_contents(dfunc, FALSE);
	ga_clear_strings(&dfunc->df_var_names);
    }
    else
    {
	if (add_def_function(ufunc) == FAIL)
	    return FAIL;
	new_def_function = TRUE;
    }

    ufunc->uf_def_status = UF_COMPILING;

    CLEAR_FIELD(cctx);

    cctx.ctx_compile_type = compile_type;
    cctx.ctx_ufunc = ufunc;
    cctx.ctx_lnum = -1;
    cctx.ctx_outer = outer_cctx;
    ga_init2(&cctx.ctx_locals, sizeof(lvar_T), 10);
    // Each entry on the type stack consists of two type pointers.
    ga_init2(&cctx.ctx_type_stack, sizeof(type2_T), 50);
    ga_init2(&cctx.ctx_imports, sizeof(imported_T), 10);
    cctx.ctx_type_list = &ufunc->uf_type_list;
    ga_init2(&cctx.ctx_instr, sizeof(isn_T), 50);
    instr = &cctx.ctx_instr;

    // Set the context to the function, it may be compiled when called from
    // another script.  Set the script version to the most modern one.
    // The line number will be set in next_line_from_context().
    current_sctx = ufunc->uf_script_ctx;
    current_sctx.sc_version = SCRIPT_VERSION_VIM9;

    // Don't use the flag from ":legacy" here.
    cmdmod.cmod_flags &= ~CMOD_LEGACY;

    // Make sure error messages are OK.
    do_estack_push = !estack_top_is_ufunc(ufunc, 1);
    if (do_estack_push)
	estack_push_ufunc(ufunc, 1);
    estack_compiling = TRUE;

    if (check_args_shadowing(ufunc, &cctx) == FAIL)
	goto erret;

    if (ufunc->uf_def_args.ga_len > 0)
    {
	int	count = ufunc->uf_def_args.ga_len;
	int	first_def_arg = ufunc->uf_args.ga_len - count;
	int	i;
	char_u	*arg;
	int	off = STACK_FRAME_SIZE + (ufunc->uf_va_name != NULL ? 1 : 0);
	int	did_set_arg_type = FALSE;

	// Produce instructions for the default values of optional arguments.
	SOURCING_LNUM = 0;  // line number unknown
	for (i = 0; i < count; ++i)
	{
	    type_T	*val_type;
	    int		arg_idx = first_def_arg + i;
	    where_T	where = WHERE_INIT;
	    int		r;
	    int		jump_instr_idx = instr->ga_len;
	    isn_T	*isn;

	    // Use a JUMP_IF_ARG_SET instruction to skip if the value was given.
	    if (generate_JUMP_IF_ARG_SET(&cctx, i - count - off) == FAIL)
		goto erret;

	    // Make sure later arguments are not found.
	    ufunc->uf_args_visible = arg_idx;

	    arg = ((char_u **)(ufunc->uf_def_args.ga_data))[i];
	    r = compile_expr0(&arg, &cctx);

	    if (r == FAIL)
		goto erret;

	    // If no type specified use the type of the default value.
	    // Otherwise check that the default value type matches the
	    // specified type.
	    val_type = get_type_on_stack(&cctx, 0);
	    where.wt_index = arg_idx + 1;
	    if (ufunc->uf_arg_types[arg_idx] == &t_unknown)
	    {
		did_set_arg_type = TRUE;
		ufunc->uf_arg_types[arg_idx] = val_type;
	    }
	    else if (need_type_where(val_type, ufunc->uf_arg_types[arg_idx],
				       -1, where, &cctx, FALSE, FALSE) == FAIL)
		goto erret;

	    if (generate_STORE(&cctx, ISN_STORE, i - count - off, NULL) == FAIL)
		goto erret;

	    // set instruction index in JUMP_IF_ARG_SET to here
	    isn = ((isn_T *)instr->ga_data) + jump_instr_idx;
	    isn->isn_arg.jumparg.jump_where = instr->ga_len;
	}

	if (did_set_arg_type)
	    set_function_type(ufunc);
    }
    ufunc->uf_args_visible = ufunc->uf_args.ga_len;

    /*
     * Loop over all the lines of the function and generate instructions.
     */
    for (;;)
    {
	exarg_T	    ea;
	int	    starts_with_colon = FALSE;
	char_u	    *cmd;
	cmdmod_T    local_cmdmod;

	// Bail out on the first error to avoid a flood of errors and report
	// the right line number when inside try/catch.
	if (did_emsg_before != did_emsg)
	    goto erret;

	if (line != NULL && *line == '|')
	    // the line continues after a '|'
	    ++line;
	else if (line != NULL && *skipwhite(line) != NUL
		&& !(*line == '#' && (line == cctx.ctx_line_start
						    || VIM_ISWHITE(line[-1]))))
	{
	    semsg(_(e_trailing_characters_str), line);
	    goto erret;
	}
	else if (line != NULL && vim9_bad_comment(skipwhite(line)))
	    goto erret;
	else
	{
	    line = next_line_from_context(&cctx, FALSE);
	    if (cctx.ctx_lnum >= ufunc->uf_lines.ga_len)
	    {
		// beyond the last line
#ifdef FEAT_PROFILE
		if (cctx.ctx_skip != SKIP_YES)
		    may_generate_prof_end(&cctx, prof_lnum);
#endif
		break;
	    }
	    // Make a copy, splitting off nextcmd and removing trailing spaces
	    // may change it.
	    if (line != NULL)
	    {
		line = vim_strsave(line);
		vim_free(line_to_free);
		line_to_free = line;
	    }
	}

	CLEAR_FIELD(ea);
	ea.cmdlinep = &line;
	ea.cmd = skipwhite(line);

	if (*ea.cmd == '#')
	{
	    // "#" starts a comment
	    line = (char_u *)"";
	    continue;
	}

#ifdef FEAT_PROFILE
	if (cctx.ctx_compile_type == CT_PROFILE && cctx.ctx_lnum != prof_lnum
						  && cctx.ctx_skip != SKIP_YES)
	{
	    may_generate_prof_end(&cctx, prof_lnum);

	    prof_lnum = cctx.ctx_lnum;
	    generate_instr(&cctx, ISN_PROF_START);
	}
#endif
	if (cctx.ctx_compile_type == CT_DEBUG && cctx.ctx_lnum != debug_lnum
						  && cctx.ctx_skip != SKIP_YES)
	{
	    debug_lnum = cctx.ctx_lnum;
	    generate_instr_debug(&cctx);
	}
	cctx.ctx_prev_lnum = cctx.ctx_lnum + 1;

	// Some things can be recognized by the first character.
	switch (*ea.cmd)
	{
	    case '}':
		{
		    // "}" ends a block scope
		    scopetype_T stype = cctx.ctx_scope == NULL
					  ? NO_SCOPE : cctx.ctx_scope->se_type;

		    if (stype == BLOCK_SCOPE)
		    {
			compile_endblock(&cctx);
			line = ea.cmd;
		    }
		    else
		    {
			emsg(_(e_using_rcurly_outside_if_block_scope));
			goto erret;
		    }
		    if (line != NULL)
			line = skipwhite(ea.cmd + 1);
		    continue;
		}

	    case '{':
		// "{" starts a block scope
		// "{'a': 1}->func() is something else
		if (ends_excmd(*skipwhite(ea.cmd + 1)))
		{
		    line = compile_block(ea.cmd, &cctx);
		    continue;
		}
		break;
	}

	/*
	 * COMMAND MODIFIERS
	 */
	cctx.ctx_has_cmdmod = FALSE;
	if (parse_command_modifiers(&ea, &errormsg, &local_cmdmod, FALSE)
								       == FAIL)
	{
	    if (errormsg != NULL)
		goto erret;
	    // empty line or comment
	    line = (char_u *)"";
	    continue;
	}
	generate_cmdmods(&cctx, &local_cmdmod);
	undo_cmdmod(&local_cmdmod);

	// Check if there was a colon after the last command modifier or before
	// the current position.
	for (p = ea.cmd; p >= line; --p)
	{
	    if (*p == ':')
		starts_with_colon = TRUE;
	    if (p < ea.cmd && !VIM_ISWHITE(*p))
		break;
	}

	// Skip ":call" to get to the function name, unless using :legacy
	p = ea.cmd;
	if (!(local_cmdmod.cmod_flags & CMOD_LEGACY))
	{
	    if (checkforcmd(&ea.cmd, "call", 3))
	    {
		if (*ea.cmd == '(')
		    // not for "call()"
		    ea.cmd = p;
		else
		    ea.cmd = skipwhite(ea.cmd);
	    }

	    if (!starts_with_colon)
	    {
		int	    assign;

		// Check for assignment after command modifiers.
		assign = may_compile_assignment(&ea, &line, &cctx);
		if (assign == OK)
		    goto nextline;
		if (assign == FAIL)
		    goto erret;
	    }
	}

	/*
	 * COMMAND after range
	 * 'text'->func() should not be confused with 'a mark
	 * "++nr" and "--nr" are eval commands
	 * in "$ENV->func()" the "$" is not a range
	 */
	cmd = ea.cmd;
	if ((*cmd != '$' || starts_with_colon)
		&& (starts_with_colon || !(*cmd == '\''
		       || (cmd[0] == cmd[1] && (*cmd == '+' || *cmd == '-')))))
	{
	    ea.cmd = skip_range(ea.cmd, TRUE, NULL);
	    if (ea.cmd > cmd)
	    {
		if (!starts_with_colon
				   && !(local_cmdmod.cmod_flags & CMOD_LEGACY))
		{
		    semsg(_(e_colon_required_before_range_str), cmd);
		    goto erret;
		}
		ea.addr_count = 1;
		if (ends_excmd2(line, ea.cmd))
		{
		    // A range without a command: jump to the line.
		    generate_EXEC(&cctx, ISN_EXECRANGE,
					      vim_strnsave(cmd, ea.cmd - cmd));
		    line = ea.cmd;
		    goto nextline;
		}
	    }
	}
	p = find_ex_command(&ea, NULL,
		starts_with_colon || (local_cmdmod.cmod_flags & CMOD_LEGACY)
						  ? NULL : item_exists, &cctx);

	if (p == NULL)
	{
	    if (cctx.ctx_skip != SKIP_YES)
		emsg(_(e_ambiguous_use_of_user_defined_command));
	    goto erret;
	}

	// When using ":legacy cmd" always use compile_exec().
	if (local_cmdmod.cmod_flags & CMOD_LEGACY)
	{
	    char_u *start = ea.cmd;

	    switch (ea.cmdidx)
	    {
		case CMD_if:
		case CMD_elseif:
		case CMD_else:
		case CMD_endif:
		case CMD_for:
		case CMD_endfor:
		case CMD_continue:
		case CMD_break:
		case CMD_while:
		case CMD_endwhile:
		case CMD_try:
		case CMD_catch:
		case CMD_finally:
		case CMD_endtry:
			semsg(_(e_cannot_use_legacy_with_command_str), ea.cmd);
			goto erret;
		default: break;
	    }

	    // ":legacy return expr" needs to be handled differently.
	    if (checkforcmd(&start, "return", 4))
		ea.cmdidx = CMD_return;
	    else
		ea.cmdidx = CMD_legacy;
	}

	if (p == ea.cmd && ea.cmdidx != CMD_SIZE)
	{
	    if (cctx.ctx_skip == SKIP_YES && ea.cmdidx != CMD_eval)
	    {
		line += STRLEN(line);
		goto nextline;
	    }
	    else if (ea.cmdidx != CMD_eval)
	    {
		// CMD_var cannot happen, compile_assignment() above would be
		// used.  Most likely an assignment to a non-existing variable.
		semsg(_(e_command_not_recognized_str), ea.cmd);
		goto erret;
	    }
	}

	if (cctx.ctx_had_return
		&& ea.cmdidx != CMD_elseif
		&& ea.cmdidx != CMD_else
		&& ea.cmdidx != CMD_endif
		&& ea.cmdidx != CMD_endfor
		&& ea.cmdidx != CMD_endwhile
		&& ea.cmdidx != CMD_catch
		&& ea.cmdidx != CMD_finally
		&& ea.cmdidx != CMD_endtry)
	{
	    emsg(_(e_unreachable_code_after_return));
	    goto erret;
	}

	p = skipwhite(p);
	if (ea.cmdidx != CMD_SIZE
			    && ea.cmdidx != CMD_write && ea.cmdidx != CMD_read)
	{
	    if (ea.cmdidx >= 0)
		ea.argt = excmd_get_argt(ea.cmdidx);
	    if ((ea.argt & EX_BANG) && *p == '!')
	    {
		ea.forceit = TRUE;
		p = skipwhite(p + 1);
	    }
	}

	switch (ea.cmdidx)
	{
	    case CMD_def:
	    case CMD_function:
		    ea.arg = p;
		    line = compile_nested_function(&ea, &cctx, &line_to_free);
		    break;

	    case CMD_return:
		    line = compile_return(p, check_return_type,
				 local_cmdmod.cmod_flags & CMOD_LEGACY, &cctx);
		    cctx.ctx_had_return = TRUE;
		    break;

	    case CMD_let:
		    emsg(_(e_cannot_use_let_in_vim9_script));
		    break;
	    case CMD_var:
	    case CMD_final:
	    case CMD_const:
	    case CMD_increment:
	    case CMD_decrement:
		    line = compile_assignment(p, &ea, ea.cmdidx, &cctx);
		    if (line == p)
			line = NULL;
		    break;

	    case CMD_unlet:
	    case CMD_unlockvar:
	    case CMD_lockvar:
		    line = compile_unletlock(p, &ea, &cctx);
		    break;

	    case CMD_import:
		    emsg(_(e_import_can_only_be_used_in_script));
		    line = NULL;
		    break;

	    case CMD_if:
		    line = compile_if(p, &cctx);
		    break;
	    case CMD_elseif:
		    line = compile_elseif(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_else:
		    line = compile_else(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_endif:
		    line = compile_endif(p, &cctx);
		    break;

	    case CMD_while:
		    line = compile_while(p, &cctx);
		    break;
	    case CMD_endwhile:
		    line = compile_endwhile(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;

	    case CMD_for:
		    line = compile_for(p, &cctx);
		    break;
	    case CMD_endfor:
		    line = compile_endfor(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_continue:
		    line = compile_continue(p, &cctx);
		    break;
	    case CMD_break:
		    line = compile_break(p, &cctx);
		    break;

	    case CMD_try:
		    line = compile_try(p, &cctx);
		    break;
	    case CMD_catch:
		    line = compile_catch(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_finally:
		    line = compile_finally(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_endtry:
		    line = compile_endtry(p, &cctx);
		    cctx.ctx_had_return = FALSE;
		    break;
	    case CMD_throw:
		    line = compile_throw(p, &cctx);
		    break;

	    case CMD_eval:
		    line = compile_eval(p, &cctx);
		    break;

	    case CMD_echo:
	    case CMD_echon:
	    case CMD_execute:
	    case CMD_echomsg:
	    case CMD_echoerr:
	    case CMD_echoconsole:
		    line = compile_mult_expr(p, ea.cmdidx, &cctx);
		    break;

	    case CMD_put:
		    ea.cmd = cmd;
		    line = compile_put(p, &ea, &cctx);
		    break;

	    case CMD_substitute:
		    if (check_global_and_subst(ea.cmd, p) == FAIL)
			goto erret;
		    if (cctx.ctx_skip == SKIP_YES)
			line = (char_u *)"";
		    else
		    {
			ea.arg = p;
			line = compile_substitute(line, &ea, &cctx);
		    }
		    break;

	    case CMD_redir:
		    ea.arg = p;
		    line = compile_redir(line, &ea, &cctx);
		    break;

	    case CMD_cexpr:
	    case CMD_lexpr:
	    case CMD_caddexpr:
	    case CMD_laddexpr:
	    case CMD_cgetexpr:
	    case CMD_lgetexpr:
#ifdef FEAT_QUICKFIX
		    ea.arg = p;
		    line = compile_cexpr(line, &ea, &cctx);
#else
		    ex_ni(&ea);
		    line = NULL;
#endif
		    break;

	    case CMD_append:
	    case CMD_change:
	    case CMD_insert:
	    case CMD_k:
	    case CMD_t:
	    case CMD_xit:
		    not_in_vim9(&ea);
		    goto erret;

	    case CMD_SIZE:
		    if (cctx.ctx_skip != SKIP_YES)
		    {
			semsg(_(e_invalid_command_str), ea.cmd);
			goto erret;
		    }
		    // We don't check for a next command here.
		    line = (char_u *)"";
		    break;

	    case CMD_lua:
	    case CMD_mzscheme:
	    case CMD_perl:
	    case CMD_py3:
	    case CMD_python3:
	    case CMD_python:
	    case CMD_pythonx:
	    case CMD_ruby:
	    case CMD_tcl:
		    ea.arg = p;
		    if (vim_strchr(line, '\n') == NULL)
			line = compile_exec(line, &ea, &cctx);
		    else
			// heredoc lines have been concatenated with NL
			// characters in get_function_body()
			line = compile_script(line, &cctx);
		    break;

	    case CMD_global:
		    if (check_global_and_subst(ea.cmd, p) == FAIL)
			goto erret;
		    // FALLTHROUGH
	    default:
		    // Not recognized, execute with do_cmdline_cmd().
		    ea.arg = p;
		    line = compile_exec(line, &ea, &cctx);
		    break;
	}
nextline:
	if (line == NULL)
	    goto erret;
	line = skipwhite(line);

	// Undo any command modifiers.
	generate_undo_cmdmods(&cctx);

	if (cctx.ctx_type_stack.ga_len < 0)
	{
	    iemsg("Type stack underflow");
	    goto erret;
	}
    }

    if (cctx.ctx_scope != NULL)
    {
	if (cctx.ctx_scope->se_type == IF_SCOPE)
	    emsg(_(e_missing_endif));
	else if (cctx.ctx_scope->se_type == WHILE_SCOPE)
	    emsg(_(e_missing_endwhile));
	else if (cctx.ctx_scope->se_type == FOR_SCOPE)
	    emsg(_(e_missing_endfor));
	else
	    emsg(_(e_missing_rcurly));
	goto erret;
    }

    if (!cctx.ctx_had_return)
    {
	if (ufunc->uf_ret_type->tt_type == VAR_UNKNOWN)
	    ufunc->uf_ret_type = &t_void;
	else if (ufunc->uf_ret_type->tt_type != VAR_VOID)
	{
	    emsg(_(e_missing_return_statement));
	    goto erret;
	}

	// Return void if there is no return at the end.
	generate_instr(&cctx, ISN_RETURN_VOID);
    }

    // When compiled with ":silent!" and there was an error don't consider the
    // function compiled.
    if (emsg_silent == 0 || did_emsg_silent == did_emsg_silent_before)
    {
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;
	dfunc->df_deleted = FALSE;
	dfunc->df_script_seq = current_sctx.sc_seq;
#ifdef FEAT_PROFILE
	if (cctx.ctx_compile_type == CT_PROFILE)
	{
	    dfunc->df_instr_prof = instr->ga_data;
	    dfunc->df_instr_prof_count = instr->ga_len;
	}
	else
#endif
	if (cctx.ctx_compile_type == CT_DEBUG)
	{
	    dfunc->df_instr_debug = instr->ga_data;
	    dfunc->df_instr_debug_count = instr->ga_len;
	}
	else
	{
	    dfunc->df_instr = instr->ga_data;
	    dfunc->df_instr_count = instr->ga_len;
	}
	dfunc->df_varcount = dfunc->df_var_names.ga_len;
	dfunc->df_has_closure = cctx.ctx_has_closure;
	if (cctx.ctx_outer_used)
	    ufunc->uf_flags |= FC_CLOSURE;
	ufunc->uf_def_status = UF_COMPILED;
    }

    ret = OK;

erret:
    if (ufunc->uf_def_status == UF_COMPILING)
    {
	dfunc_T	*dfunc = ((dfunc_T *)def_functions.ga_data)
							 + ufunc->uf_dfunc_idx;

	// Compiling aborted, free the generated instructions.
	clear_instr_ga(instr);
	VIM_CLEAR(dfunc->df_name);
	ga_clear_strings(&dfunc->df_var_names);

	// If using the last entry in the table and it was added above, we
	// might as well remove it.
	if (!dfunc->df_deleted && new_def_function
			    && ufunc->uf_dfunc_idx == def_functions.ga_len - 1)
	{
	    --def_functions.ga_len;
	    ufunc->uf_dfunc_idx = 0;
	}
	ufunc->uf_def_status = UF_COMPILE_ERROR;

	while (cctx.ctx_scope != NULL)
	    drop_scope(&cctx);

	if (errormsg != NULL)
	    emsg(errormsg);
	else if (did_emsg == did_emsg_before)
	    emsg(_(e_compiling_def_function_failed));
    }

    if (cctx.ctx_redir_lhs.lhs_name != NULL)
    {
	if (ret == OK)
	{
	    emsg(_(e_missing_redir_end));
	    ret = FAIL;
	}
	vim_free(cctx.ctx_redir_lhs.lhs_name);
	vim_free(cctx.ctx_redir_lhs.lhs_whole);
    }

    current_sctx = save_current_sctx;
    estack_compiling = save_estack_compiling;
    cmdmod.cmod_flags =	save_cmod_flags;
    if (do_estack_push)
	estack_pop();

    vim_free(line_to_free);
    free_imported(&cctx);
    free_locals(&cctx);
    ga_clear(&cctx.ctx_type_stack);
    return ret;
}