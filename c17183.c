_nc_init_pair(SCREEN *sp, int pair, int f, int b)
{
    static colorpair_t null_pair;
    colorpair_t result = null_pair;
    colorpair_t previous;
    int maxcolors;

    T((T_CALLED("init_pair(%p,%d,%d,%d)"), (void *) sp, pair, f, b));

    if (!ValidPair(sp, pair))
	returnCode(ERR);

    maxcolors = MaxColors;

    ReservePairs(sp, pair);
    previous = sp->_color_pairs[pair];
#if NCURSES_EXT_FUNCS
    if (sp->_default_color || sp->_assumed_color) {
	bool isDefault = FALSE;
	bool wasDefault = FALSE;
	int default_pairs = sp->_default_pairs;

	/*
	 * Map caller's color number, e.g., -1, 0, 1, .., 7, etc., into
	 * internal unsigned values which we will store in the _color_pairs[]
	 * table.
	 */
	if (isDefaultColor(f)) {
	    f = COLOR_DEFAULT;
	    isDefault = TRUE;
	} else if (!OkColorHi(f)) {
	    returnCode(ERR);
	}

	if (isDefaultColor(b)) {
	    b = COLOR_DEFAULT;
	    isDefault = TRUE;
	} else if (!OkColorHi(b)) {
	    returnCode(ERR);
	}

	/*
	 * Check if the table entry that we are going to init/update used
	 * default colors.
	 */
	if (isDefaultColor(FORE_OF(previous))
	    || isDefaultColor(BACK_OF(previous)))
	    wasDefault = TRUE;

	/*
	 * Keep track of the number of entries in the color pair table which
	 * used a default color.
	 */
	if (isDefault && !wasDefault) {
	    ++default_pairs;
	} else if (wasDefault && !isDefault) {
	    --default_pairs;
	}

	/*
	 * As an extension, ncurses allows the pair number to exceed the
	 * terminal's color_pairs value for pairs using a default color.
	 *
	 * Note that updating a pair which used a default color with one
	 * that does not will decrement the count - and possibly interfere
	 * with sequentially adding new pairs.
	 */
	if (pair > (sp->_pair_count + default_pairs)) {
	    returnCode(ERR);
	}
	sp->_default_pairs = default_pairs;
    } else
#endif
    {
	if ((f < 0) || !OkColorHi(f)
	    || (b < 0) || !OkColorHi(b)
	    || (pair < 1)) {
	    returnCode(ERR);
	}
    }

    /*
     * When a pair's content is changed, replace its colors (if pair was
     * initialized before a screen update is performed replacing original
     * pair colors with the new ones).
     */
    MakeColorPair(result, f, b);
    if ((FORE_OF(previous) != 0
	 || BACK_OF(previous) != 0)
	&& !isSamePair(previous, result)) {
	_nc_change_pair(sp, pair);
    }

    _nc_reset_color_pair(sp, pair, &result);
    sp->_color_pairs[pair] = result;
    _nc_set_color_pair(sp, pair, cpINIT);

    if (GET_SCREEN_PAIR(sp) == pair)
	SET_SCREEN_PAIR(sp, (int) (~0));	/* force attribute update */

#ifdef USE_TERM_DRIVER
    CallDriver_3(sp, td_initpair, pair, f, b);
#else
    if (initialize_pair && InPalette(f) && InPalette(b)) {
	const color_t *tp = DefaultPalette;

	TR(TRACE_ATTRS,
	   ("initializing pair: pair = %d, fg=(%d,%d,%d), bg=(%d,%d,%d)",
	    (int) pair,
	    (int) tp[f].red, (int) tp[f].green, (int) tp[f].blue,
	    (int) tp[b].red, (int) tp[b].green, (int) tp[b].blue));

	NCURSES_PUTP2("initialize_pair",
		      TPARM_7(initialize_pair,
			      pair,
			      (int) tp[f].red,
			      (int) tp[f].green,
			      (int) tp[f].blue,
			      (int) tp[b].red,
			      (int) tp[b].green,
			      (int) tp[b].blue));
    }
#endif

    returnCode(OK);
}