drv_initcolor(TERMINAL_CONTROL_BLOCK * TCB,
	      int color, int r, int g, int b)
{
    SCREEN *sp = TCB->csp;

    AssertTCB();
    if (initialize_color != NULL) {
	NCURSES_PUTP2("initialize_color",
		      TPARM_4(initialize_color, color, r, g, b));
    }
}