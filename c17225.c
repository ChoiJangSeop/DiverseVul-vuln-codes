set_foreground_color(NCURSES_SP_DCLx int fg, NCURSES_SP_OUTC outc)
{
#ifdef USE_TERM_DRIVER
    CallDriver_3(SP_PARM, td_color, TRUE, fg, outc);
#else
    if (set_a_foreground) {
	TPUTS_TRACE("set_a_foreground");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_1(set_a_foreground, fg),
				1, outc);
    } else {
	TPUTS_TRACE("set_foreground");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_1(set_foreground, toggled_colors(fg)),
				1, outc);
    }
#endif
}