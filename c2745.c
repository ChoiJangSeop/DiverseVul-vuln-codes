process_pfa(FILE *ifp, const char *ifp_filename, struct font_reader *fr)
{
    /* Loop until no more input. We need to look for `currentfile eexec' to
       start eexec section (hex to binary conversion) and line of all zeros to
       switch back to ASCII. */

    /* Don't use fgets() in case line-endings are indicated by bare \r's, as
       occurs in Macintosh fonts. */

    /* 2.Aug.1999 - At the behest of Tom Kacvinsky <tjk@ams.org>, support
       binary PFA fonts. */

    char buffer[LINESIZE];
    int c = 0;
    int blocktyp = PFA_ASCII;
    char saved_orphan = 0;
    (void)ifp_filename;

    while (c != EOF) {
	char *line = buffer, *last = buffer;
	int crlf = 0;
	c = getc(ifp);
	while (c != EOF && c != '\r' && c != '\n' && last < buffer + LINESIZE - 1) {
	    *last++ = c;
	    c = getc(ifp);
	}

	/* handle the end of the line */
	if (last == buffer + LINESIZE - 1)
	    /* buffer overrun: don't append newline even if we have it */
	    ungetc(c, ifp);
	else if (c == '\r' && blocktyp != PFA_BINARY) {
	    /* change CR or CR/LF into LF, unless reading binary data! (This
	       condition was wrong before, caused Thanh problems -
	       6.Mar.2001) */
	    c = getc(ifp);
	    if (c != '\n')
		ungetc(c, ifp), crlf = 1;
	    else
		crlf = 2;
	    *last++ = '\n';
	} else if (c != EOF)
	    *last++ = c;

	*last = 0;

	/* now that we have the line, handle it */
	if (blocktyp == PFA_ASCII) {
	    if (strncmp(line, "currentfile eexec", 17) == 0 && isspace(line[17])) {
		char saved_p;
		/* assert(line == buffer); */
		for (line += 18; isspace(*line); line++)
		    /* nada */;
		saved_p = *line;
		*line = 0;
		fr->output_ascii(buffer, line - buffer);
		*line = saved_p;
		blocktyp = PFA_EEXEC_TEST;
		if (!*line)
		    continue;
	    } else {
		fr->output_ascii(line, last - line);
		continue;
	    }
	}

	/* check immediately after "currentfile eexec" for ASCII or binary */
	if (blocktyp == PFA_EEXEC_TEST) {
	    /* 8.Feb.2004: fix bug if first character in a binary eexec block
	       is 0, reported by Werner Lemberg */
	    for (; line < last && isspace(*line); line++)
		/* nada */;
	    if (line == last)
		continue;
	    else if (last >= line + 4 && isxdigit(line[0]) && isxdigit(line[1])
		     && isxdigit(line[2]) && isxdigit(line[3]))
		blocktyp = PFA_HEX;
	    else
		blocktyp = PFA_BINARY;
	    memmove(buffer, line, last - line + 1);
	    last = buffer + (last - line);
	    line = buffer;
	    /* patch up crlf fix */
	    if (blocktyp == PFA_BINARY && crlf) {
		last[-1] = '\r';
		if (crlf == 2)
		    *last++ = '\n';
	    }
	}

	/* blocktyp == PFA_HEX || blocktyp == PFA_BINARY */
	if (all_zeroes(line)) {	/* XXX not safe */
	    fr->output_ascii(line, last - line);
	    blocktyp = PFA_ASCII;
	} else if (blocktyp == PFA_HEX) {
	    int len = translate_hex_string(line, &saved_orphan);
	    if (len)
		fr->output_binary((unsigned char *)line, len);
	} else
	    fr->output_binary((unsigned char *)line, last - line);
    }

    fr->output_end();
}