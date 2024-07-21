set_cs_start(char *line)
{
  char *p, *q, *r;

  if ((p = strstr(line, "string currentfile"))) {
    /* enforce presence of `readstring' -- 5/29/99 */
    if (!strstr(line, "readstring"))
      return;
    /* locate the name of the charstring start command */
    *p = '\0';					  /* damage line[] */
    q = strrchr(line, '/');
    if (q) {
      r = cs_start;
      ++q;
      while (!isspace(*q) && *q != '{')
	*r++ = *q++;
      *r = '\0';
    }
    *p = 's';					  /* repair line[] */
  }
}