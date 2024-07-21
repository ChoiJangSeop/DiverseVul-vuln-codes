unix_read_all_fd_to_string (int      fd,
                            char    *buf,
                            ssize_t  max_bytes)
{
  ssize_t bytes_read;

  while (max_bytes > 1 && (bytes_read = read (fd, buf, MAX (4096, max_bytes - 1))))
    {
      if (bytes_read < 0)
        return FALSE;
      buf += bytes_read;
      max_bytes -= bytes_read;
    }
  *buf = '\0';
  return TRUE;
}