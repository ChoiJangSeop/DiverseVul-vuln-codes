file_method(const char *s)	/* I - Filename or URL */
{
  if (strncmp(s, "http:", 5) == 0)
    return ("http");
  else if (strncmp(s, "https:", 6) == 0)
    return ("https");
  else if (strncmp(s, "ftp:", 4) == 0)
    return ("ftp");
  else if (strncmp(s, "mailto:", 7) == 0)
    return ("mailto");
  else
    return (NULL);
}