NCURSES_SP_NAME(vidputs) (NCURSES_SP_DCLx
			  chtype newmode,
			  NCURSES_SP_OUTC outc)
{
    attr_t turn_on, turn_off;
    int pair;
    bool reverse = FALSE;
    bool can_color = (SP_PARM == 0 || SP_PARM->_coloron);
#if NCURSES_EXT_FUNCS
    bool fix_pair0 = (SP_PARM != 0 && SP_PARM->_coloron && !SP_PARM->_default_color);
#else
#define fix_pair0 FALSE
#endif

    newmode &= A_ATTRIBUTES;

    T((T_CALLED("vidputs(%p,%s)"), (void *) SP_PARM, _traceattr(newmode)));

    if (!IsValidTIScreen(SP_PARM))
	returnCode(ERR);

    /* this allows us to go on whether or not newterm() has been called */
    if (SP_PARM)
	PreviousAttr = AttrOf(SCREEN_ATTRS(SP_PARM));

    TR(TRACE_ATTRS, ("previous attribute was %s", _traceattr(PreviousAttr)));

    if ((SP_PARM != 0)
	&& (magic_cookie_glitch > 0)) {
#if USE_XMC_SUPPORT
	static const chtype table[] =
	{
	    A_STANDOUT,
	    A_UNDERLINE,
	    A_REVERSE,
	    A_BLINK,
	    A_DIM,
	    A_BOLD,
	    A_INVIS,
	    A_PROTECT,
#if USE_ITALIC
	    A_ITALIC,
#endif
	};
	unsigned n;
	int used = 0;
#ifdef max_attributes		/* not in U/Win */
	int limit = (max_attributes <= 0) ? 1 : max_attributes;
#else
	int limit = 1;
#endif
	chtype retain = 0;

	/*
	 * Limit the number of attribute bits set in the newmode according to
	 * the terminfo max_attributes value.
	 */
	for (n = 0; n < SIZEOF(table); ++n) {
	    if ((table[n] & SP_PARM->_ok_attributes) == 0) {
		newmode &= ~table[n];
	    } else if ((table[n] & newmode) != 0) {
		if (used++ >= limit) {
		    newmode &= ~table[n];
		    if (newmode == retain)
			break;
		} else {
		    retain = newmode;
		}
	    }
	}
#else
	newmode &= ~(SP_PARM->_xmc_suppress);
#endif
	TR(TRACE_ATTRS, ("suppressed attribute is %s", _traceattr(newmode)));
    }

    /*
     * If we have a terminal that cannot combine color with video
     * attributes, use the colors in preference.
     */
    if (((newmode & A_COLOR) != 0
	 || fix_pair0)
	&& (no_color_video > 0)) {
	/*
	 * If we had chosen the A_xxx definitions to correspond to the
	 * no_color_video mask, we could simply shift it up and mask off the
	 * attributes.  But we did not (actually copied Solaris' definitions).
	 * However, this is still simpler/faster than a lookup table.
	 *
	 * The 63 corresponds to A_STANDOUT, A_UNDERLINE, A_REVERSE, A_BLINK,
	 * A_DIM, A_BOLD which are 1:1 with no_color_video.  The bits that
	 * correspond to A_INVIS, A_PROTECT (192) must be shifted up 1 and
	 * A_ALTCHARSET (256) down 2 to line up.  We use the NCURSES_BITS
	 * macro so this will work properly for the wide-character layout.
	 */
	unsigned value = (unsigned) no_color_video;
	attr_t mask = NCURSES_BITS((value & 63)
				   | ((value & 192) << 1)
				   | ((value & 256) >> 2), 8);

	if ((mask & A_REVERSE) != 0
	    && (newmode & A_REVERSE) != 0) {
	    reverse = TRUE;
	    mask &= ~A_REVERSE;
	}
	newmode &= ~mask;
    }

    if (newmode == PreviousAttr)
	returnCode(OK);

    pair = PairNumber(newmode);

    if (reverse) {
	newmode &= ~A_REVERSE;
    }

    turn_off = (~newmode & PreviousAttr) & ALL_BUT_COLOR;
    turn_on = (newmode & ~(PreviousAttr & TPARM_ATTR)) & ALL_BUT_COLOR;

    SetColorsIf(((pair == 0) && !fix_pair0), PreviousAttr);

    if (newmode == A_NORMAL) {
	if ((PreviousAttr & A_ALTCHARSET) && exit_alt_charset_mode) {
	    doPut(exit_alt_charset_mode);
	    PreviousAttr &= ~A_ALTCHARSET;
	}
	if (PreviousAttr) {
	    if (exit_attribute_mode) {
		doPut(exit_attribute_mode);
	    } else {
		if (!SP_PARM || SP_PARM->_use_rmul) {
		    TurnOff(A_UNDERLINE, exit_underline_mode);
		}
		if (!SP_PARM || SP_PARM->_use_rmso) {
		    TurnOff(A_STANDOUT, exit_standout_mode);
		}
#if USE_ITALIC
		if (!SP_PARM || SP_PARM->_use_ritm) {
		    TurnOff(A_ITALIC, exit_italics_mode);
		}
#endif
	    }
	    PreviousAttr &= ALL_BUT_COLOR;
	}

	SetColorsIf((pair != 0) || fix_pair0, PreviousAttr);
    } else if (set_attributes) {
	if (turn_on || turn_off) {
	    TPUTS_TRACE("set_attributes");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    tparm(set_attributes,
					  (newmode & A_STANDOUT) != 0,
					  (newmode & A_UNDERLINE) != 0,
					  (newmode & A_REVERSE) != 0,
					  (newmode & A_BLINK) != 0,
					  (newmode & A_DIM) != 0,
					  (newmode & A_BOLD) != 0,
					  (newmode & A_INVIS) != 0,
					  (newmode & A_PROTECT) != 0,
					  (newmode & A_ALTCHARSET) != 0),
				    1, outc);
	    PreviousAttr &= ALL_BUT_COLOR;
	}
#if USE_ITALIC
	if (!SP_PARM || SP_PARM->_use_ritm) {
	    if (turn_on & A_ITALIC) {
		TurnOn(A_ITALIC, enter_italics_mode);
	    } else if (turn_off & A_ITALIC) {
		TurnOff(A_ITALIC, exit_italics_mode);
	    }
	}
#endif
	SetColorsIf((pair != 0) || fix_pair0, PreviousAttr);
    } else {

	TR(TRACE_ATTRS, ("turning %s off", _traceattr(turn_off)));

	TurnOff(A_ALTCHARSET, exit_alt_charset_mode);

	if (!SP_PARM || SP_PARM->_use_rmul) {
	    TurnOff(A_UNDERLINE, exit_underline_mode);
	}

	if (!SP_PARM || SP_PARM->_use_rmso) {
	    TurnOff(A_STANDOUT, exit_standout_mode);
	}
#if USE_ITALIC
	if (!SP_PARM || SP_PARM->_use_ritm) {
	    TurnOff(A_ITALIC, exit_italics_mode);
	}
#endif
	if (turn_off && exit_attribute_mode) {
	    doPut(exit_attribute_mode);
	    turn_on |= (newmode & ALL_BUT_COLOR);
	    PreviousAttr &= ALL_BUT_COLOR;
	}
	SetColorsIf((pair != 0) || fix_pair0, PreviousAttr);

	TR(TRACE_ATTRS, ("turning %s on", _traceattr(turn_on)));
	/* *INDENT-OFF* */
	TurnOn(A_ALTCHARSET,	enter_alt_charset_mode);
	TurnOn(A_BLINK,		enter_blink_mode);
	TurnOn(A_BOLD,		enter_bold_mode);
	TurnOn(A_DIM,		enter_dim_mode);
	TurnOn(A_REVERSE,	enter_reverse_mode);
	TurnOn(A_STANDOUT,	enter_standout_mode);
	TurnOn(A_PROTECT,	enter_protected_mode);
	TurnOn(A_INVIS,		enter_secure_mode);
	TurnOn(A_UNDERLINE,	enter_underline_mode);
#if USE_ITALIC
	TurnOn(A_ITALIC,	enter_italics_mode);
#endif
#if USE_WIDEC_SUPPORT && defined(enter_horizontal_hl_mode)
	TurnOn(A_HORIZONTAL,	enter_horizontal_hl_mode);
	TurnOn(A_LEFT,		enter_left_hl_mode);
	TurnOn(A_LOW,		enter_low_hl_mode);
	TurnOn(A_RIGHT,		enter_right_hl_mode);
	TurnOn(A_TOP,		enter_top_hl_mode);
	TurnOn(A_VERTICAL,	enter_vertical_hl_mode);
#endif
	/* *INDENT-ON* */
    }

    if (reverse)
	newmode |= A_REVERSE;

    if (SP_PARM)
	SetAttr(SCREEN_ATTRS(SP_PARM), newmode);
    else
	PreviousAttr = newmode;

    returnCode(OK);
}