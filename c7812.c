httpPrintf(http_t     *http,		/* I - HTTP connection */
           const char *format,		/* I - printf-style format string */
	   ...)				/* I - Additional args as needed */
{
  ssize_t	bytes;			/* Number of bytes to write */
  char		buf[16384];		/* Buffer for formatted string */
  va_list	ap;			/* Variable argument pointer */


  DEBUG_printf(("2httpPrintf(http=%p, format=\"%s\", ...)", (void *)http, format));

  va_start(ap, format);
  bytes = vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  DEBUG_printf(("3httpPrintf: (" CUPS_LLFMT " bytes) %s", CUPS_LLCAST bytes, buf));

  if (http->data_encoding == HTTP_ENCODING_FIELDS)
    return ((int)httpWrite2(http, buf, (size_t)bytes));
  else
  {
    if (http->wused)
    {
      DEBUG_puts("4httpPrintf: flushing existing data...");

      if (httpFlushWrite(http) < 0)
	return (-1);
    }

    return ((int)http_write(http, buf, (size_t)bytes));
  }
}