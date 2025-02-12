do_one_cmd(
    char_u		**cmdlinep,
    int			sourcing,
#ifdef FEAT_EVAL
    struct condstack	*cstack,
#endif
    char_u		*(*fgetline)(int, void *, int),
    void		*cookie)		/* argument for fgetline() */
{
    char_u		*p;
    linenr_T		lnum;
    long		n;
    char		*errormsg = NULL;	/* error message */
    char_u		*after_modifier = NULL;
    exarg_T		ea;			/* Ex command arguments */
    int			save_msg_scroll = msg_scroll;
    cmdmod_T		save_cmdmod;
    int			ni;			/* set when Not Implemented */
    char_u		*cmd;

    vim_memset(&ea, 0, sizeof(ea));
    ea.line1 = 1;
    ea.line2 = 1;
#ifdef FEAT_EVAL
    ++ex_nesting_level;
#endif

    /* When the last file has not been edited :q has to be typed twice. */
    if (quitmore
#ifdef FEAT_EVAL
	    /* avoid that a function call in 'statusline' does this */
	    && !getline_equal(fgetline, cookie, get_func_line)
#endif
	    /* avoid that an autocommand, e.g. QuitPre, does this */
	    && !getline_equal(fgetline, cookie, getnextac))
	--quitmore;

    /*
     * Reset browse, confirm, etc..  They are restored when returning, for
     * recursive calls.
     */
    save_cmdmod = cmdmod;

    /* "#!anything" is handled like a comment. */
    if ((*cmdlinep)[0] == '#' && (*cmdlinep)[1] == '!')
	goto doend;

/*
 * 1. Skip comment lines and leading white space and colons.
 * 2. Handle command modifiers.
 */
    // The "ea" structure holds the arguments that can be used.
    ea.cmd = *cmdlinep;
    ea.cmdlinep = cmdlinep;
    ea.getline = fgetline;
    ea.cookie = cookie;
#ifdef FEAT_EVAL
    ea.cstack = cstack;
#endif
    if (parse_command_modifiers(&ea, &errormsg, FALSE) == FAIL)
	goto doend;

    after_modifier = ea.cmd;

#ifdef FEAT_EVAL
    ea.skip = did_emsg || got_int || did_throw || (cstack->cs_idx >= 0
			 && !(cstack->cs_flags[cstack->cs_idx] & CSF_ACTIVE));
#else
    ea.skip = (if_level > 0);
#endif

/*
 * 3. Skip over the range to find the command.  Let "p" point to after it.
 *
 * We need the command to know what kind of range it uses.
 */
    cmd = ea.cmd;
    ea.cmd = skip_range(ea.cmd, NULL);
    if (*ea.cmd == '*' && vim_strchr(p_cpo, CPO_STAR) == NULL)
	ea.cmd = skipwhite(ea.cmd + 1);
    p = find_command(&ea, NULL);

#ifdef FEAT_EVAL
# ifdef FEAT_PROFILE
    // Count this line for profiling if skip is TRUE.
    if (do_profiling == PROF_YES
	    && (!ea.skip || cstack->cs_idx == 0 || (cstack->cs_idx > 0
		     && (cstack->cs_flags[cstack->cs_idx - 1] & CSF_ACTIVE))))
    {
	int skip = did_emsg || got_int || did_throw;

	if (ea.cmdidx == CMD_catch)
	    skip = !skip && !(cstack->cs_idx >= 0
			  && (cstack->cs_flags[cstack->cs_idx] & CSF_THROWN)
			  && !(cstack->cs_flags[cstack->cs_idx] & CSF_CAUGHT));
	else if (ea.cmdidx == CMD_else || ea.cmdidx == CMD_elseif)
	    skip = skip || !(cstack->cs_idx >= 0
			  && !(cstack->cs_flags[cstack->cs_idx]
						  & (CSF_ACTIVE | CSF_TRUE)));
	else if (ea.cmdidx == CMD_finally)
	    skip = FALSE;
	else if (ea.cmdidx != CMD_endif
		&& ea.cmdidx != CMD_endfor
		&& ea.cmdidx != CMD_endtry
		&& ea.cmdidx != CMD_endwhile)
	    skip = ea.skip;

	if (!skip)
	{
	    if (getline_equal(fgetline, cookie, get_func_line))
		func_line_exec(getline_cookie(fgetline, cookie));
	    else if (getline_equal(fgetline, cookie, getsourceline))
		script_line_exec();
	}
    }
# endif

    /* May go to debug mode.  If this happens and the ">quit" debug command is
     * used, throw an interrupt exception and skip the next command. */
    dbg_check_breakpoint(&ea);
    if (!ea.skip && got_int)
    {
	ea.skip = TRUE;
	(void)do_intthrow(cstack);
    }
#endif

/*
 * 4. parse a range specifier of the form: addr [,addr] [;addr] ..
 *
 * where 'addr' is:
 *
 * %	      (entire file)
 * $  [+-NUM]
 * 'x [+-NUM] (where x denotes a currently defined mark)
 * .  [+-NUM]
 * [+-NUM]..
 * NUM
 *
 * The ea.cmd pointer is updated to point to the first character following the
 * range spec. If an initial address is found, but no second, the upper bound
 * is equal to the lower.
 */

    /* ea.addr_type for user commands is set by find_ucmd */
    if (!IS_USER_CMDIDX(ea.cmdidx))
    {
	if (ea.cmdidx != CMD_SIZE)
	    ea.addr_type = cmdnames[(int)ea.cmdidx].cmd_addr_type;
	else
	    ea.addr_type = ADDR_LINES;

	/* :wincmd range depends on the argument. */
	if (ea.cmdidx == CMD_wincmd && p != NULL)
	    get_wincmd_addr_type(skipwhite(p), &ea);
    }

    ea.cmd = cmd;
    if (parse_cmd_address(&ea, &errormsg, FALSE) == FAIL)
	goto doend;

/*
 * 5. Parse the command.
 */

    /*
     * Skip ':' and any white space
     */
    ea.cmd = skipwhite(ea.cmd);
    while (*ea.cmd == ':')
	ea.cmd = skipwhite(ea.cmd + 1);

    /*
     * If we got a line, but no command, then go to the line.
     * If we find a '|' or '\n' we set ea.nextcmd.
     */
    if (*ea.cmd == NUL || *ea.cmd == '"'
			      || (ea.nextcmd = check_nextcmd(ea.cmd)) != NULL)
    {
	/*
	 * strange vi behaviour:
	 * ":3"		jumps to line 3
	 * ":3|..."	prints line 3
	 * ":|"		prints current line
	 */
	if (ea.skip)	    /* skip this if inside :if */
	    goto doend;
	if (*ea.cmd == '|' || (exmode_active && ea.line1 != ea.line2))
	{
	    ea.cmdidx = CMD_print;
	    ea.argt = RANGE+COUNT+TRLBAR;
	    if ((errormsg = invalid_range(&ea)) == NULL)
	    {
		correct_range(&ea);
		ex_print(&ea);
	    }
	}
	else if (ea.addr_count != 0)
	{
	    if (ea.line2 > curbuf->b_ml.ml_line_count)
	    {
		/* With '-' in 'cpoptions' a line number past the file is an
		 * error, otherwise put it at the end of the file. */
		if (vim_strchr(p_cpo, CPO_MINUS) != NULL)
		    ea.line2 = -1;
		else
		    ea.line2 = curbuf->b_ml.ml_line_count;
	    }

	    if (ea.line2 < 0)
		errormsg = _(e_invrange);
	    else
	    {
		if (ea.line2 == 0)
		    curwin->w_cursor.lnum = 1;
		else
		    curwin->w_cursor.lnum = ea.line2;
		beginline(BL_SOL | BL_FIX);
	    }
	}
	goto doend;
    }

    /* If this looks like an undefined user command and there are CmdUndefined
     * autocommands defined, trigger the matching autocommands. */
    if (p != NULL && ea.cmdidx == CMD_SIZE && !ea.skip
	    && ASCII_ISUPPER(*ea.cmd)
	    && has_cmdundefined())
    {
	int ret;

	p = ea.cmd;
	while (ASCII_ISALNUM(*p))
	    ++p;
	p = vim_strnsave(ea.cmd, (int)(p - ea.cmd));
	ret = apply_autocmds(EVENT_CMDUNDEFINED, p, p, TRUE, NULL);
	vim_free(p);
	/* If the autocommands did something and didn't cause an error, try
	 * finding the command again. */
	p = (ret
#ifdef FEAT_EVAL
		&& !aborting()
#endif
		) ? find_command(&ea, NULL) : ea.cmd;
    }

#ifdef FEAT_USR_CMDS
    if (p == NULL)
    {
	if (!ea.skip)
	    errormsg = _("E464: Ambiguous use of user-defined command");
	goto doend;
    }
    /* Check for wrong commands. */
    if (*p == '!' && ea.cmd[1] == 0151 && ea.cmd[0] == 78
	    && !IS_USER_CMDIDX(ea.cmdidx))
    {
	errormsg = uc_fun_cmd();
	goto doend;
    }
#endif
    if (ea.cmdidx == CMD_SIZE)
    {
	if (!ea.skip)
	{
	    STRCPY(IObuff, _("E492: Not an editor command"));
	    if (!sourcing)
	    {
		/* If the modifier was parsed OK the error must be in the
		 * following command */
		if (after_modifier != NULL)
		    append_command(after_modifier);
		else
		    append_command(*cmdlinep);
	    }
	    errormsg = (char *)IObuff;
	    did_emsg_syntax = TRUE;
	}
	goto doend;
    }

    ni = (!IS_USER_CMDIDX(ea.cmdidx)
	    && (cmdnames[ea.cmdidx].cmd_func == ex_ni
#ifdef HAVE_EX_SCRIPT_NI
	     || cmdnames[ea.cmdidx].cmd_func == ex_script_ni
#endif
	     ));

#ifndef FEAT_EVAL
    /*
     * When the expression evaluation is disabled, recognize the ":if" and
     * ":endif" commands and ignore everything in between it.
     */
    if (ea.cmdidx == CMD_if)
	++if_level;
    if (if_level)
    {
	if (ea.cmdidx == CMD_endif)
	    --if_level;
	goto doend;
    }

#endif

    /* forced commands */
    if (*p == '!' && ea.cmdidx != CMD_substitute
	    && ea.cmdidx != CMD_smagic && ea.cmdidx != CMD_snomagic)
    {
	++p;
	ea.forceit = TRUE;
    }
    else
	ea.forceit = FALSE;

/*
 * 6. Parse arguments.
 */
    if (!IS_USER_CMDIDX(ea.cmdidx))
	ea.argt = (long)cmdnames[(int)ea.cmdidx].cmd_argt;

    if (!ea.skip)
    {
#ifdef HAVE_SANDBOX
	if (sandbox != 0 && !(ea.argt & SBOXOK))
	{
	    /* Command not allowed in sandbox. */
	    errormsg = _(e_sandbox);
	    goto doend;
	}
#endif
	if (!curbuf->b_p_ma && (ea.argt & MODIFY))
	{
	    /* Command not allowed in non-'modifiable' buffer */
	    errormsg = _(e_modifiable);
	    goto doend;
	}

	if (text_locked() && !(ea.argt & CMDWIN)
		&& !IS_USER_CMDIDX(ea.cmdidx))
	{
	    /* Command not allowed when editing the command line. */
	    errormsg = _(get_text_locked_msg());
	    goto doend;
	}

	/* Disallow editing another buffer when "curbuf_lock" is set.
	 * Do allow ":checktime" (it is postponed).
	 * Do allow ":edit" (check for an argument later).
	 * Do allow ":file" with no arguments (check for an argument later). */
	if (!(ea.argt & CMDWIN)
		&& ea.cmdidx != CMD_checktime
		&& ea.cmdidx != CMD_edit
		&& ea.cmdidx != CMD_file
		&& !IS_USER_CMDIDX(ea.cmdidx)
		&& curbuf_locked())
	    goto doend;

	if (!ni && !(ea.argt & RANGE) && ea.addr_count > 0)
	{
	    /* no range allowed */
	    errormsg = _(e_norange);
	    goto doend;
	}
    }

    if (!ni && !(ea.argt & BANG) && ea.forceit)	/* no <!> allowed */
    {
	errormsg = _(e_nobang);
	goto doend;
    }

    /*
     * Don't complain about the range if it is not used
     * (could happen if line_count is accidentally set to 0).
     */
    if (!ea.skip && !ni)
    {
	/*
	 * If the range is backwards, ask for confirmation and, if given, swap
	 * ea.line1 & ea.line2 so it's forwards again.
	 * When global command is busy, don't ask, will fail below.
	 */
	if (!global_busy && ea.line1 > ea.line2)
	{
	    if (msg_silent == 0)
	    {
		if (sourcing || exmode_active)
		{
		    errormsg = _("E493: Backwards range given");
		    goto doend;
		}
		if (ask_yesno((char_u *)
			_("Backwards range given, OK to swap"), FALSE) != 'y')
		    goto doend;
	    }
	    lnum = ea.line1;
	    ea.line1 = ea.line2;
	    ea.line2 = lnum;
	}
	if ((errormsg = invalid_range(&ea)) != NULL)
	    goto doend;
    }

    if ((ea.argt & NOTADR) && ea.addr_count == 0) /* default is 1, not cursor */
	ea.line2 = 1;

    correct_range(&ea);

#ifdef FEAT_FOLDING
    if (((ea.argt & WHOLEFOLD) || ea.addr_count >= 2) && !global_busy
	    && ea.addr_type == ADDR_LINES)
    {
	/* Put the first line at the start of a closed fold, put the last line
	 * at the end of a closed fold. */
	(void)hasFolding(ea.line1, &ea.line1, NULL);
	(void)hasFolding(ea.line2, NULL, &ea.line2);
    }
#endif

#ifdef FEAT_QUICKFIX
    /*
     * For the ":make" and ":grep" commands we insert the 'makeprg'/'grepprg'
     * option here, so things like % get expanded.
     */
    p = replace_makeprg(&ea, p, cmdlinep);
    if (p == NULL)
	goto doend;
#endif

    /*
     * Skip to start of argument.
     * Don't do this for the ":!" command, because ":!! -l" needs the space.
     */
    if (ea.cmdidx == CMD_bang)
	ea.arg = p;
    else
	ea.arg = skipwhite(p);

    // ":file" cannot be run with an argument when "curbuf_lock" is set
    if (ea.cmdidx == CMD_file && *ea.arg != NUL && curbuf_locked())
	goto doend;

    /*
     * Check for "++opt=val" argument.
     * Must be first, allow ":w ++enc=utf8 !cmd"
     */
    if (ea.argt & ARGOPT)
	while (ea.arg[0] == '+' && ea.arg[1] == '+')
	    if (getargopt(&ea) == FAIL && !ni)
	    {
		errormsg = _(e_invarg);
		goto doend;
	    }

    if (ea.cmdidx == CMD_write || ea.cmdidx == CMD_update)
    {
	if (*ea.arg == '>')			/* append */
	{
	    if (*++ea.arg != '>')		/* typed wrong */
	    {
		errormsg = _("E494: Use w or w>>");
		goto doend;
	    }
	    ea.arg = skipwhite(ea.arg + 1);
	    ea.append = TRUE;
	}
	else if (*ea.arg == '!' && ea.cmdidx == CMD_write)  /* :w !filter */
	{
	    ++ea.arg;
	    ea.usefilter = TRUE;
	}
    }

    if (ea.cmdidx == CMD_read)
    {
	if (ea.forceit)
	{
	    ea.usefilter = TRUE;		/* :r! filter if ea.forceit */
	    ea.forceit = FALSE;
	}
	else if (*ea.arg == '!')		/* :r !filter */
	{
	    ++ea.arg;
	    ea.usefilter = TRUE;
	}
    }

    if (ea.cmdidx == CMD_lshift || ea.cmdidx == CMD_rshift)
    {
	ea.amount = 1;
	while (*ea.arg == *ea.cmd)		/* count number of '>' or '<' */
	{
	    ++ea.arg;
	    ++ea.amount;
	}
	ea.arg = skipwhite(ea.arg);
    }

    /*
     * Check for "+command" argument, before checking for next command.
     * Don't do this for ":read !cmd" and ":write !cmd".
     */
    if ((ea.argt & EDITCMD) && !ea.usefilter)
	ea.do_ecmd_cmd = getargcmd(&ea.arg);

    /*
     * Check for '|' to separate commands and '"' to start comments.
     * Don't do this for ":read !cmd" and ":write !cmd".
     */
    if ((ea.argt & TRLBAR) && !ea.usefilter)
	separate_nextcmd(&ea);

    /*
     * Check for <newline> to end a shell command.
     * Also do this for ":read !cmd", ":write !cmd" and ":global".
     * Any others?
     */
    else if (ea.cmdidx == CMD_bang
	    || ea.cmdidx == CMD_terminal
	    || ea.cmdidx == CMD_global
	    || ea.cmdidx == CMD_vglobal
	    || ea.usefilter)
    {
	for (p = ea.arg; *p; ++p)
	{
	    /* Remove one backslash before a newline, so that it's possible to
	     * pass a newline to the shell and also a newline that is preceded
	     * with a backslash.  This makes it impossible to end a shell
	     * command in a backslash, but that doesn't appear useful.
	     * Halving the number of backslashes is incompatible with previous
	     * versions. */
	    if (*p == '\\' && p[1] == '\n')
		STRMOVE(p, p + 1);
	    else if (*p == '\n')
	    {
		ea.nextcmd = p + 1;
		*p = NUL;
		break;
	    }
	}
    }

    if ((ea.argt & DFLALL) && ea.addr_count == 0)
    {
	buf_T	    *buf;

	ea.line1 = 1;
	switch (ea.addr_type)
	{
	    case ADDR_LINES:
		ea.line2 = curbuf->b_ml.ml_line_count;
		break;
	    case ADDR_LOADED_BUFFERS:
		buf = firstbuf;
		while (buf->b_next != NULL && buf->b_ml.ml_mfp == NULL)
		    buf = buf->b_next;
		ea.line1 = buf->b_fnum;
		buf = lastbuf;
		while (buf->b_prev != NULL && buf->b_ml.ml_mfp == NULL)
		    buf = buf->b_prev;
		ea.line2 = buf->b_fnum;
		break;
	    case ADDR_BUFFERS:
		ea.line1 = firstbuf->b_fnum;
		ea.line2 = lastbuf->b_fnum;
		break;
	    case ADDR_WINDOWS:
		ea.line2 = LAST_WIN_NR;
		break;
	    case ADDR_TABS:
		ea.line2 = LAST_TAB_NR;
		break;
	    case ADDR_TABS_RELATIVE:
		ea.line2 = 1;
		break;
	    case ADDR_ARGUMENTS:
		if (ARGCOUNT == 0)
		    ea.line1 = ea.line2 = 0;
		else
		    ea.line2 = ARGCOUNT;
		break;
#ifdef FEAT_QUICKFIX
	    case ADDR_QUICKFIX:
		ea.line2 = qf_get_size(&ea);
		if (ea.line2 == 0)
		    ea.line2 = 1;
		break;
#endif
	}
    }

    /* accept numbered register only when no count allowed (:put) */
    if (       (ea.argt & REGSTR)
	    && *ea.arg != NUL
	       /* Do not allow register = for user commands */
	    && (!IS_USER_CMDIDX(ea.cmdidx) || *ea.arg != '=')
	    && !((ea.argt & COUNT) && VIM_ISDIGIT(*ea.arg)))
    {
#ifndef FEAT_CLIPBOARD
	/* check these explicitly for a more specific error message */
	if (*ea.arg == '*' || *ea.arg == '+')
	{
	    errormsg = _(e_invalidreg);
	    goto doend;
	}
#endif
	if (valid_yank_reg(*ea.arg, (ea.cmdidx != CMD_put
					      && !IS_USER_CMDIDX(ea.cmdidx))))
	{
	    ea.regname = *ea.arg++;
#ifdef FEAT_EVAL
	    /* for '=' register: accept the rest of the line as an expression */
	    if (ea.arg[-1] == '=' && ea.arg[0] != NUL)
	    {
		set_expr_line(vim_strsave(ea.arg));
		ea.arg += STRLEN(ea.arg);
	    }
#endif
	    ea.arg = skipwhite(ea.arg);
	}
    }

    /*
     * Check for a count.  When accepting a BUFNAME, don't use "123foo" as a
     * count, it's a buffer name.
     */
    if ((ea.argt & COUNT) && VIM_ISDIGIT(*ea.arg)
	    && (!(ea.argt & BUFNAME) || *(p = skipdigits(ea.arg)) == NUL
							  || VIM_ISWHITE(*p)))
    {
	n = getdigits(&ea.arg);
	ea.arg = skipwhite(ea.arg);
	if (n <= 0 && !ni && (ea.argt & ZEROR) == 0)
	{
	    errormsg = _(e_zerocount);
	    goto doend;
	}
	if (ea.argt & NOTADR)	/* e.g. :buffer 2, :sleep 3 */
	{
	    ea.line2 = n;
	    if (ea.addr_count == 0)
		ea.addr_count = 1;
	}
	else
	{
	    ea.line1 = ea.line2;
	    ea.line2 += n - 1;
	    ++ea.addr_count;
	    /*
	     * Be vi compatible: no error message for out of range.
	     */
	    if (ea.addr_type == ADDR_LINES
		    && ea.line2 > curbuf->b_ml.ml_line_count)
		ea.line2 = curbuf->b_ml.ml_line_count;
	}
    }

    /*
     * Check for flags: 'l', 'p' and '#'.
     */
    if (ea.argt & EXFLAGS)
	get_flags(&ea);
						/* no arguments allowed */
    if (!ni && !(ea.argt & EXTRA) && *ea.arg != NUL
	    && *ea.arg != '"' && (*ea.arg != '|' || (ea.argt & TRLBAR) == 0))
    {
	errormsg = _(e_trailing);
	goto doend;
    }

    if (!ni && (ea.argt & NEEDARG) && *ea.arg == NUL)
    {
	errormsg = _(e_argreq);
	goto doend;
    }

#ifdef FEAT_EVAL
    /*
     * Skip the command when it's not going to be executed.
     * The commands like :if, :endif, etc. always need to be executed.
     * Also make an exception for commands that handle a trailing command
     * themselves.
     */
    if (ea.skip)
    {
	switch (ea.cmdidx)
	{
	    /* commands that need evaluation */
	    case CMD_while:
	    case CMD_endwhile:
	    case CMD_for:
	    case CMD_endfor:
	    case CMD_if:
	    case CMD_elseif:
	    case CMD_else:
	    case CMD_endif:
	    case CMD_try:
	    case CMD_catch:
	    case CMD_finally:
	    case CMD_endtry:
	    case CMD_function:
				break;

	    /* Commands that handle '|' themselves.  Check: A command should
	     * either have the TRLBAR flag, appear in this list or appear in
	     * the list at ":help :bar". */
	    case CMD_aboveleft:
	    case CMD_and:
	    case CMD_belowright:
	    case CMD_botright:
	    case CMD_browse:
	    case CMD_call:
	    case CMD_confirm:
	    case CMD_delfunction:
	    case CMD_djump:
	    case CMD_dlist:
	    case CMD_dsearch:
	    case CMD_dsplit:
	    case CMD_echo:
	    case CMD_echoerr:
	    case CMD_echomsg:
	    case CMD_echon:
	    case CMD_execute:
	    case CMD_filter:
	    case CMD_help:
	    case CMD_hide:
	    case CMD_ijump:
	    case CMD_ilist:
	    case CMD_isearch:
	    case CMD_isplit:
	    case CMD_keepalt:
	    case CMD_keepjumps:
	    case CMD_keepmarks:
	    case CMD_keeppatterns:
	    case CMD_leftabove:
	    case CMD_let:
	    case CMD_lockmarks:
	    case CMD_lua:
	    case CMD_match:
	    case CMD_mzscheme:
	    case CMD_noautocmd:
	    case CMD_noswapfile:
	    case CMD_perl:
	    case CMD_psearch:
	    case CMD_python:
	    case CMD_py3:
	    case CMD_python3:
	    case CMD_return:
	    case CMD_rightbelow:
	    case CMD_ruby:
	    case CMD_silent:
	    case CMD_smagic:
	    case CMD_snomagic:
	    case CMD_substitute:
	    case CMD_syntax:
	    case CMD_tab:
	    case CMD_tcl:
	    case CMD_throw:
	    case CMD_tilde:
	    case CMD_topleft:
	    case CMD_unlet:
	    case CMD_verbose:
	    case CMD_vertical:
	    case CMD_wincmd:
				break;

	    default:		goto doend;
	}
    }
#endif

    if (ea.argt & XFILE)
    {
	if (expand_filename(&ea, cmdlinep, &errormsg) == FAIL)
	    goto doend;
    }

    /*
     * Accept buffer name.  Cannot be used at the same time with a buffer
     * number.  Don't do this for a user command.
     */
    if ((ea.argt & BUFNAME) && *ea.arg != NUL && ea.addr_count == 0
	    && !IS_USER_CMDIDX(ea.cmdidx))
    {
	/*
	 * :bdelete, :bwipeout and :bunload take several arguments, separated
	 * by spaces: find next space (skipping over escaped characters).
	 * The others take one argument: ignore trailing spaces.
	 */
	if (ea.cmdidx == CMD_bdelete || ea.cmdidx == CMD_bwipeout
						  || ea.cmdidx == CMD_bunload)
	    p = skiptowhite_esc(ea.arg);
	else
	{
	    p = ea.arg + STRLEN(ea.arg);
	    while (p > ea.arg && VIM_ISWHITE(p[-1]))
		--p;
	}
	ea.line2 = buflist_findpat(ea.arg, p, (ea.argt & BUFUNL) != 0,
								FALSE, FALSE);
	if (ea.line2 < 0)	    /* failed */
	    goto doend;
	ea.addr_count = 1;
	ea.arg = skipwhite(p);
    }

    /* The :try command saves the emsg_silent flag, reset it here when
     * ":silent! try" was used, it should only apply to :try itself. */
    if (ea.cmdidx == CMD_try && ea.did_esilent > 0)
    {
	emsg_silent -= ea.did_esilent;
	if (emsg_silent < 0)
	    emsg_silent = 0;
	ea.did_esilent = 0;
    }

/*
 * 7. Execute the command.
 */

#ifdef FEAT_USR_CMDS
    if (IS_USER_CMDIDX(ea.cmdidx))
    {
	/*
	 * Execute a user-defined command.
	 */
	do_ucmd(&ea);
    }
    else
#endif
    {
	/*
	 * Call the function to execute the command.
	 */
	ea.errmsg = NULL;
	(cmdnames[ea.cmdidx].cmd_func)(&ea);
	if (ea.errmsg != NULL)
	    errormsg = _(ea.errmsg);
    }

#ifdef FEAT_EVAL
    /*
     * If the command just executed called do_cmdline(), any throw or ":return"
     * or ":finish" encountered there must also check the cstack of the still
     * active do_cmdline() that called this do_one_cmd().  Rethrow an uncaught
     * exception, or reanimate a returned function or finished script file and
     * return or finish it again.
     */
    if (need_rethrow)
	do_throw(cstack);
    else if (check_cstack)
    {
	if (source_finished(fgetline, cookie))
	    do_finish(&ea, TRUE);
	else if (getline_equal(fgetline, cookie, get_func_line)
						   && current_func_returned())
	    do_return(&ea, TRUE, FALSE, NULL);
    }
    need_rethrow = check_cstack = FALSE;
#endif

doend:
    if (curwin->w_cursor.lnum == 0)	/* can happen with zero line number */
    {
	curwin->w_cursor.lnum = 1;
	curwin->w_cursor.col = 0;
    }

    if (errormsg != NULL && *errormsg != NUL && !did_emsg)
    {
	if (sourcing)
	{
	    if (errormsg != (char *)IObuff)
	    {
		STRCPY(IObuff, errormsg);
		errormsg = (char *)IObuff;
	    }
	    append_command(*cmdlinep);
	}
	emsg(errormsg);
    }
#ifdef FEAT_EVAL
    do_errthrow(cstack,
	    (ea.cmdidx != CMD_SIZE && !IS_USER_CMDIDX(ea.cmdidx))
			? cmdnames[(int)ea.cmdidx].cmd_name : (char_u *)NULL);
#endif

    if (ea.verbose_save >= 0)
	p_verbose = ea.verbose_save;

    free_cmdmod();
    cmdmod = save_cmdmod;

    if (ea.save_msg_silent != -1)
    {
	/* messages could be enabled for a serious error, need to check if the
	 * counters don't become negative */
	if (!did_emsg || msg_silent > ea.save_msg_silent)
	    msg_silent = ea.save_msg_silent;
	emsg_silent -= ea.did_esilent;
	if (emsg_silent < 0)
	    emsg_silent = 0;
	/* Restore msg_scroll, it's set by file I/O commands, even when no
	 * message is actually displayed. */
	msg_scroll = save_msg_scroll;

	/* "silent reg" or "silent echo x" inside "redir" leaves msg_col
	 * somewhere in the line.  Put it back in the first column. */
	if (redirecting())
	    msg_col = 0;
    }

#ifdef HAVE_SANDBOX
    if (ea.did_sandbox)
	--sandbox;
#endif

    if (ea.nextcmd && *ea.nextcmd == NUL)	/* not really a next command */
	ea.nextcmd = NULL;

#ifdef FEAT_EVAL
    --ex_nesting_level;
#endif

    return ea.nextcmd;
}