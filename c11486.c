normal_cmd(
    oparg_T	*oap,
    int		toplevel UNUSED)	// TRUE when called from main()
{
    cmdarg_T	ca;			// command arguments
    int		c;
    int		ctrl_w = FALSE;		// got CTRL-W command
    int		old_col = curwin->w_curswant;
    int		need_flushbuf = FALSE;	// need to call out_flush()
    pos_T	old_pos;		// cursor position before command
    int		mapped_len;
    static int	old_mapped_len = 0;
    int		idx;
    int		set_prevcount = FALSE;
    int		save_did_cursorhold = did_cursorhold;

    CLEAR_FIELD(ca);	// also resets ca.retval
    ca.oap = oap;

    // Use a count remembered from before entering an operator.  After typing
    // "3d" we return from normal_cmd() and come back here, the "3" is
    // remembered in "opcount".
    ca.opcount = opcount;

    // If there is an operator pending, then the command we take this time
    // will terminate it. Finish_op tells us to finish the operation before
    // returning this time (unless the operation was cancelled).
#ifdef CURSOR_SHAPE
    c = finish_op;
#endif
    finish_op = (oap->op_type != OP_NOP);
#ifdef CURSOR_SHAPE
    if (finish_op != c)
    {
	ui_cursor_shape();		// may show different cursor shape
# ifdef FEAT_MOUSESHAPE
	update_mouseshape(-1);
# endif
    }
#endif
    may_trigger_modechanged();

    // When not finishing an operator and no register name typed, reset the
    // count.
    if (!finish_op && !oap->regname)
    {
	ca.opcount = 0;
#ifdef FEAT_EVAL
	set_prevcount = TRUE;
#endif
    }

    // Restore counts from before receiving K_CURSORHOLD.  This means after
    // typing "3", handling K_CURSORHOLD and then typing "2" we get "32", not
    // "3 * 2".
    if (oap->prev_opcount > 0 || oap->prev_count0 > 0)
    {
	ca.opcount = oap->prev_opcount;
	ca.count0 = oap->prev_count0;
	oap->prev_opcount = 0;
	oap->prev_count0 = 0;
    }

    mapped_len = typebuf_maplen();

    State = MODE_NORMAL_BUSY;
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = FALSE;	// allow scrolling here
#endif

#ifdef FEAT_EVAL
    // Set v:count here, when called from main() and not a stuffed
    // command, so that v:count can be used in an expression mapping
    // when there is no count. Do set it for redo.
    if (toplevel && readbuf1_empty())
	set_vcount_ca(&ca, &set_prevcount);
#endif

    /*
     * Get the command character from the user.
     */
    c = safe_vgetc();
    LANGMAP_ADJUST(c, get_real_state() != MODE_SELECT);

    // If a mapping was started in Visual or Select mode, remember the length
    // of the mapping.  This is used below to not return to Insert mode for as
    // long as the mapping is being executed.
    if (restart_edit == 0)
	old_mapped_len = 0;
    else if (old_mapped_len
		|| (VIsual_active && mapped_len == 0 && typebuf_maplen() > 0))
	old_mapped_len = typebuf_maplen();

    if (c == NUL)
	c = K_ZERO;

    // In Select mode, typed text replaces the selection.
    if (VIsual_active
	    && VIsual_select
	    && (vim_isprintc(c) || c == NL || c == CAR || c == K_KENTER))
    {
	int len;

	// Fake a "c"hange command.  When "restart_edit" is set (e.g., because
	// 'insertmode' is set) fake a "d"elete command, Insert mode will
	// restart automatically.
	// Insert the typed character in the typeahead buffer, so that it can
	// be mapped in Insert mode.  Required for ":lmap" to work.
	len = ins_char_typebuf(vgetc_char, vgetc_mod_mask);

	// When recording and gotchars() was called the character will be
	// recorded again, remove the previous recording.
	if (KeyTyped)
	    ungetchars(len);

	if (restart_edit != 0)
	    c = 'd';
	else
	    c = 'c';
	msg_nowait = TRUE;	// don't delay going to insert mode
	old_mapped_len = 0;	// do go to Insert mode
    }

    // If the window was made so small that nothing shows, make it at least one
    // line and one column when typing a command.
    if (KeyTyped && !KeyStuffed)
	win_ensure_size();

#ifdef FEAT_CMDL_INFO
    need_flushbuf = add_to_showcmd(c);
#endif

    // Get the command count
    c = normal_cmd_get_count(&ca, c, toplevel, set_prevcount, &ctrl_w,
							&need_flushbuf);

    // Find the command character in the table of commands.
    // For CTRL-W we already got nchar when looking for a count.
    if (ctrl_w)
    {
	ca.nchar = c;
	ca.cmdchar = Ctrl_W;
    }
    else
	ca.cmdchar = c;
    idx = find_command(ca.cmdchar);
    if (idx < 0)
    {
	// Not a known command: beep.
	clearopbeep(oap);
	goto normal_end;
    }

    if (text_locked() && (nv_cmds[idx].cmd_flags & NV_NCW))
    {
	// This command is not allowed while editing a cmdline: beep.
	clearopbeep(oap);
	text_locked_msg();
	goto normal_end;
    }
    if ((nv_cmds[idx].cmd_flags & NV_NCW) && curbuf_locked())
	goto normal_end;

    // In Visual/Select mode, a few keys are handled in a special way.
    if (VIsual_active)
    {
	// when 'keymodel' contains "stopsel" may stop Select/Visual mode
	if (km_stopsel
		&& (nv_cmds[idx].cmd_flags & NV_STS)
		&& !(mod_mask & MOD_MASK_SHIFT))
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);
	}

	// Keys that work different when 'keymodel' contains "startsel"
	if (km_startsel)
	{
	    if (nv_cmds[idx].cmd_flags & NV_SS)
	    {
		unshift_special(&ca);
		idx = find_command(ca.cmdchar);
		if (idx < 0)
		{
		    // Just in case
		    clearopbeep(oap);
		    goto normal_end;
		}
	    }
	    else if ((nv_cmds[idx].cmd_flags & NV_SSS)
					       && (mod_mask & MOD_MASK_SHIFT))
		mod_mask &= ~MOD_MASK_SHIFT;
	}
    }

