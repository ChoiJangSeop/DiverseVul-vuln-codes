reset_scroll_region(NCURSES_SP_DCL0)
/* Set the scroll-region to a known state (the default) */
{
    if (change_scroll_region) {
	NCURSES_PUTP2("change_scroll_region",
		      TPARM_2(change_scroll_region,
			      0, screen_lines(SP_PARM) - 1));
    }
}