append_to_file (char const *from, char const *to)
{
  int tofd;

  if ((tofd = safe_open (to, O_WRONLY | O_BINARY | O_APPEND, 0)) < 0)
    pfatal ("Can't reopen file %s", quotearg (to));
  copy_to_fd (from, tofd);
  if (close (tofd) != 0)
    write_fatal ();
}