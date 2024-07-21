scroll_csr_backward(NCURSES_SP_DCLx
		    int n,
		    int top,
		    int bot,
		    int miny,
		    int maxy,
		    NCURSES_CH_T blank)
{
    int i;

    if (n == 1 && scroll_reverse && top == miny && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	NCURSES_PUTP2("scroll_reverse", scroll_reverse);
    } else if (n == 1 && insert_line && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	NCURSES_PUTP2("insert_line", insert_line);
    } else if (parm_rindex && top == miny && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	TPUTS_TRACE("parm_rindex");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_2(parm_rindex, n, 0),
				n,
				NCURSES_SP_NAME(_nc_outch));
    } else if (parm_insert_line && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	TPUTS_TRACE("parm_insert_line");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_2(parm_insert_line, n, 0),
				n,
				NCURSES_SP_NAME(_nc_outch));
    } else if (scroll_reverse && top == miny && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	for (i = 0; i < n; i++) {
	    NCURSES_PUTP2("scroll_reverse", scroll_reverse);
	}
    } else if (insert_line && bot == maxy) {
	GoTo(NCURSES_SP_ARGx top, 0);
	UpdateAttrs(SP_PARM, blank);
	for (i = 0; i < n; i++) {
	    NCURSES_PUTP2("insert_line", insert_line);
	}
    } else
	return ERR;

#if NCURSES_EXT_FUNCS
    if (FILL_BCE(SP_PARM)) {
	int j;
	for (i = 0; i < n; i++) {
	    GoTo(NCURSES_SP_ARGx top + i, 0);
	    for (j = 0; j < screen_columns(SP_PARM); j++)
		PutChar(NCURSES_SP_ARGx CHREF(blank));
	}
    }
#endif
    return OK;
}