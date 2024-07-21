main(int argc, char *argv[])
{
    int fg, bg, ch;
    wchar_t wch[2];
    double r;
    double c;
#if HAVE_USE_DEFAULT_COLORS
    bool d_option = FALSE;
#endif
    int m_option = 2;
    int r_option = 0;
    int s_option = 1;
    size_t need;
    char *my_env;

    while ((ch = getopt(argc, argv, "T:dem:r:s:x")) != -1) {
	switch (ch) {
	case 'T':
	    need = 6 + strlen(optarg);
	    my_env = malloc(need);
	    _nc_SPRINTF(my_env, _nc_SLIMIT(need) "TERM=%s", optarg);
	    putenv(my_env);
	    break;
#if HAVE_USE_DEFAULT_COLORS
	case 'd':
	    d_option = TRUE;
	    break;
#endif
#if HAVE_USE_ENV
	case 'e':
	    use_env(TRUE);
	    break;
#endif
	case 'm':
	    m_option = atoi(optarg);
	    break;
	case 'r':
	    r_option = atoi(optarg);
	    break;
	case 's':
	    s_option = atoi(optarg);
	    break;
#if HAVE_ALLOC_PAIR
	case 'x':
	    x_option = TRUE;
	    break;
#endif
	default:
	    usage();
	    break;
	}
    }

    srand((unsigned) time(0));

    SetupAlarm(r_option);
    InitAndCatch(initscr(), onsig);
    if (has_colors()) {
	start_color();
#if HAVE_USE_DEFAULT_COLORS
	if (d_option)
	    use_default_colors();
#endif
#if HAVE_ALLOC_PAIR
	if (x_option) {
	    ;			/* nothing */
	} else
#endif
	{
	    for (fg = 0; fg < COLORS; fg++) {
		for (bg = 0; bg < COLORS; bg++) {
		    int pair;
		    if (interrupted) {
			cleanup();
			ExitProgram(EXIT_FAILURE);
		    }
		    pair = mypair(fg, bg);
		    if (pair > 0) {
			InitPair(pair, fg, bg);
		    }
		}
	    }
	}
    }

    r = (double) (LINES - (2 * m_option));
    c = (double) (COLS - (2 * m_option));
    started = time((time_t *) 0);

    fg = COLOR_WHITE;
    bg = COLOR_BLACK;
    wch[1] = 0;
    while (!interrupted) {
	int x = (int) (c * ranf()) + m_option;
	int y = (int) (r * ranf()) + m_option;
	int p = (ranf() > 0.9) ? '*' : ' ';

	move(y, x);
	if (has_colors()) {
	    int z = (int) (ranf() * COLORS);
	    if (ranf() > 0.01) {
		set_colors(fg = z, bg);
	    } else {
		set_colors(fg, bg = z);
		napms(s_option);
	    }
	} else {
	    if (ranf() <= 0.01) {
		if (ranf() > 0.6) {
		    attr_on(WA_REVERSE, NULL);
		} else {
		    attr_off(WA_REVERSE, NULL);
		}
		napms(s_option);
	    }
	}
	wch[0] = (wchar_t) p;
	addnwstr(wch, 1);
	refresh();
	++total_chars;
    }
    cleanup();
    ExitProgram(EXIT_SUCCESS);
}