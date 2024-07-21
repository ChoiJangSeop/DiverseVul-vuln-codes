main(int argc, char *argv[])
{
    int ch;
    int num_colors;
    int num_lines;
    int num_columns;
    int e_option = 0;
    int m_option = 2;
    int r_option = 0;
    int s_option = 1;
    double r;
    double c;
    char buffer[1024];
    char area[1024];
    char *name;
    size_t need;
    char *my_env;

    while ((ch = getopt(argc, argv, "T:em:r:s:")) != -1) {
	switch (ch) {
	case 'T':
	    need = 6 + strlen(optarg);
	    my_env = malloc(need);
	    _nc_SPRINTF(my_env, _nc_SLIMIT(need) "TERM=%s", optarg);
	    putenv(my_env);
	    break;
	case 'e':
	    e_option = 1;
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

    if ((name = getenv("TERM")) == 0) {
	fprintf(stderr, "TERM is not set\n");
	ExitProgram(EXIT_FAILURE);
    }

    srand((unsigned) time(0));

    SetupAlarm((unsigned) r_option);
    InitAndCatch(ch = tgetent(buffer, name), onsig);
    if (ch < 0) {
	fprintf(stderr, "terminal description not found\n");
	ExitProgram(EXIT_FAILURE);
    } else {
	size_t t;
	char *ap = area;
	for (t = 0; t < SIZEOF(my_caps); ++t) {
	    *(my_caps[t].value) = tgetstr((NCURSES_CONST char *)
					  my_caps[t].name, &ap);
	}
    }

    num_colors = tgetnum("Co");
#define GetNumber(cap,env) get_number(cap, e_option ? env : 0)
    num_lines = GetNumber("li", "LINES");
    num_columns = GetNumber("co", "COLUMNS");

    outs(t_cl);
    outs(t_vi);
    if (num_colors > 1) {
	if (!VALID_STRING(t_AF)
	    || !VALID_STRING(t_AB)
	    || (!VALID_STRING(t_oc) && !VALID_STRING(t_op)))
	    num_colors = -1;
    }

    r = (double) (num_lines - (2 * m_option));
    c = (double) (num_columns - (2 * m_option));
    started = time((time_t *) 0);

    while (!interrupted) {
	int x = (int) (c * ranf()) + m_option;
	int y = (int) (r * ranf()) + m_option;
	int p = (ranf() > 0.9) ? '*' : ' ';

	tputs(tgoto(t_cm, x, y), 1, outc);
	if (num_colors > 0) {
	    int z = (int) (ranf() * num_colors);
	    if (ranf() > 0.01) {
		tputs(tgoto(t_AF, 0, z), 1, outc);
	    } else {
		tputs(tgoto(t_AB, 0, z), 1, outc);
		my_napms(s_option);
	    }
	} else if (VALID_STRING(t_me)
		   && VALID_STRING(t_mr)) {
	    if (ranf() <= 0.01) {
		outs((ranf() > 0.6)
		     ? t_mr
		     : t_me);
		my_napms(s_option);
	    }
	}
	outc(p);
	fflush(stdout);
	++total_chars;
    }
    cleanup();
    ExitProgram(EXIT_SUCCESS);
}