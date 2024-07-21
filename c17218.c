check_conflict(TERMTYPE2 *tp)
{
    bool conflict = FALSE;
    unsigned j, k;

    if (!(_nc_syntax == SYN_TERMCAP && capdump)) {
	char *check = calloc((size_t) (NUM_STRINGS(tp) + 1), sizeof(char));
	NAME_VALUE *given = get_fkey_list(tp);

	if (check == 0)
	    failed("check_conflict");

	for (j = 0; given[j].keycode; ++j) {
	    const char *a = given[j].value;
	    bool first = TRUE;

	    if (!VALID_STRING(a))
		continue;

	    for (k = j + 1; given[k].keycode; k++) {
		const char *b = given[k].value;

		if (!VALID_STRING(b))
		    continue;
		if (check[k])
		    continue;

		if (!_nc_capcmp(a, b)) {
		    check[j] = 1;
		    check[k] = 1;
		    if (first) {
			if (!conflict) {
			    _nc_warning("conflicting key definitions (using the last)");
			    conflict = TRUE;
			}
			fprintf(stderr, "...");
			show_fkey_name(given + j);
			fprintf(stderr, " is the same as");
			show_fkey_name(given + k);
			first = FALSE;
		    } else {
			fprintf(stderr, ", ");
			show_fkey_name(given + k);
		    }
		}
	    }
	    if (!first)
		fprintf(stderr, "\n");
	}
#if NCURSES_XNAMES
	if (using_extensions) {
	    /* *INDENT-OFF* */
	    static struct {
		const char *xcurses;
		const char *shifted;
	    } table[] = {
		{ "kDC",  NULL },
		{ "kDN",  "kind" },
		{ "kEND", NULL },
		{ "kHOM", NULL },
		{ "kLFT", NULL },
		{ "kNXT", NULL },
		{ "kPRV", NULL },
		{ "kRIT", NULL },
		{ "kUP",  "kri" },
		{ NULL,   NULL },
	    };
	    /* *INDENT-ON* */

	    /*
	     * SVr4 curses defines the "xcurses" names listed above except for
	     * the special cases in the "shifted" column.  When using these
	     * names for xterm's extensions, that was confusing, and resulted
	     * in adding extended capabilities with "2" (shift) suffix.  This
	     * check warns about unnecessary use of extensions for this quirk.
	     */
	    for (j = 0; given[j].keycode; ++j) {
		const char *find = given[j].name;
		int value;
		char ch;

		if (!VALID_STRING(given[j].value))
		    continue;

		for (k = 0; table[k].xcurses; ++k) {
		    const char *test = table[k].xcurses;
		    size_t size = strlen(test);

		    if (!strncmp(find, test, size) && strcmp(find, test)) {
			switch (sscanf(find + size, "%d%c", &value, &ch)) {
			case 1:
			    if (value == 2) {
				_nc_warning("expected '%s' rather than '%s'",
					    (table[k].shifted
					     ? table[k].shifted
					     : test), find);
			    } else if (value < 2 || value > 15) {
				_nc_warning("expected numeric 2..15 '%s'", find);
			    }
			    break;
			default:
			    _nc_warning("expected numeric suffix for '%s'", find);
			    break;
			}
			break;
		    }
		}
	    }
	}
#endif
	free(given);
	free(check);
    }
}