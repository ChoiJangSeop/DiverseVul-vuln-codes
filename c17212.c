EmitRange(NCURSES_SP_DCLx const NCURSES_CH_T *ntext, int num)
{
    int i;

    TR(TRACE_CHARPUT, ("EmitRange %d:%s", num, _nc_viscbuf(ntext, num)));

    if (erase_chars || repeat_char) {
	while (num > 0) {
	    int runcount;
	    NCURSES_CH_T ntext0;

	    while (num > 1 && !CharEq(ntext[0], ntext[1])) {
		PutChar(NCURSES_SP_ARGx CHREF(ntext[0]));
		ntext++;
		num--;
	    }
	    ntext0 = ntext[0];
	    if (num == 1) {
		PutChar(NCURSES_SP_ARGx CHREF(ntext0));
		return 0;
	    }
	    runcount = 2;

	    while (runcount < num && CharEq(ntext[runcount], ntext0))
		runcount++;

	    /*
	     * The cost expression in the middle isn't exactly right.
	     * _cup_ch_cost is an upper bound on the cost for moving to the
	     * end of the erased area, but not the cost itself (which we
	     * can't compute without emitting the move).  This may result
	     * in erase_chars not getting used in some situations for
	     * which it would be marginally advantageous.
	     */
	    if (erase_chars
		&& runcount > SP_PARM->_ech_cost + SP_PARM->_cup_ch_cost
		&& can_clear_with(NCURSES_SP_ARGx CHREF(ntext0))) {
		UpdateAttrs(SP_PARM, ntext0);
		NCURSES_PUTP2("erase_chars", TPARM_1(erase_chars, runcount));

		/*
		 * If this is the last part of the given interval,
		 * don't bother moving cursor, since it can be the
		 * last update on the line.
		 */
		if (runcount < num) {
		    GoTo(NCURSES_SP_ARGx
			 SP_PARM->_cursrow,
			 SP_PARM->_curscol + runcount);
		} else {
		    return 1;	/* cursor stays in the middle */
		}
	    } else if (repeat_char != 0 &&
#if BSD_TPUTS
		       !isdigit(UChar(CharOf(ntext0))) &&
#endif
#if USE_WIDEC_SUPPORT
		       (!SP_PARM->_screen_unicode &&
			(CharOf(ntext0) < ((AttrOf(ntext0) & A_ALTCHARSET)
					   ? ACS_LEN
					   : 256))) &&
#endif
		       runcount > SP_PARM->_rep_cost) {
		NCURSES_CH_T temp;
		bool wrap_possible = (SP_PARM->_curscol + runcount >=
				      screen_columns(SP_PARM));
		int rep_count = runcount;

		if (wrap_possible)
		    rep_count--;

		UpdateAttrs(SP_PARM, ntext0);
		temp = ntext0;
		if ((AttrOf(temp) & A_ALTCHARSET) &&
		    SP_PARM->_acs_map != 0 &&
		    (SP_PARM->_acs_map[CharOf(temp)] & A_CHARTEXT) != 0) {
		    SetChar(temp,
			    (SP_PARM->_acs_map[CharOf(ntext0)] & A_CHARTEXT),
			    AttrOf(ntext0) | A_ALTCHARSET);
		}
		NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
					TPARM_2(repeat_char,
						CharOf(temp),
						rep_count),
					1,
					NCURSES_SP_NAME(_nc_outch));
		SP_PARM->_curscol += rep_count;

		if (wrap_possible)
		    PutChar(NCURSES_SP_ARGx CHREF(ntext0));
	    } else {
		for (i = 0; i < runcount; i++)
		    PutChar(NCURSES_SP_ARGx CHREF(ntext[i]));
	    }
	    ntext += runcount;
	    num -= runcount;
	}
	return 0;
    }

    for (i = 0; i < num; i++)
	PutChar(NCURSES_SP_ARGx CHREF(ntext[i]));
    return 0;
}