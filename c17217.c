main(int argc, char *argv[])
{
    int ch;
    int fg, bg;
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

    while ((ch = getopt(argc, argv, "T:dem:r:s:")) != -1) {
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
	for (fg = 0; fg < COLORS; fg++) {
	    for (bg = 0; bg < COLORS; bg++) {
		int pair;
		if (interrupted) {
		    cleanup();
		    ExitProgram(EXIT_FAILURE);
		}
		pair = mypair(fg, bg);
		if (pair > 0)
		    init_pair((short) pair, (short) fg, (short) bg);
	    }
	}
    }

    r = (double) (LINES - (m_option * 2));
    c = (double) (COLS - (m_option * 2));
    started = time((time_t *) 0);

    fg = COLOR_WHITE;
    bg = COLOR_BLACK;
    while (!interrupted) {
	int x = (int) (c * ranf()) + m_option;
	int y = (int) (r * ranf()) + m_option;
	int p = (ranf() > 0.9) ? '*' : ' ';

	move(y, x);
	if (has_colors()) {
	    int z = (int) (ranf() * COLORS);
	    if (ranf() > 0.01) {
		set_colors(fg = z, bg);
		attron(COLOR_PAIR(mypair(fg, bg)));
	    } else {
		set_colors(fg, bg = z);
		napms(s_option);
	    }
	} else {
	    if (ranf() <= 0.01) {
		if (ranf() > 0.6) {
		    attron(A_REVERSE);
		} else {
		    attroff(A_REVERSE);
		}
		napms(s_option);
	    }
	}
	AddCh(p);
	refresh();
	++total_chars;
    }
    cleanup();
    ExitProgram(EXIT_SUCCESS);
}