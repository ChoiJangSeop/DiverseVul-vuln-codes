set_background_color(NCURSES_SP_DCLx int bg, NCURSES_SP_OUTC outc)
{
#ifdef USE_TERM_DRIVER
    CallDriver_3(SP_PARM, td_color, FALSE, bg, outc);
#else
    if (set_a_background) {
	TPUTS_TRACE("set_a_background");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_1(set_a_background, bg),
				1, outc);
    } else {
	TPUTS_TRACE("set_background");
	NCURSES_SP_NAME(tputs) (NCURSES_SP_ARGx
				TPARM_1(set_background, toggled_colors(bg)),
				1, outc);
    }
#endif
}