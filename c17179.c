NCURSES_SP_NAME(_nc_scrolln) (NCURSES_SP_DCLx
			      int n,
			      int top,
			      int bot,
			      int maxy)
/* scroll region from top to bot by n lines */
{
    NCURSES_CH_T blank;
    int i;
    bool cursor_saved = FALSE;
    int res;

    TR(TRACE_MOVE, ("_nc_scrolln(%p, %d, %d, %d, %d)",
		    (void *) SP_PARM, n, top, bot, maxy));

    if (!IsValidScreen(SP_PARM))
	return (ERR);

    blank = ClrBlank(NCURSES_SP_ARGx StdScreen(SP_PARM));

#if USE_XMC_SUPPORT
    /*
     * If we scroll, we might remove a cookie.
     */
    if (magic_cookie_glitch > 0) {
	return (ERR);
    }
#endif

    if (n > 0) {		/* scroll up (forward) */
	/*
	 * Explicitly clear if stuff pushed off top of region might
	 * be saved by the terminal.
	 */
	res = scroll_csr_forward(NCURSES_SP_ARGx n, top, bot, 0, maxy, blank);

	if (res == ERR && change_scroll_region) {
	    if ((((n == 1 && scroll_forward) || parm_index)
		 && (SP_PARM->_cursrow == bot || SP_PARM->_cursrow == bot - 1))
		&& save_cursor && restore_cursor) {
		cursor_saved = TRUE;
		NCURSES_PUTP2("save_cursor", save_cursor);
	    }
	    NCURSES_PUTP2("change_scroll_region",
			  TPARM_2(change_scroll_region, top, bot));
	    if (cursor_saved) {
		NCURSES_PUTP2("restore_cursor", restore_cursor);
	    } else {
		SP_PARM->_cursrow = SP_PARM->_curscol = -1;
	    }

	    res = scroll_csr_forward(NCURSES_SP_ARGx n, top, bot, top, bot, blank);

	    NCURSES_PUTP2("change_scroll_region",
			  TPARM_2(change_scroll_region, 0, maxy));
	    SP_PARM->_cursrow = SP_PARM->_curscol = -1;
	}

	if (res == ERR && SP_PARM->_nc_sp_idlok)
	    res = scroll_idl(NCURSES_SP_ARGx n, top, bot - n + 1, blank);

	/*
	 * Clear the newly shifted-in text.
	 */
	if (res != ERR
	    && (non_dest_scroll_region || (memory_below && bot == maxy))) {
	    static const NCURSES_CH_T blank2 = NewChar(BLANK_TEXT);
	    if (bot == maxy && clr_eos) {
		GoTo(NCURSES_SP_ARGx bot - n + 1, 0);
		ClrToEOS(NCURSES_SP_ARGx blank2);
	    } else {
		for (i = 0; i < n; i++) {
		    GoTo(NCURSES_SP_ARGx bot - i, 0);
		    ClrToEOL(NCURSES_SP_ARGx blank2, FALSE);
		}
	    }
	}

    } else {			/* (n < 0) - scroll down (backward) */
	res = scroll_csr_backward(NCURSES_SP_ARGx -n, top, bot, 0, maxy, blank);

	if (res == ERR && change_scroll_region) {
	    if (top != 0
		&& (SP_PARM->_cursrow == top ||
		    SP_PARM->_cursrow == top - 1)
		&& save_cursor && restore_cursor) {
		cursor_saved = TRUE;
		NCURSES_PUTP2("save_cursor", save_cursor);
	    }
	    NCURSES_PUTP2("change_scroll_region",
			  TPARM_2(change_scroll_region, top, bot));
	    if (cursor_saved) {
		NCURSES_PUTP2("restore_cursor", restore_cursor);
	    } else {
		SP_PARM->_cursrow = SP_PARM->_curscol = -1;
	    }

	    res = scroll_csr_backward(NCURSES_SP_ARGx
				      -n, top, bot, top, bot, blank);

	    NCURSES_PUTP2("change_scroll_region",
			  TPARM_2(change_scroll_region, 0, maxy));
	    SP_PARM->_cursrow = SP_PARM->_curscol = -1;
	}

	if (res == ERR && SP_PARM->_nc_sp_idlok)
	    res = scroll_idl(NCURSES_SP_ARGx -n, bot + n + 1, top, blank);

	/*
	 * Clear the newly shifted-in text.
	 */
	if (res != ERR
	    && (non_dest_scroll_region || (memory_above && top == 0))) {
	    static const NCURSES_CH_T blank2 = NewChar(BLANK_TEXT);
	    for (i = 0; i < -n; i++) {
		GoTo(NCURSES_SP_ARGx i + top, 0);
		ClrToEOL(NCURSES_SP_ARGx blank2, FALSE);
	    }
	}
    }

    if (res == ERR)
	return (ERR);

    _nc_scroll_window(CurScreen(SP_PARM), n,
		      (NCURSES_SIZE_T) top,
		      (NCURSES_SIZE_T) bot,
		      blank);

    /* shift hash values too - they can be reused */
    NCURSES_SP_NAME(_nc_scroll_oldhash) (NCURSES_SP_ARGx n, top, bot);

    return (OK);
}