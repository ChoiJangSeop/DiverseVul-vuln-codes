addChar(char c, Lineprop mode)
#endif
{
    Lineprop m = CharEffect(mode);
#ifdef USE_M17N
    char c = *p;

    if (mode & PC_WCHAR2)
	return;
#endif
    do_effects(m);
    if (mode & PC_SYMBOL) {
	char **symbol;
#ifdef USE_M17N
	int w = (mode & PC_KANJI) ? 2 : 1;

	c = ((char)wtf_get_code((wc_uchar *) p) & 0x7f) - SYMBOL_BASE;
#else
	c -= SYMBOL_BASE;
#endif
	if (graph_ok() && c < N_GRAPH_SYMBOL) {
	    if (!graph_mode) {
		graphstart();
		graph_mode = TRUE;
	    }
#ifdef USE_M17N
	    if (w == 2 && WcOption.use_wide)
		addstr(graph2_symbol[(int)c]);
	    else
#endif
		addch(*graph_symbol[(int)c]);
	}
	else {
#ifdef USE_M17N
	    symbol = get_symbol(DisplayCharset, &w);
	    addstr(symbol[(int)c]);
#else
	    symbol = get_symbol();
	    addch(*symbol[(int)c]);
#endif
	}
    }
    else if (mode & PC_CTRL) {
	switch (c) {
	case '\t':
	    addch(c);
	    break;
	case '\n':
	    addch(' ');
	    break;
	case '\r':
	    break;
	case DEL_CODE:
	    addstr("^?");
	    break;
	default:
	    addch('^');
	    addch(c + '@');
	    break;
	}
    }
#ifdef USE_M17N
    else if (mode & PC_UNKNOWN) {
	char buf[5];
	sprintf(buf, "[%.2X]",
		(unsigned char)wtf_get_code((wc_uchar *) p) | 0x80);
	addstr(buf);
    }
    else
	addmch(p, len);
#else
    else if (0x80 <= (unsigned char)c && (unsigned char)c <= NBSP_CODE)
	addch(' ');
    else
	addch(c);
#endif
}