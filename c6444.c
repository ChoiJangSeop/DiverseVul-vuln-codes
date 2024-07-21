void imap_quote_string (char *dest, size_t dlen, const char *src)
{
  static const char quote[] = "\"\\";
  char *pt;
  const char *s;

  pt = dest;
  s  = src;

  *pt++ = '"';
  /* save room for trailing quote-char */
  dlen -= 2;

  for (; *s && dlen; s++)
  {
    if (strchr (quote, *s))
    {
      dlen -= 2;
      if (!dlen)
	break;
      *pt++ = '\\';
      *pt++ = *s;
    }
    else
    {
      *pt++ = *s;
      dlen--;
    }
  }
  *pt++ = '"';
  *pt = 0;
}