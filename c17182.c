position_check(NCURSES_SP_DCLx int expected_y, int expected_x, char *legend)
/* check to see if the real cursor position matches the virtual */
{
    char buf[20];
    char *s;
    int y, x;

    if (!_nc_tracing || (expected_y < 0 && expected_x < 0))
	return;

    NCURSES_SP_NAME(_nc_flush) (NCURSES_SP_ARG);
    memset(buf, '\0', sizeof(buf));
    NCURSES_PUTP2_FLUSH("cpr", "\033[6n");	/* only works on ANSI-compatibles */
    *(s = buf) = 0;
    do {
	int ask = sizeof(buf) - 1 - (s - buf);
	int got = read(0, s, ask);
	if (got == 0)
	    break;
	s += got;
    } while (strchr(buf, 'R') == 0);
    _tracef("probe returned %s", _nc_visbuf(buf));

    /* try to interpret as a position report */
    if (sscanf(buf, "\033[%d;%dR", &y, &x) != 2) {
	_tracef("position probe failed in %s", legend);
    } else {
	if (expected_x < 0)
	    expected_x = x - 1;
	if (expected_y < 0)
	    expected_y = y - 1;
	if (y - 1 != expected_y || x - 1 != expected_x) {
	    NCURSES_SP_NAME(beep) (NCURSES_SP_ARG);
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    tparm("\033[%d;%dH",
					  expected_y + 1,
					  expected_x + 1),
				    1, NCURSES_SP_NAME(_nc_outch));
	    _tracef("position seen (%d, %d) doesn't match expected one (%d, %d) in %s",
		    y - 1, x - 1, expected_y, expected_x, legend);
	} else {
	    _tracef("position matches OK in %s", legend);
	}
    }
}