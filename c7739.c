plan_b (char const *filename)
{
  int ifd;
  FILE *ifp;
  int c;
  size_t len;
  size_t maxlen;
  bool found_revision;
  size_t i;
  char const *rev;
  size_t revlen;
  lin line = 1;

  if (instat.st_size == 0)
    filename = NULL_DEVICE;
  if ((ifd = safe_open (filename, O_RDONLY | binary_transput, 0)) < 0
      || ! (ifp = fdopen (ifd, binary_transput ? "rb" : "r")))
    pfatal ("Can't open file %s", quotearg (filename));
  if (TMPINNAME_needs_removal)
    {
      /* Reopen the existing temporary file. */
      tifd = create_file (TMPINNAME, O_RDWR | O_BINARY, 0, true);
    }
  else
    {
      tifd = make_tempfile (&TMPINNAME, 'i', NULL, O_RDWR | O_BINARY,
			    S_IRUSR | S_IWUSR);
      if (tifd == -1)
	pfatal ("Can't create temporary file %s", TMPINNAME);
      TMPINNAME_needs_removal = true;
    }
  i = 0;
  len = 0;
  maxlen = 1;
  rev = revision;
  found_revision = !rev;
  revlen = rev ? strlen (rev) : 0;

  while ((c = getc (ifp)) != EOF)
    {
      if (++len > ((size_t) -1) / 2)
	lines_too_long (filename);

      if (c == '\n')
	{
	  if (++line < 0)
	    too_many_lines (filename);
	  if (maxlen < len)
	      maxlen = len;
	  len = 0;
	}

      if (!found_revision)
	{
	  if (i == revlen)
	    {
	      found_revision = ISSPACE ((unsigned char) c);
	      i = (size_t) -1;
	    }
	  else if (i != (size_t) -1)
	    i = rev[i]==c ? i + 1 : (size_t) -1;

	  if (i == (size_t) -1  &&  ISSPACE ((unsigned char) c))
	    i = 0;
	}
    }

  if (revision)
    report_revision (found_revision);
  Fseek (ifp, 0, SEEK_SET);		/* rewind file */
  for (tibufsize = TIBUFSIZE_MINIMUM;  tibufsize < maxlen;  tibufsize <<= 1)
    /* do nothing */ ;
  lines_per_buf = tibufsize / maxlen;
  tireclen = maxlen;
  tibuf[0] = xmalloc (2 * tibufsize);
  tibuf[1] = tibuf[0] + tibufsize;

  for (line = 1; ; line++)
    {
      char *p = tibuf[0] + maxlen * (line % lines_per_buf);
      char const *p0 = p;
      if (! (line % lines_per_buf))	/* new block */
	if (write (tifd, tibuf[0], tibufsize) != tibufsize)
	  write_fatal ();
      if ((c = getc (ifp)) == EOF)
	break;

      for (;;)
	{
	  *p++ = c;
	  if (c == '\n')
	    {
	      last_line_size = p - p0;
	      break;
	    }

	  if ((c = getc (ifp)) == EOF)
	    {
	      last_line_size = p - p0;
	      line++;
	      goto EOF_reached;
	    }
	}
    }
 EOF_reached:
  if (ferror (ifp)  ||  fclose (ifp) != 0)
    read_fatal ();

  if (line % lines_per_buf  !=  0)
    if (write (tifd, tibuf[0], tibufsize) != tibufsize)
      write_fatal ();
  input_lines = line - 1;
}