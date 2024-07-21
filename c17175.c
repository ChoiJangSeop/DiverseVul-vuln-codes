drv_do_color(TERMINAL_CONTROL_BLOCK * TCB,
	     int old_pair,
	     int pair,
	     int reverse,
	     NCURSES_SP_OUTC outc)
{
    SCREEN *sp = TCB->csp;
    int fg = COLOR_DEFAULT;
    int bg = COLOR_DEFAULT;
    int old_fg, old_bg;

    AssertTCB();
    if (sp == 0)
	return;

    if (pair < 0 || pair >= COLOR_PAIRS) {
	return;
    } else if (pair != 0) {
	if (set_color_pair) {
	    TPUTS_TRACE("set_color_pair");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    TPARM_1(set_color_pair, pair), 1, outc);
	    return;
	} else if (sp != 0) {
	    _nc_pair_content(SP_PARM, pair, &fg, &bg);
	}
    }

    if (old_pair >= 0
	&& sp != 0
	&& _nc_pair_content(SP_PARM, old_pair, &old_fg, &old_bg) != ERR) {
	if ((isDefaultColor(fg) && !isDefaultColor(old_fg))
	    || (isDefaultColor(bg) && !isDefaultColor(old_bg))) {
#if NCURSES_EXT_FUNCS
	    /*
	     * A minor optimization - but extension.  If "AX" is specified in
	     * the terminal description, treat it as screen's indicator of ECMA
	     * SGR 39 and SGR 49, and assume the two sequences are independent.
	     */
	    if (sp->_has_sgr_39_49
		&& isDefaultColor(old_bg)
		&& !isDefaultColor(old_fg)) {
		NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx "\033[39m", 1, outc);
	    } else if (sp->_has_sgr_39_49
		       && isDefaultColor(old_fg)
		       && !isDefaultColor(old_bg)) {
		NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx "\033[49m", 1, outc);
	    } else
#endif
		drv_rescol(TCB);
	}
    } else {
	drv_rescol(TCB);
	if (old_pair < 0)
	    return;
    }

#if NCURSES_EXT_FUNCS
    if (isDefaultColor(fg))
	fg = default_fg(sp);
    if (isDefaultColor(bg))
	bg = default_bg(sp);
#endif

    if (reverse) {
	int xx = fg;
	fg = bg;
	bg = xx;
    }

    TR(TRACE_ATTRS, ("setting colors: pair = %d, fg = %d, bg = %d", pair,
		     fg, bg));

    if (!isDefaultColor(fg)) {
	drv_setcolor(TCB, TRUE, fg, outc);
    }
    if (!isDefaultColor(bg)) {
	drv_setcolor(TCB, FALSE, bg, outc);
    }
}