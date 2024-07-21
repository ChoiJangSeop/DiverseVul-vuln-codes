static int _assemble_line(FILE *f, char *buffer, int buf_len)
{
    char *p = buffer;
    char *s, *os;
    int used = 0;

    /* loop broken with a 'break' when a non-'\\n' ended line is read */

    D(("called."));
    for (;;) {
	if (used >= buf_len) {
	    /* Overflow */
	    D(("_assemble_line: overflow"));
	    return -1;
	}
	if (fgets(p, buf_len - used, f) == NULL) {
	    if (used) {
		/* Incomplete read */
		return -1;
	    } else {
		/* EOF */
		return 0;
	    }
	}

	/* skip leading spaces --- line may be blank */

	s = p + strspn(p, " \n\t");
	if (*s && (*s != '#')) {
	    os = s;

	    /*
	     * we are only interested in characters before the first '#'
	     * character
	     */

	    while (*s && *s != '#')
		 ++s;
	    if (*s == '#') {
		 *s = '\0';
		 used += strlen(os);
		 break;                /* the line has been read */
	    }

	    s = os;

	    /*
	     * Check for backslash by scanning back from the end of
	     * the entered line, the '\n' has been included since
	     * normally a line is terminated with this
	     * character. fgets() should only return one though!
	     */

	    s += strlen(s);
	    while (s > os && ((*--s == ' ') || (*s == '\t')
			      || (*s == '\n')));

	    /* check if it ends with a backslash */
	    if (*s == '\\') {
		*s = '\0';              /* truncate the line here */
		used += strlen(os);
		p = s;                  /* there is more ... */
	    } else {
		/* End of the line! */
		used += strlen(os);
		break;                  /* this is the complete line */
	    }

	} else {
	    /* Nothing in this line */
	    /* Don't move p         */
	}
    }

    return used;
}