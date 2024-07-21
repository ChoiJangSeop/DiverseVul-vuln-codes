check_params(TERMTYPE2 *tp, const char *name, char *value, int extended)
{
    int expected = expected_params(name);
    int actual = 0;
    int n;
    bool params[NUM_PARM];
    char *s = value;

#ifdef set_top_margin_parm
    if (!strcmp(name, "smgbp")
	&& set_top_margin_parm == 0)
	expected = 2;
#endif

    for (n = 0; n < NUM_PARM; n++)
	params[n] = FALSE;

    while (*s != 0) {
	if (*s == '%') {
	    if (*++s == '\0') {
		_nc_warning("expected character after %% in %s", name);
		break;
	    } else if (*s == 'p') {
		if (*++s == '\0' || !isdigit((int) *s)) {
		    _nc_warning("expected digit after %%p in %s", name);
		    return;
		} else {
		    n = (*s - '0');
		    if (n > actual)
			actual = n;
		    params[n] = TRUE;
		}
	    }
	}
	s++;
    }

#if NCURSES_XNAMES
    if (extended) {
	int check = is_user_capability(name);
	if (check != actual && (check >= 0 && actual >= 0)) {
	    _nc_warning("extended %s capability has %d parameters, expected %d",
			name, actual, check);
	} else if (debug_level > 1) {
	    _nc_warning("extended %s capability has %d parameters, as expected",
			name, actual);
	}
	expected = actual;
    }
#else
    (void) extended;
#endif

    if (params[0]) {
	_nc_warning("%s refers to parameter 0 (%%p0), which is not allowed", name);
    }
    if (value == set_attributes || expected < 0) {
	;
    } else if (expected != actual) {
	_nc_warning("%s uses %d parameters, expected %d", name,
		    actual, expected);
	for (n = 1; n < actual; n++) {
	    if (!params[n])
		_nc_warning("%s omits parameter %d", name, n);
	}
    }

    /*
     * Counting "%p" markers does not account for termcap expressions which
     * may not have been fully translated.  Also, tparm does its own analysis.
     * Report differences here.
     */
    if (actual >= 0) {
	char *p_is_s[NUM_PARM];
	int popcount;
	int analyzed = _nc_tparm_analyze(value, p_is_s, &popcount);
	if (analyzed < popcount) {
	    analyzed = popcount;
	}
	if (actual != analyzed && expected != analyzed) {
#if NCURSES_XNAMES
	    int user_cap = is_user_capability(name);
	    if ((user_cap == analyzed) && using_extensions) {
		;		/* ignore */
	    } else if (user_cap >= 0) {
		_nc_warning("tparm will use %d parameters for %s, expected %d",
			    analyzed, name, user_cap);
	    } else
#endif
	    {
		_nc_warning("tparm analyzed %d parameters for %s, expected %d",
			    analyzed, name, actual);
	    }
	}
    }
}