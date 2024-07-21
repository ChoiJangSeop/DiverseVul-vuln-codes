InsStr(NCURSES_SP_DCLx NCURSES_CH_T *line, int count)
{
    TR(TRACE_UPDATE, ("InsStr(%p, %p,%d) called",
		      (void *) SP_PARM,
		      (void *) line, count));

    /* Prefer parm_ich as it has the smallest cost - no need to shift
     * the whole line on each character. */
    /* The order must match that of InsCharCost. */
    if (parm_ich) {
	TPUTS_TRACE("parm_ich");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_1(parm_ich, count),
				1,
				NCURSES_SP_NAME(_nc_outch));
	while (count > 0) {
	    PutAttrChar(NCURSES_SP_ARGx CHREF(*line));
	    line++;
	    count--;
	}
    } else if (enter_insert_mode && exit_insert_mode) {
	NCURSES_PUTP2("enter_insert_mode", enter_insert_mode);
	while (count > 0) {
	    PutAttrChar(NCURSES_SP_ARGx CHREF(*line));
	    if (insert_padding) {
		NCURSES_PUTP2("insert_padding", insert_padding);
	    }
	    line++;
	    count--;
	}
	NCURSES_PUTP2("exit_insert_mode", exit_insert_mode);
    } else {
	while (count > 0) {
	    NCURSES_PUTP2("insert_character", insert_character);
	    PutAttrChar(NCURSES_SP_ARGx CHREF(*line));
	    if (insert_padding) {
		NCURSES_PUTP2("insert_padding", insert_padding);
	    }
	    line++;
	    count--;
	}
    }
    position_check(NCURSES_SP_ARGx
		   SP_PARM->_cursrow,
		   SP_PARM->_curscol, "InsStr");
}