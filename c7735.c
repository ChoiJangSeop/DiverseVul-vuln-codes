copy_to_fd (const char *from, int tofd)
{
  int fromfd;
  ssize_t i;

  if ((fromfd = safe_open (from, O_RDONLY | O_BINARY, 0)) < 0)
    pfatal ("Can't reopen file %s", quotearg (from));
  while ((i = read (fromfd, buf, bufsize)) != 0)
    {
      if (i == (ssize_t) -1)
	read_fatal ();
      if (full_write (tofd, buf, i) != i)
	write_fatal ();
    }
  if (close (fromfd) != 0)
    read_fatal ();
}