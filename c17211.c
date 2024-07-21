drv_setcolor(TERMINAL_CONTROL_BLOCK * TCB,
	     int fore,
	     int color,
	     NCURSES_SP_OUTC outc)
{
    SCREEN *sp;

    AssertTCB();
    SetSP();

    if (fore) {
	if (set_a_foreground) {
	    TPUTS_TRACE("set_a_foreground");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    TPARM_1(set_a_foreground, color), 1, outc);
	} else {
	    TPUTS_TRACE("set_foreground");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    TPARM_1(set_foreground,
					    toggled_colors(color)), 1, outc);
	}
    } else {
	if (set_a_background) {
	    TPUTS_TRACE("set_a_background");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    TPARM_1(set_a_background, color), 1, outc);
	} else {
	    TPUTS_TRACE("set_background");
	    NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				    TPARM_1(set_background,
					    toggled_colors(color)), 1, outc);
	}
    }
}