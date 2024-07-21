plan_a (char const *filename)
{
  char const *s;
  char const *lim;
  char const **ptr;
  char *buffer;
  lin iline;
  size_t size = instat.st_size;

  /* Fail if the file size doesn't fit in a size_t,
     or if storage isn't available.  */
  if (! (size == instat.st_size
	 && (buffer = malloc (size ? size : (size_t) 1))))
    return false;

  /* Read the input file, but don't bother reading it if it's empty.
     When creating files, the files do not actually exist.  */
  if (size)
    {
      if (S_ISREG (instat.st_mode))
        {
	  int ifd = safe_open (filename, O_RDONLY|binary_transput, 0);
	  size_t buffered = 0, n;
	  if (ifd < 0)
	    pfatal ("can't open file %s", quotearg (filename));

	  while (size - buffered != 0)
	    {
	      n = read (ifd, buffer + buffered, size - buffered);
	      if (n == 0)
		{
		  /* Some non-POSIX hosts exaggerate st_size in text mode;
		     or the file may have shrunk!  */
		  size = buffered;
		  break;
		}
	      if (n == (size_t) -1)
		{
		  /* Perhaps size is too large for this host.  */
		  close (ifd);
		  free (buffer);
		  return false;
		}
	      buffered += n;
	    }

	  if (close (ifd) != 0)
	    read_fatal ();
	}
      else if (S_ISLNK (instat.st_mode))
	{
	  ssize_t n;
	  n = safe_readlink (filename, buffer, size);
	  if (n < 0)
	    pfatal ("can't read %s %s", "symbolic link", quotearg (filename));
	  size = n;
	}
      else
	{
	  free (buffer);
	  return false;
	}
  }

  /* Scan the buffer and build array of pointers to lines.  */
  lim = buffer + size;
  iline = 3; /* 1 unused, 1 for SOF, 1 for EOF if last line is incomplete */
  for (s = buffer;  (s = (char *) memchr (s, '\n', lim - s));  s++)
    if (++iline < 0)
      too_many_lines (filename);
  if (! (iline == (size_t) iline
	 && (size_t) iline * sizeof *ptr / sizeof *ptr == (size_t) iline
	 && (ptr = (char const **) malloc ((size_t) iline * sizeof *ptr))))
    {
      free (buffer);
      return false;
    }
  iline = 0;
  for (s = buffer;  ;  s++)
    {
      ptr[++iline] = s;
      if (! (s = (char *) memchr (s, '\n', lim - s)))
	break;
    }
  if (size && lim[-1] != '\n')
    ptr[++iline] = lim;
  input_lines = iline - 1;

  if (revision)
    {
      char const *rev = revision;
      int rev0 = rev[0];
      bool found_revision = false;
      size_t revlen = strlen (rev);

      if (revlen <= size)
	{
	  char const *limrev = lim - revlen;

	  for (s = buffer;  (s = (char *) memchr (s, rev0, limrev - s));  s++)
	    if (memcmp (s, rev, revlen) == 0
		&& (s == buffer || ISSPACE ((unsigned char) s[-1]))
		&& (s + 1 == limrev || ISSPACE ((unsigned char) s[revlen])))
	      {
		found_revision = true;
		break;
	      }
	}

      report_revision (found_revision);
    }

  /* Plan A will work.  */
  i_buffer = buffer;
  i_ptr = ptr;
  return true;
}