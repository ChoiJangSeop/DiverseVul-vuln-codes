eval0_retarg(
    char_u	*arg,
    typval_T	*rettv,
    exarg_T	*eap,
    evalarg_T	*evalarg,
    char_u	**retarg)
{
    int		ret;
    char_u	*p;
    char_u	*expr_end;
    int		did_emsg_before = did_emsg;
    int		called_emsg_before = called_emsg;
    int		flags = evalarg == NULL ? 0 : evalarg->eval_flags;
    int		check_for_end = retarg == NULL;
    int		end_error = FALSE;

    p = skipwhite(arg);
    ret = eval1(&p, rettv, evalarg);
    expr_end = p;
    p = skipwhite(p);

    // In Vim9 script a command block is not split at NL characters for
    // commands using an expression argument.  Skip over a '#' comment to check
    // for a following NL.  Require white space before the '#'.
    if (in_vim9script() && p > expr_end && retarg == NULL)
	while (*p == '#')
	{
	    char_u *nl = vim_strchr(p, NL);

	    if (nl == NULL)
		break;
	    p = skipwhite(nl + 1);
	    if (eap != NULL && *p != NUL)
		eap->nextcmd = p;
	    check_for_end = FALSE;
	}

    if (ret != FAIL && check_for_end)
	end_error = !ends_excmd2(arg, p);
    if (ret == FAIL || end_error)
    {
	if (ret != FAIL)
	    clear_tv(rettv);
	/*
	 * Report the invalid expression unless the expression evaluation has
	 * been cancelled due to an aborting error, an interrupt, or an
	 * exception, or we already gave a more specific error.
	 * Also check called_emsg for when using assert_fails().
	 */
	if (!aborting()
		&& did_emsg == did_emsg_before
		&& called_emsg == called_emsg_before
		&& (flags & EVAL_CONSTANT) == 0
		&& (!in_vim9script() || !vim9_bad_comment(p)))
	{
	    if (end_error)
		semsg(_(e_trailing_characters_str), p);
	    else
		semsg(_(e_invalid_expression_str), arg);
	}

	// Some of the expression may not have been consumed.  Do not check for
	// a next command to avoid more errors, unless "|" is following, which
	// could only be a command separator.
	if (eap != NULL && skipwhite(p)[0] == '|' && skipwhite(p)[1] != '|')
	    eap->nextcmd = check_nextcmd(p);
	return FAIL;
    }

    if (retarg != NULL)
	*retarg = p;
    else if (check_for_end && eap != NULL)
	set_nextcmd(eap, p);

    return ret;
}