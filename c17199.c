drv_initpair(TERMINAL_CONTROL_BLOCK * TCB, int pair, int f, int b)
{
    SCREEN *sp;

    AssertTCB();
    SetSP();

    if ((initialize_pair != NULL) && InPalette(f) && InPalette(b)) {
	const color_t *tp = InfoOf(sp).defaultPalette;

	TR(TRACE_ATTRS,
	   ("initializing pair: pair = %d, fg=(%d,%d,%d), bg=(%d,%d,%d)",
	    pair,
	    tp[f].red, tp[f].green, tp[f].blue,
	    tp[b].red, tp[b].green, tp[b].blue));

	NCURSES_PUTP2("initialize_pair",
		      TPARM_7(initialize_pair,
			      pair,
			      tp[f].red, tp[f].green, tp[f].blue,
			      tp[b].red, tp[b].green, tp[b].blue));
    }
}