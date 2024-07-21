set_num_option(
    int		opt_idx,		// index in options[] table
    char_u	*varp,			// pointer to the option variable
    long	value,			// new value
    char	*errbuf,		// buffer for error messages
    size_t	errbuflen,		// length of "errbuf"
    int		opt_flags)		// OPT_LOCAL, OPT_GLOBAL,
					// OPT_MODELINE, etc.
{
    char	*errmsg = NULL;
    long	old_value = *(long *)varp;
#if defined(FEAT_EVAL)
    long	old_global_value = 0;	// only used when setting a local and
					// global option
#endif
    long	old_Rows = Rows;	// remember old Rows
    long	old_Columns = Columns;	// remember old Columns
    long	*pp = (long *)varp;

    // Disallow changing some options from secure mode.
    if ((secure
#ifdef HAVE_SANDBOX
		|| sandbox != 0
#endif
		) && (options[opt_idx].flags & P_SECURE))
	return e_not_allowed_here;

#if defined(FEAT_EVAL)
    // Save the global value before changing anything. This is needed as for
    // a global-only option setting the "local value" in fact sets the global
    // value (since there is only one value).
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	old_global_value = *(long *)get_varp_scope(&(options[opt_idx]),
								   OPT_GLOBAL);
#endif

    *pp = value;
#ifdef FEAT_EVAL
    // Remember where the option was set.
    set_option_sctx_idx(opt_idx, opt_flags, current_sctx);
#endif
#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    if (curbuf->b_p_sw < 0)
    {
	errmsg = e_argument_must_be_positive;
#ifdef FEAT_VARTABS
	// Use the first 'vartabstop' value, or 'tabstop' if vts isn't in use.
	curbuf->b_p_sw = tabstop_count(curbuf->b_p_vts_array) > 0
	               ? tabstop_first(curbuf->b_p_vts_array)
		       : curbuf->b_p_ts;
#else
	curbuf->b_p_sw = curbuf->b_p_ts;
#endif
    }

    /*
     * Number options that need some action when changed
     */
    if (pp == &p_wh || pp == &p_hh)
    {
	// 'winheight' and 'helpheight'
	if (p_wh < 1)
	{
	    errmsg = e_argument_must_be_positive;
	    p_wh = 1;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = e_winheight_cannot_be_smaller_than_winminheight;
	    p_wh = p_wmh;
	}
	if (p_hh < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    p_hh = 0;
	}

	// Change window height NOW
	if (!ONE_WINDOW)
	{
	    if (pp == &p_wh && curwin->w_height < p_wh)
		win_setheight((int)p_wh);
	    if (pp == &p_hh && curbuf->b_help && curwin->w_height < p_hh)
		win_setheight((int)p_hh);
	}
    }
    else if (pp == &p_wmh)
    {
	// 'winminheight'
	if (p_wmh < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    p_wmh = 0;
	}
	if (p_wmh > p_wh)
	{
	    errmsg = e_winheight_cannot_be_smaller_than_winminheight;
	    p_wmh = p_wh;
	}
	win_setminheight();
    }
    else if (pp == &p_wiw)
    {
	// 'winwidth'
	if (p_wiw < 1)
	{
	    errmsg = e_argument_must_be_positive;
	    p_wiw = 1;
	}
	if (p_wmw > p_wiw)
	{
	    errmsg = e_winwidth_cannot_be_smaller_than_winminwidth;
	    p_wiw = p_wmw;
	}

	// Change window width NOW
	if (!ONE_WINDOW && curwin->w_width < p_wiw)
	    win_setwidth((int)p_wiw);
    }
    else if (pp == &p_wmw)
    {
	// 'winminwidth'
	if (p_wmw < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    p_wmw = 0;
	}
	if (p_wmw > p_wiw)
	{
	    errmsg = e_winwidth_cannot_be_smaller_than_winminwidth;
	    p_wmw = p_wiw;
	}
	win_setminwidth();
    }

    // (re)set last window status line
    else if (pp == &p_ls)
    {
	last_status(FALSE);
    }

    // (re)set tab page line
    else if (pp == &p_stal)
    {
	shell_new_rows();	// recompute window positions and heights
    }

#ifdef FEAT_GUI
    else if (pp == &p_linespace)
    {
	// Recompute gui.char_height and resize the Vim window to keep the
	// same number of lines.
	if (gui.in_use && gui_mch_adjust_charheight() == OK)
	    gui_set_shellsize(FALSE, FALSE, RESIZE_VERT);
    }
#endif

#ifdef FEAT_FOLDING
    // 'foldlevel'
    else if (pp == &curwin->w_p_fdl)
    {
	if (curwin->w_p_fdl < 0)
	    curwin->w_p_fdl = 0;
	newFoldLevel();
    }

    // 'foldminlines'
    else if (pp == &curwin->w_p_fml)
    {
	foldUpdateAll(curwin);
    }

    // 'foldnestmax'
    else if (pp == &curwin->w_p_fdn)
    {
	if (foldmethodIsSyntax(curwin) || foldmethodIsIndent(curwin))
	    foldUpdateAll(curwin);
    }

    // 'foldcolumn'
    else if (pp == &curwin->w_p_fdc)
    {
	if (curwin->w_p_fdc < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    curwin->w_p_fdc = 0;
	}
	else if (curwin->w_p_fdc > 12)
	{
	    errmsg = e_invalid_argument;
	    curwin->w_p_fdc = 12;
	}
    }
#endif // FEAT_FOLDING

#if defined(FEAT_FOLDING) || defined(FEAT_CINDENT)
    // 'shiftwidth' or 'tabstop'
    else if (pp == &curbuf->b_p_sw || pp == &curbuf->b_p_ts)
    {
# ifdef FEAT_FOLDING
	if (foldmethodIsIndent(curwin))
	    foldUpdateAll(curwin);
# endif
# ifdef FEAT_CINDENT
	// When 'shiftwidth' changes, or it's zero and 'tabstop' changes:
	// parse 'cinoptions'.
	if (pp == &curbuf->b_p_sw || curbuf->b_p_sw == 0)
	    parse_cino(curbuf);
# endif
    }
#endif

    // 'maxcombine'
    else if (pp == &p_mco)
    {
	if (p_mco > MAX_MCO)
	    p_mco = MAX_MCO;
	else if (p_mco < 0)
	    p_mco = 0;
	screenclear();	    // will re-allocate the screen
    }

    else if (pp == &curbuf->b_p_iminsert)
    {
	if (curbuf->b_p_iminsert < 0 || curbuf->b_p_iminsert > B_IMODE_LAST)
	{
	    errmsg = e_invalid_argument;
	    curbuf->b_p_iminsert = B_IMODE_NONE;
	}
	p_iminsert = curbuf->b_p_iminsert;
	if (termcap_active)	// don't do this in the alternate screen
	    showmode();
#if defined(FEAT_KEYMAP)
	// Show/unshow value of 'keymap' in status lines.
	status_redraw_curbuf();
#endif
    }

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
    // 'imstyle'
    else if (pp == &p_imst)
    {
	if (p_imst != IM_ON_THE_SPOT && p_imst != IM_OVER_THE_SPOT)
	    errmsg = e_invalid_argument;
    }
#endif

    else if (pp == &p_window)
    {
	if (p_window < 1)
	    p_window = 1;
	else if (p_window >= Rows)
	    p_window = Rows - 1;
    }

    else if (pp == &curbuf->b_p_imsearch)
    {
	if (curbuf->b_p_imsearch < -1 || curbuf->b_p_imsearch > B_IMODE_LAST)
	{
	    errmsg = e_invalid_argument;
	    curbuf->b_p_imsearch = B_IMODE_NONE;
	}
	p_imsearch = curbuf->b_p_imsearch;
    }

    // if 'titlelen' has changed, redraw the title
    else if (pp == &p_titlelen)
    {
	if (p_titlelen < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    p_titlelen = 85;
	}
	if (starting != NO_SCREEN && old_value != p_titlelen)
	    need_maketitle = TRUE;
    }

    // if p_ch changed value, change the command line height
    else if (pp == &p_ch)
    {
	if (p_ch < 1)
	{
	    errmsg = e_argument_must_be_positive;
	    p_ch = 1;
	}
	if (p_ch > Rows - min_rows() + 1)
	    p_ch = Rows - min_rows() + 1;

	// Only compute the new window layout when startup has been
	// completed. Otherwise the frame sizes may be wrong.
	if (p_ch != old_value && full_screen
#ifdef FEAT_GUI
		&& !gui.starting
#endif
	   )
	    command_height();
    }

    // when 'updatecount' changes from zero to non-zero, open swap files
    else if (pp == &p_uc)
    {
	if (p_uc < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    p_uc = 100;
	}
	if (p_uc && !old_value)
	    ml_open_files();
    }
#ifdef FEAT_CONCEAL
    else if (pp == &curwin->w_p_cole)
    {
	if (curwin->w_p_cole < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    curwin->w_p_cole = 0;
	}
	else if (curwin->w_p_cole > 3)
	{
	    errmsg = e_invalid_argument;
	    curwin->w_p_cole = 3;
	}
    }
#endif
#ifdef MZSCHEME_GUI_THREADS
    else if (pp == &p_mzq)
	mzvim_reset_timer();
#endif

#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
    // 'pyxversion'
    else if (pp == &p_pyx)
    {
	if (p_pyx != 0 && p_pyx != 2 && p_pyx != 3)
	    errmsg = e_invalid_argument;
    }
#endif

    // sync undo before 'undolevels' changes
    else if (pp == &p_ul)
    {
	// use the old value, otherwise u_sync() may not work properly
	p_ul = old_value;
	u_sync(TRUE);
	p_ul = value;
    }
    else if (pp == &curbuf->b_p_ul)
    {
	// use the old value, otherwise u_sync() may not work properly
	curbuf->b_p_ul = old_value;
	u_sync(TRUE);
	curbuf->b_p_ul = value;
    }

#ifdef FEAT_LINEBREAK
    // 'numberwidth' must be positive
    else if (pp == &curwin->w_p_nuw)
    {
	if (curwin->w_p_nuw < 1)
	{
	    errmsg = e_argument_must_be_positive;
	    curwin->w_p_nuw = 1;
	}
	if (curwin->w_p_nuw > 20)
	{
	    errmsg = e_invalid_argument;
	    curwin->w_p_nuw = 20;
	}
	curwin->w_nrwidth_line_count = 0; // trigger a redraw
    }
#endif

    else if (pp == &curbuf->b_p_tw)
    {
	if (curbuf->b_p_tw < 0)
	{
	    errmsg = e_argument_must_be_positive;
	    curbuf->b_p_tw = 0;
	}
#ifdef FEAT_SYN_HL
	{
	    win_T	*wp;
	    tabpage_T	*tp;

	    FOR_ALL_TAB_WINDOWS(tp, wp)
		check_colorcolumn(wp);
	}
#endif
    }

    /*
     * Check the bounds for numeric options here
     */
    if (Rows < min_rows() && full_screen)
    {
	if (errbuf != NULL)
	{
	    vim_snprintf(errbuf, errbuflen,
			       _(e_need_at_least_nr_lines), min_rows());
	    errmsg = errbuf;
	}
	Rows = min_rows();
    }
    if (Columns < MIN_COLUMNS && full_screen)
    {
	if (errbuf != NULL)
	{
	    vim_snprintf(errbuf, errbuflen,
			    _(e_need_at_least_nr_columns), MIN_COLUMNS);
	    errmsg = errbuf;
	}
	Columns = MIN_COLUMNS;
    }
    limit_screen_size();

    /*
     * If the screen (shell) height has been changed, assume it is the
     * physical screenheight.
     */
    if (old_Rows != Rows || old_Columns != Columns)
    {
	// Changing the screen size is not allowed while updating the screen.
	if (updating_screen)
	    *pp = old_value;
	else if (full_screen
#ifdef FEAT_GUI
		&& !gui.starting
#endif
	    )
	    set_shellsize((int)Columns, (int)Rows, TRUE);
	else
	{
	    // Postpone the resizing; check the size and cmdline position for
	    // messages.
	    check_shellsize();
	    if (cmdline_row > Rows - p_ch && Rows > p_ch)
		cmdline_row = Rows - p_ch;
	}
	if (p_window >= Rows || !option_was_set((char_u *)"window"))
	    p_window = Rows - 1;
    }

    if (curbuf->b_p_ts <= 0)
    {
	errmsg = e_argument_must_be_positive;
	curbuf->b_p_ts = 8;
    }
    if (p_tm < 0)
    {
	errmsg = e_argument_must_be_positive;
	p_tm = 0;
    }
    if ((curwin->w_p_scr <= 0
		|| (curwin->w_p_scr > curwin->w_height
		    && curwin->w_height > 0))
	    && full_screen)
    {
	if (pp == &(curwin->w_p_scr))
	{
	    if (curwin->w_p_scr != 0)
		errmsg = e_invalid_scroll_size;
	    win_comp_scroll(curwin);
	}
	// If 'scroll' became invalid because of a side effect silently adjust
	// it.
	else if (curwin->w_p_scr <= 0)
	    curwin->w_p_scr = 1;
	else // curwin->w_p_scr > curwin->w_height
	    curwin->w_p_scr = curwin->w_height;
    }
    if (p_hi < 0)
    {
	errmsg = e_argument_must_be_positive;
	p_hi = 0;
    }
    else if (p_hi > 10000)
    {
	errmsg = e_invalid_argument;
	p_hi = 10000;
    }
    if (p_re < 0 || p_re > 2)
    {
	errmsg = e_invalid_argument;
	p_re = 0;
    }
    if (p_report < 0)
    {
	errmsg = e_argument_must_be_positive;
	p_report = 1;
    }
    if ((p_sj < -100 || p_sj >= Rows) && full_screen)
    {
	if (Rows != old_Rows)	// Rows changed, just adjust p_sj
	    p_sj = Rows / 2;
	else
	{
	    errmsg = e_invalid_scroll_size;
	    p_sj = 1;
	}
    }
    if (p_so < 0 && full_screen)
    {
	errmsg = e_argument_must_be_positive;
	p_so = 0;
    }
    if (p_siso < 0 && full_screen)
    {
	errmsg = e_argument_must_be_positive;
	p_siso = 0;
    }
#ifdef FEAT_CMDWIN
    if (p_cwh < 1)
    {
	errmsg = e_argument_must_be_positive;
	p_cwh = 1;
    }
#endif
    if (p_ut < 0)
    {
	errmsg = e_argument_must_be_positive;
	p_ut = 2000;
    }
    if (p_ss < 0)
    {
	errmsg = e_argument_must_be_positive;
	p_ss = 0;
    }

    // May set global value for local option.
    if ((opt_flags & (OPT_LOCAL | OPT_GLOBAL)) == 0)
	*(long *)get_varp_scope(&(options[opt_idx]), OPT_GLOBAL) = *pp;

    options[opt_idx].flags |= P_WAS_SET;

#if defined(FEAT_EVAL)
    apply_optionset_autocmd(opt_idx, opt_flags, old_value, old_global_value,
								value, errmsg);
#endif

    comp_col();			    // in case 'columns' or 'ls' changed
    if (curwin->w_curswant != MAXCOL
		     && (options[opt_idx].flags & (P_CURSWANT | P_RALL)) != 0)
	curwin->w_set_curswant = TRUE;
    if ((opt_flags & OPT_NO_REDRAW) == 0)
	check_redraw(options[opt_idx].flags);

    return errmsg;
}