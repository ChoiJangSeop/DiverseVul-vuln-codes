main(int argc,
     char *argv[])
{
    int ch;
    double r;
    double c;
    int my_colors;
    int f_option = 0;
    int m_option = 2;
    int r_option = 0;
    int s_option = 1;
    size_t need;
    char *my_env;

    while ((ch = getopt(argc, argv, "T:efm:r:s:")) != -1) {
	switch (ch) {
	case 'T':
	    need = 6 + strlen(optarg);
	    my_env = malloc(need);
	    _nc_SPRINTF(my_env, _nc_SLIMIT(need) "TERM=%s", optarg);
	    putenv(my_env);
	    break;
#if HAVE_USE_ENV
	case 'e':
	    use_env(TRUE);
	    break;
#endif
	case 'f':
	    f_option = 1;
	    break;
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

    SetupAlarm(r_option);
    InitAndCatch(setupterm((char *) 0, 1, (int *) 0), onsig);

    srand((unsigned) time(0));

    outs(clear_screen);
    outs(cursor_invisible);

#define GetNumber(ln,sn) get_number(f_option ? #sn : 0, ln)
    my_colors = GetNumber(max_colors, colors);
    if (my_colors > 1) {
	if (!VALID_STRING(set_a_foreground)
	    || !VALID_STRING(set_a_background)
	    || (!VALID_STRING(orig_colors) && !VALID_STRING(orig_pair)))
	    my_colors = -1;
    }

    r = (double) (GetNumber(lines, lines) - (m_option * 2));
    c = (double) (GetNumber(columns, cols) - (m_option * 2));
    started = time((time_t *) 0);

    while (!interrupted) {
	int x = (int) (c * ranf()) + m_option;
	int y = (int) (r * ranf()) + m_option;
	int p = (ranf() > 0.9) ? '*' : ' ';

	tputs(tparm3(cursor_address, y, x), 1, outc);
	if (my_colors > 0) {
	    int z = (int) (ranf() * my_colors);
	    if (ranf() > 0.01) {
		tputs(tparm2(set_a_foreground, z), 1, outc);
	    } else {
		tputs(tparm2(set_a_background, z), 1, outc);
		napms(s_option);
	    }
	} else if (VALID_STRING(exit_attribute_mode)
		   && VALID_STRING(enter_reverse_mode)) {
	    if (ranf() <= 0.01) {
		outs((ranf() > 0.6)
		     ? enter_reverse_mode
		     : exit_attribute_mode);
		napms(s_option);
	    }
	}
	outc(p);
	fflush(stdout);
	++total_chars;
    }
    cleanup();
    ExitProgram(EXIT_SUCCESS);
}