#ifdef FEAT_RIGHTLEFT
    if (curwin->w_p_rl && KeyTyped && !KeyStuffed
					  && (nv_cmds[idx].cmd_flags & NV_RL))
    {
	// Invert horizontal movements and operations.  Only when typed by the
	// user directly, not when the result of a mapping or "x" translated
	// to "dl".
	switch (ca.cmdchar)
	{
	    case 'l':	    ca.cmdchar = 'h'; break;
	    case K_RIGHT:   ca.cmdchar = K_LEFT; break;
	    case K_S_RIGHT: ca.cmdchar = K_S_LEFT; break;
	    case K_C_RIGHT: ca.cmdchar = K_C_LEFT; break;
	    case 'h':	    ca.cmdchar = 'l'; break;
	    case K_LEFT:    ca.cmdchar = K_RIGHT; break;
	    case K_S_LEFT:  ca.cmdchar = K_S_RIGHT; break;
	    case K_C_LEFT:  ca.cmdchar = K_C_RIGHT; break;
	    case '>':	    ca.cmdchar = '<'; break;
	    case '<':	    ca.cmdchar = '>'; break;
	}
	idx = find_command(ca.cmdchar);
    }
#endif

    // Get additional characters if we need them.
    if (normal_cmd_needs_more_chars(&ca, nv_cmds[idx].cmd_flags))
	idx = normal_cmd_get_more_chars(idx, &ca, &need_flushbuf);

#ifdef FEAT_CMDL_INFO
    // Flush the showcmd characters onto the screen so we can see them while
    // the command is being executed.  Only do this when the shown command was
    // actually displayed, otherwise this will slow down a lot when executing
    // mappings.
    if (need_flushbuf)
	out_flush();
