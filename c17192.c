scroll_idl(NCURSES_SP_DCLx int n, int del, int ins, NCURSES_CH_T blank)
{
    int i;

    if (!((parm_delete_line || delete_line) && (parm_insert_line || insert_line)))
	return ERR;

    GoTo(NCURSES_SP_ARGx del, 0);
    UpdateAttrs(SP_PARM, blank);
    if (n == 1 && delete_line) {
	NCURSES_PUTP2("delete_line", delete_line);
    } else if (parm_delete_line) {
	TPUTS_TRACE("parm_delete_line");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_2(parm_delete_line, n, 0),
				n,
				NCURSES_SP_NAME(_nc_outch));
    } else {			/* if (delete_line) */
	for (i = 0; i < n; i++) {
	    NCURSES_PUTP2("delete_line", delete_line);
	}
    }

    GoTo(NCURSES_SP_ARGx ins, 0);
    UpdateAttrs(SP_PARM, blank);
    if (n == 1 && insert_line) {
	NCURSES_PUTP2("insert_line", insert_line);
    } else if (parm_insert_line) {
	TPUTS_TRACE("parm_insert_line");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_2(parm_insert_line, n, 0),
				n,
				NCURSES_SP_NAME(_nc_outch));
    } else {			/* if (insert_line) */
	for (i = 0; i < n; i++) {
	    NCURSES_PUTP2("insert_line", insert_line);
	}
    }

    return OK;
}