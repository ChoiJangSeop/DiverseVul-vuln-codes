do_ed_script (char const *inname, char const *outname,
	      bool *outname_needs_removal, FILE *ofp)
{
    static char const editor_program[] = EDITOR_PROGRAM;

    file_offset beginning_of_this_line;
    FILE *pipefp = 0;
    size_t chars_read;

    if (! dry_run && ! skip_rest_of_patch) {
	int exclusive = *outname_needs_removal ? 0 : O_EXCL;
	if (inerrno != ENOENT)
	  {
	    *outname_needs_removal = true;
	    copy_file (inname, outname, 0, exclusive, instat.st_mode, true);
	  }
	sprintf (buf, "%s %s%s", editor_program,
		 verbosity == VERBOSE ? "" : "- ",
		 outname);
	fflush (stdout);
	pipefp = popen(buf, binary_transput ? "wb" : "w");
	if (!pipefp)
	  pfatal ("Can't open pipe to %s", quotearg (buf));
    }
    for (;;) {
	char ed_command_letter;
	beginning_of_this_line = file_tell (pfp);
	chars_read = get_line ();
	if (! chars_read) {
	    next_intuit_at(beginning_of_this_line,p_input_line);
	    break;
	}
	ed_command_letter = get_ed_command_letter (buf);
	if (ed_command_letter) {
	    if (pipefp)
		if (! fwrite (buf, sizeof *buf, chars_read, pipefp))
		    write_fatal ();
	    if (ed_command_letter != 'd' && ed_command_letter != 's') {
	        p_pass_comments_through = true;
		while ((chars_read = get_line ()) != 0) {
		    if (pipefp)
			if (! fwrite (buf, sizeof *buf, chars_read, pipefp))
			    write_fatal ();
		    if (chars_read == 2  &&  strEQ (buf, ".\n"))
			break;
		}
		p_pass_comments_through = false;
	    }
	}
	else {
	    next_intuit_at(beginning_of_this_line,p_input_line);
	    break;
	}
    }
    if (!pipefp)
      return;
    if (fwrite ("w\nq\n", sizeof (char), (size_t) 4, pipefp) == 0
	|| fflush (pipefp) != 0)
      write_fatal ();
    if (pclose (pipefp) != 0)
      fatal ("%s FAILED", editor_program);

    if (ofp)
      {
	FILE *ifp = fopen (outname, binary_transput ? "rb" : "r");
	int c;
	if (!ifp)
	  pfatal ("can't open '%s'", outname);
	while ((c = getc (ifp)) != EOF)
	  if (putc (c, ofp) == EOF)
	    write_fatal ();
	if (ferror (ifp) || fclose (ifp) != 0)
	  read_fatal ();
      }
}