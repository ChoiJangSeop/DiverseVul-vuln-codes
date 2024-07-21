int main(int argc, char *argv[])
{
  char *p, *q, *r;

  Clp_Parser *clp =
    Clp_NewParser(argc, (const char * const *)argv, sizeof(options) / sizeof(options[0]), options);
  program_name = Clp_ProgramName(clp);

  /* interpret command line arguments using CLP */
  while (1) {
    int opt = Clp_Next(clp);
    switch (opt) {

     case BLOCK_LEN_OPT:
      blocklen = clp->val.i;
      break;

     output_file:
     case OUTPUT_OPT:
      if (ofp)
	fatal_error("output file already specified");
      if (strcmp(clp->vstr, "-") == 0)
	ofp = stdout;
      else if (!(ofp = fopen(clp->vstr, "w")))
	fatal_error("%s: %s", clp->vstr, strerror(errno));
      break;

     case PFB_OPT:
      pfb = 1;
      break;

     case PFA_OPT:
      pfb = 0;
      break;

     case HELP_OPT:
      usage();
      exit(0);
      break;

     case VERSION_OPT:
      printf("t1asm (LCDF t1utils) %s\n", VERSION);
      printf("Copyright (C) 1992-2010 I. Lee Hetherington, Eddie Kohler et al.\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
      exit(0);
      break;

     case Clp_NotOption:
      if (ifp && ofp)
	fatal_error("too many arguments");
      else if (ifp)
	goto output_file;
      if (strcmp(clp->vstr, "-") == 0)
	ifp = stdin;
      else if (!(ifp = fopen(clp->vstr, "r")))
	fatal_error("%s: %s", clp->vstr, strerror(errno));
      break;

     case Clp_Done:
      goto done;

     case Clp_BadOption:
      short_usage();
      exit(1);
      break;

    }
  }

 done:
  if (!pfb) {
    if (blocklen == -1)
      blocklen = 64;
    else if (blocklen < 8) {
      blocklen = 8;
      error("warning: line length raised to %d", blocklen);
    } else if (blocklen > 1024) {
      blocklen = 1024;
      error("warning: line length lowered to %d", blocklen);
    }
  }

  if (!ifp) ifp = stdin;
  if (!ofp) ofp = stdout;

  if (pfb)
    init_pfb_writer(&w, blocklen, ofp);

#if defined(_MSDOS) || defined(_WIN32)
  /* If we are processing a PFB (binary) output */
  /* file, we must set its file mode to binary. */
  if (pfb)
    _setmode(_fileno(ofp), _O_BINARY);
#endif

  /* Finally, we loop until no more input. Some special things to look for are
     the `currentfile eexec' line, the beginning of the `/Subrs' or
     `/CharStrings' definition, the definition of `/lenIV', and the definition
     of the charstring start command which has `...string currentfile...' in
     it.

     Being careful: Check with `/Subrs' and `/CharStrings' to see that a
     number follows the token -- otherwise, the token is probably nested in a
     subroutine a la Adobe Jenson, and we shouldn't pay attention to it.

     Bugs: Occurrence of `/Subrs 9' in a comment will fool t1asm.

     Thanks to Tom Kacvinsky <tjk@ams.org> who reported that some fonts come
     without /Subrs sections and provided a patch. */

  while (!feof(ifp) && !ferror(ifp)) {
    t1utils_getline();

    if (!ever_active) {
      if (strncmp(line, "currentfile eexec", 17) == 0 && isspace(line[17])) {
	/* Allow arbitrary whitespace after "currentfile eexec".
	   Thanks to Tom Kacvinsky <tjk@ams.org> for reporting this.
	   Note: strlen("currentfile eexec") == 17. */
	for (p = line + 18; isspace(*p); p++)
	  ;
	eexec_start(p);
	continue;
      } else if (strncmp(line, "/lenIV", 6) == 0) {
	lenIV = atoi(line + 6);
      } else if ((p = strstr(line, "string currentfile"))
		 && strstr(line, "readstring")) { /* enforce `readstring' */
	/* locate the name of the charstring start command */
	*p = '\0';                                  /* damage line[] */
	q = strrchr(line, '/');
	if (q) {
	  r = cs_start;
	  ++q;
	  while (!isspace(*q) && *q != '{')
	    *r++ = *q++;
	  *r = '\0';
	}
	*p = 's';                                   /* repair line[] */
      }
    }

    if (!active) {
      if ((p = strstr(line, "/Subrs")) && isdigit(p[7]))
	ever_active = active = 1;
      else if ((p = strstr(line, "/CharStrings")) && isdigit(p[13]))
	ever_active = active = 1;
    }
    if ((p = strstr(line, "currentfile closefile"))) {
      /* 2/14/99 -- happy Valentine's day! -- don't look for `mark
	 currentfile closefile'; the `mark' might be on a different line */
      /* 1/3/2002 -- happy new year! -- Luc Devroye reports a failure with
         some printers when `currentfile closefile' is followed by space */
      p += sizeof("currentfile closefile") - 1;
      for (q = p; isspace(*q) && *q != '\n'; q++)
	/* nada */;
      if (q == p && !*q)
	error("warning: `currentfile closefile' line too long");
      else if (q != p) {
	if (*q != '\n')
	  error("text after `currentfile closefile' ignored");
	*p++ = '\n';
	*p++ = '\0';
      }
      eexec_string(line);
      break;
    }

    eexec_string(line);

    /* output line data */
    if (start_charstring) {
      if (!cs_start[0])
	fatal_error("couldn't find charstring start command");
      parse_charstring();
    }
  }

  /* Handle remaining PostScript after the eexec section */
  if (in_eexec)
    eexec_end();

  /* There may be additional code. */
  while (!feof(ifp) && !ferror(ifp)) {
    t1utils_getline();
    eexec_string(line);
  }

  if (pfb)
    pfb_writer_end(&w);

  /* the end! */
  if (!ever_active)
    error("warning: no charstrings found in input file");
  fclose(ifp);
  fclose(ofp);
  return 0;
}