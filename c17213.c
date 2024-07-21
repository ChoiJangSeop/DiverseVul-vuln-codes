tput_cmd(int fd, TTY * saved_settings, bool opt_x, int argc, char *argv[])
{
    NCURSES_CONST char *name;
    char *s;
    int status;
#if !PURE_TERMINFO
    bool termcap = FALSE;
#endif

    name = check_aliases(argv[0], FALSE);
    if (is_reset || is_init) {
	TTY oldmode;

	int terasechar = -1;	/* new erase character */
	int intrchar = -1;	/* new interrupt character */
	int tkillchar = -1;	/* new kill character */

	if (is_reset) {
	    reset_start(stdout, TRUE, FALSE);
	    reset_tty_settings(fd, saved_settings);
	} else {
	    reset_start(stdout, FALSE, TRUE);
	}

#if HAVE_SIZECHANGE
	set_window_size(fd, &lines, &columns);
#else
	(void) fd;
#endif
	set_control_chars(saved_settings, terasechar, intrchar, tkillchar);
	set_conversions(saved_settings);
	if (send_init_strings(fd, &oldmode)) {
	    reset_flush();
	}

	update_tty_settings(&oldmode, saved_settings);
	return 0;
    }

    if (strcmp(name, "longname") == 0) {
	PUTS(longname());
	return 0;
    }
#if !PURE_TERMINFO
  retry:
#endif
    if (strcmp(name, "clear") == 0) {
	return (clear_cmd(opt_x) == ERR) ? ErrUsage : 0;
    } else if ((status = tigetflag(name)) != -1) {
	return exit_code(BOOLEAN, status);
    } else if ((status = tigetnum(name)) != CANCELLED_NUMERIC) {
	(void) printf("%d\n", status);
	return exit_code(NUMBER, 0);
    } else if ((s = tigetstr(name)) == CANCELLED_STRING) {
#if !PURE_TERMINFO
	if (!termcap) {
	    const struct name_table_entry *np;

	    termcap = TRUE;
	    if ((np = _nc_find_entry(name, _nc_get_hash_table(termcap))) != 0) {
		switch (np->nte_type) {
		case BOOLEAN:
		    name = boolnames[np->nte_index];
		    break;

		case NUMBER:
		    name = numnames[np->nte_index];
		    break;

		case STRING:
		    name = strnames[np->nte_index];
		    break;
		}
		goto retry;
	    }
	}
#endif
	quit(ErrCapName, "unknown terminfo capability '%s'", name);
    } else if (VALID_STRING(s)) {
	if (argc > 1) {
	    int k;
	    int ignored;
	    long numbers[1 + NUM_PARM];
	    char *strings[1 + NUM_PARM];
	    char *p_is_s[NUM_PARM];

	    /* Nasty hack time. The tparm function needs to see numeric
	     * parameters as numbers, not as pointers to their string
	     * representations
	     */

	    for (k = 1; (k < argc) && (k < NUM_PARM); k++) {
		char *tmp = 0;
		strings[k] = argv[k];
		numbers[k] = strtol(argv[k], &tmp, 0);
		if (tmp == 0 || *tmp != 0)
		    numbers[k] = 0;
	    }
	    for (k = argc; k <= NUM_PARM; k++) {
		numbers[k] = 0;
		strings[k] = 0;
	    }

	    switch (tparm_type(name)) {
	    case Num_Str:
		s = TPARM_2(s, numbers[1], strings[2]);
		break;
	    case Num_Str_Str:
		s = TPARM_3(s, numbers[1], strings[2], strings[3]);
		break;
	    case Numbers:
	    default:
		(void) _nc_tparm_analyze(s, p_is_s, &ignored);
#define myParam(n) (p_is_s[n - 1] != 0 ? ((TPARM_ARG) strings[n]) : numbers[n])
		s = TPARM_9(s,
			    myParam(1),
			    myParam(2),
			    myParam(3),
			    myParam(4),
			    myParam(5),
			    myParam(6),
			    myParam(7),
			    myParam(8),
			    myParam(9));
		break;
	    }
	}

	/* use putp() in order to perform padding */
	putp(s);
	return exit_code(STRING, 0);
    }
    return exit_code(STRING, 1);
}