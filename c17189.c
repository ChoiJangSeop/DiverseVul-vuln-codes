_nc_init_color(SCREEN *sp, int color, int r, int g, int b)
{
    int result = ERR;
    int maxcolors;

    T((T_CALLED("init_color(%p,%d,%d,%d,%d)"),
       (void *) sp,
       color,
       r, g, b));

    if (sp == 0 || sp->_direct_color.value)
	returnCode(result);

    maxcolors = MaxColors;

    if (InitColor
	&& sp->_coloron
	&& (color >= 0 && OkColorHi(color))
	&& (okRGB(r) && okRGB(g) && okRGB(b))) {

	sp->_color_table[color].init = 1;
	sp->_color_table[color].r = r;
	sp->_color_table[color].g = g;
	sp->_color_table[color].b = b;

	if (UseHlsPalette) {
	    rgb2hls(r, g, b,
		    &sp->_color_table[color].red,
		    &sp->_color_table[color].green,
		    &sp->_color_table[color].blue);
	} else {
	    sp->_color_table[color].red = r;
	    sp->_color_table[color].green = g;
	    sp->_color_table[color].blue = b;
	}

#ifdef USE_TERM_DRIVER
	CallDriver_4(sp, td_initcolor, color, r, g, b);
#else
	NCURSES_PUTP2("initialize_color",
		      TPARM_4(initialize_color, color, r, g, b));
#endif
	sp->_color_defs = max(color + 1, sp->_color_defs);

	result = OK;
    }
    returnCode(result);
}