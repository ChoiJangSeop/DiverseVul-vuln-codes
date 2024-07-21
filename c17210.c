onscreen_mvcur(NCURSES_SP_DCLx
	       int yold, int xold,
	       int ynew, int xnew, int ovw,
	       NCURSES_SP_OUTC myOutCh)
/* onscreen move from (yold, xold) to (ynew, xnew) */
{
    string_desc result;
    char buffer[OPT_SIZE];
    int tactic = 0, newcost, usecost = INFINITY;
    int t5_cr_cost;

#if defined(MAIN) || defined(NCURSES_TEST)
    struct timeval before, after;

    gettimeofday(&before, NULL);
#endif /* MAIN */

#define NullResult _nc_str_null(&result, sizeof(buffer))
#define InitResult _nc_str_init(&result, buffer, sizeof(buffer))

    /* tactic #0: use direct cursor addressing */
    if (_nc_safe_strcpy(InitResult, TPARM_2(SP_PARM->_address_cursor, ynew, xnew))) {
	tactic = 0;
	usecost = SP_PARM->_cup_cost;

#if defined(TRACE) || defined(NCURSES_TEST)
	if (!(_nc_optimize_enable & OPTIMIZE_MVCUR))
	    goto nonlocal;
#endif /* TRACE */

	/*
	 * We may be able to tell in advance that the full optimization
	 * will probably not be worth its overhead.  Also, don't try to
	 * use local movement if the current attribute is anything but
	 * A_NORMAL...there are just too many ways this can screw up
	 * (like, say, local-movement \n getting mapped to some obscure
	 * character because A_ALTCHARSET is on).
	 */
	if (yold == -1 || xold == -1 || NOT_LOCAL(SP_PARM, yold, xold, ynew, xnew)) {
#if defined(MAIN) || defined(NCURSES_TEST)
	    if (!profiling) {
		(void) fputs("nonlocal\n", stderr);
		goto nonlocal;	/* always run the optimizer if profiling */
	    }
#else
	    goto nonlocal;
#endif /* MAIN */
	}
    }
#ifndef NO_OPTIMIZE
    /* tactic #1: use local movement */
    if (yold != -1 && xold != -1
	&& ((newcost = relative_move(NCURSES_SP_ARGx
				     NullResult,
				     yold, xold,
				     ynew, xnew, ovw)) != INFINITY)
	&& newcost < usecost) {
	tactic = 1;
	usecost = newcost;
    }

    /* tactic #2: use carriage-return + local movement */
    if (yold != -1 && carriage_return
	&& ((newcost = relative_move(NCURSES_SP_ARGx
				     NullResult,
				     yold, 0,
				     ynew, xnew, ovw)) != INFINITY)
	&& SP_PARM->_cr_cost + newcost < usecost) {
	tactic = 2;
	usecost = SP_PARM->_cr_cost + newcost;
    }

    /* tactic #3: use home-cursor + local movement */
    if (cursor_home
	&& ((newcost = relative_move(NCURSES_SP_ARGx
				     NullResult,
				     0, 0,
				     ynew, xnew, ovw)) != INFINITY)
	&& SP_PARM->_home_cost + newcost < usecost) {
	tactic = 3;
	usecost = SP_PARM->_home_cost + newcost;
    }

    /* tactic #4: use home-down + local movement */
    if (cursor_to_ll
	&& ((newcost = relative_move(NCURSES_SP_ARGx
				     NullResult,
				     screen_lines(SP_PARM) - 1, 0,
				     ynew, xnew, ovw)) != INFINITY)
	&& SP_PARM->_ll_cost + newcost < usecost) {
	tactic = 4;
	usecost = SP_PARM->_ll_cost + newcost;
    }

    /*
     * tactic #5: use left margin for wrap to right-hand side,
     * unless strange wrap behavior indicated by xenl might hose us.
     */
    t5_cr_cost = (xold > 0 ? SP_PARM->_cr_cost : 0);
    if (auto_left_margin && !eat_newline_glitch
	&& yold > 0 && cursor_left
	&& ((newcost = relative_move(NCURSES_SP_ARGx
				     NullResult,
				     yold - 1, screen_columns(SP_PARM) - 1,
				     ynew, xnew, ovw)) != INFINITY)
	&& t5_cr_cost + SP_PARM->_cub1_cost + newcost < usecost) {
	tactic = 5;
	usecost = t5_cr_cost + SP_PARM->_cub1_cost + newcost;
    }

    /*
     * These cases are ordered by estimated relative frequency.
     */
    if (tactic)
	InitResult;
    switch (tactic) {
    case 1:
	(void) relative_move(NCURSES_SP_ARGx
			     &result,
			     yold, xold,
			     ynew, xnew, ovw);
	break;
    case 2:
	(void) _nc_safe_strcpy(&result, carriage_return);
	(void) relative_move(NCURSES_SP_ARGx
			     &result,
			     yold, 0,
			     ynew, xnew, ovw);
	break;
    case 3:
	(void) _nc_safe_strcpy(&result, cursor_home);
	(void) relative_move(NCURSES_SP_ARGx
			     &result, 0, 0,
			     ynew, xnew, ovw);
	break;
    case 4:
	(void) _nc_safe_strcpy(&result, cursor_to_ll);
	(void) relative_move(NCURSES_SP_ARGx
			     &result,
			     screen_lines(SP_PARM) - 1, 0,
			     ynew, xnew, ovw);
	break;
    case 5:
	if (xold > 0)
	    (void) _nc_safe_strcat(&result, carriage_return);
	(void) _nc_safe_strcat(&result, cursor_left);
	(void) relative_move(NCURSES_SP_ARGx
			     &result,
			     yold - 1, screen_columns(SP_PARM) - 1,
			     ynew, xnew, ovw);
	break;
    }
#endif /* !NO_OPTIMIZE */

  nonlocal:
#if defined(MAIN) || defined(NCURSES_TEST)
    gettimeofday(&after, NULL);
    diff = after.tv_usec - before.tv_usec
	+ (after.tv_sec - before.tv_sec) * 1000000;
    if (!profiling)
	(void) fprintf(stderr,
		       "onscreen: %d microsec, %f 28.8Kbps char-equivalents\n",
		       (int) diff, diff / 288);
#endif /* MAIN */

    if (usecost != INFINITY) {
	TR(TRACE_MOVE, ("mvcur tactic %d", tactic));
	TPUTS_TRACE("mvcur");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				buffer, 1, myOutCh);
	SP_PARM->_cursrow = ynew;
	SP_PARM->_curscol = xnew;
	return (OK);
    } else
	return (ERR);
}