#endif
    if (ca.cmdchar != K_IGNORE)
    {
	if (ex_normal_busy)
	    did_cursorhold = save_did_cursorhold;
	else
	    did_cursorhold = FALSE;
    }

    State = MODE_NORMAL;

    if (ca.nchar == ESC)
    {
	clearop(oap);
	if (restart_edit == 0 && goto_im())
	    restart_edit = 'a';
	goto normal_end;
    }

    if (ca.cmdchar != K_IGNORE)
    {
	msg_didout = FALSE;    // don't scroll screen up for normal command
	msg_col = 0;
    }

    old_pos = curwin->w_cursor;		// remember where cursor was

    // When 'keymodel' contains "startsel" some keys start Select/Visual
    // mode.
    if (!VIsual_active && km_startsel)
    {
	if (nv_cmds[idx].cmd_flags & NV_SS)
	{
	    start_selection();
	    unshift_special(&ca);
	    idx = find_command(ca.cmdchar);
	}
	else if ((nv_cmds[idx].cmd_flags & NV_SSS)
					   && (mod_mask & MOD_MASK_SHIFT))
	{
	    start_selection();
	    mod_mask &= ~MOD_MASK_SHIFT;
	}
    }

    // Execute the command!
    // Call the command function found in the commands table.
    ca.arg = nv_cmds[idx].cmd_arg;
    (nv_cmds[idx].cmd_func)(&ca);

    // If we didn't start or finish an operator, reset oap->regname, unless we
    // need it later.
    if (!finish_op
	    && !oap->op_type
	    && (idx < 0 || !(nv_cmds[idx].cmd_flags & NV_KEEPREG)))
    {
	clearop(oap);
#ifdef FEAT_EVAL
	reset_reg_var();
#endif
    }

    // Get the length of mapped chars again after typing a count, second
    // character or "z333<cr>".
    if (old_mapped_len > 0)
	old_mapped_len = typebuf_maplen();

    // If an operation is pending, handle it.  But not for K_IGNORE or
    // K_MOUSEMOVE.
    if (ca.cmdchar != K_IGNORE && ca.cmdchar != K_MOUSEMOVE)
	do_pending_operator(&ca, old_col, FALSE);

    // Wait for a moment when a message is displayed that will be overwritten
    // by the mode message.
    if (normal_cmd_need_to_wait_for_msg(&ca, &old_pos))
	normal_cmd_wait_for_msg();

    // Finish up after executing a Normal mode command.
normal_end:

    msg_nowait = FALSE;

#ifdef FEAT_EVAL
    if (finish_op)
	reset_reg_var();
#endif

    // Reset finish_op, in case it was set
#ifdef CURSOR_SHAPE
    c = finish_op;
#endif
    finish_op = FALSE;
    may_trigger_modechanged();
#ifdef CURSOR_SHAPE
    // Redraw the cursor with another shape, if we were in Operator-pending
    // mode or did a replace command.
    if (c || ca.cmdchar == 'r')
    {
	ui_cursor_shape();		// may show different cursor shape
# ifdef FEAT_MOUSESHAPE
	update_mouseshape(-1);
# endif
    }
#endif

#ifdef FEAT_CMDL_INFO
    if (oap->op_type == OP_NOP && oap->regname == 0
	    && ca.cmdchar != K_CURSORHOLD)
	clear_showcmd();
#endif

    checkpcmark();		// check if we moved since setting pcmark
    vim_free(ca.searchbuf);

    if (has_mbyte)
	mb_adjust_cursor();

    if (curwin->w_p_scb && toplevel)
    {
	validate_cursor();	// may need to update w_leftcol
	do_check_scrollbind(TRUE);
    }

    if (curwin->w_p_crb && toplevel)
    {
	validate_cursor();	// may need to update w_leftcol
	do_check_cursorbind();
    }

#ifdef FEAT_TERMINAL
    // don't go to Insert mode if a terminal has a running job
    if (term_job_running(curbuf->b_term))
	restart_edit = 0;
#endif

    // May restart edit(), if we got here with CTRL-O in Insert mode (but not
    // if still inside a mapping that started in Visual mode).
    // May switch from Visual to Select mode after CTRL-O command.
    if (       oap->op_type == OP_NOP
	    && ((restart_edit != 0 && !VIsual_active && old_mapped_len == 0)
		|| restart_VIsual_select == 1)
	    && !(ca.retval & CA_COMMAND_BUSY)
	    && stuff_empty()
	    && oap->regname == 0)
    {
	if (restart_VIsual_select == 1)
	{
	    VIsual_select = TRUE;
	    may_trigger_modechanged();
	    showmode();
	    restart_VIsual_select = 0;
	    VIsual_select_reg = 0;
	}
	if (restart_edit != 0 && !VIsual_active && old_mapped_len == 0)
	    (void)edit(restart_edit, FALSE, 1L);
    }

    if (restart_VIsual_select == 2)
	restart_VIsual_select = 1;

    // Save count before an operator for next time.
    opcount = ca.opcount;
}