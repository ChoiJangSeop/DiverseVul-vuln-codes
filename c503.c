string_vformat(uschar *buffer, int buflen, char *format, va_list ap)
{
enum { L_NORMAL, L_SHORT, L_LONG, L_LONGLONG, L_LONGDOUBLE };

BOOL yield = TRUE;
int width, precision;
char *fp = format;             /* Deliberately not unsigned */
uschar *p = buffer;
uschar *last = buffer + buflen - 1;

string_datestamp_offset = -1;  /* Datestamp not inserted */

/* Scan the format and handle the insertions */

while (*fp != 0)
  {
  int length = L_NORMAL;
  int *nptr;
  int slen;
  char *null = "NULL";         /* ) These variables */
  char *item_start, *s;        /* ) are deliberately */
  char newformat[16];          /* ) not unsigned */

  /* Non-% characters just get copied verbatim */

  if (*fp != '%')
    {
    if (p >= last) { yield = FALSE; break; }
    *p++ = (uschar)*fp++;
    continue;
    }

  /* Deal with % characters. Pick off the width and precision, for checking
  strings, skipping over the flag and modifier characters. */

  item_start = fp;
  width = precision = -1;

  if (strchr("-+ #0", *(++fp)) != NULL)
    {
    if (*fp == '#') null = "";
    fp++;
    }

  if (isdigit((uschar)*fp))
    {
    width = *fp++ - '0';
    while (isdigit((uschar)*fp)) width = width * 10 + *fp++ - '0';
    }
  else if (*fp == '*')
    {
    width = va_arg(ap, int);
    fp++;
    }

  if (*fp == '.')
    {
    if (*(++fp) == '*')
      {
      precision = va_arg(ap, int);
      fp++;
      }
    else
      {
      precision = 0;
      while (isdigit((uschar)*fp))
        precision = precision*10 + *fp++ - '0';
      }
    }

  /* Skip over 'h', 'L', 'l', and 'll', remembering the item length */

  if (*fp == 'h')
    { fp++; length = L_SHORT; }
  else if (*fp == 'L')
    { fp++; length = L_LONGDOUBLE; }
  else if (*fp == 'l')
    {
    if (fp[1] == 'l')
      {
      fp += 2;
      length = L_LONGLONG;
      }
    else
      {
      fp++;
      length = L_LONG;
      }
    }

  /* Handle each specific format type. */

  switch (*fp++)
    {
    case 'n':
    nptr = va_arg(ap, int *);
    *nptr = p - buffer;
    break;

    case 'd':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
    if (p >= last - ((length > L_LONG)? 24 : 12))
      { yield = FALSE; goto END_FORMAT; }
    strncpy(newformat, item_start, fp - item_start);
    newformat[fp - item_start] = 0;

    /* Short int is promoted to int when passing through ..., so we must use
    int for va_arg(). */

    switch(length)
      {
      case L_SHORT:
      case L_NORMAL:   sprintf(CS p, newformat, va_arg(ap, int)); break;
      case L_LONG:     sprintf(CS p, newformat, va_arg(ap, long int)); break;
      case L_LONGLONG: sprintf(CS p, newformat, va_arg(ap, LONGLONG_T)); break;
      }
    while (*p) p++;
    break;

    case 'p':
    if (p >= last - 24) { yield = FALSE; goto END_FORMAT; }
    strncpy(newformat, item_start, fp - item_start);
    newformat[fp - item_start] = 0;
    sprintf(CS p, newformat, va_arg(ap, void *));
    while (*p) p++;
    break;

    /* %f format is inherently insecure if the numbers that it may be
    handed are unknown (e.g. 1e300). However, in Exim, %f is used for
    printing load averages, and these are actually stored as integers
    (load average * 1000) so the size of the numbers is constrained.
    It is also used for formatting sending rates, where the simplicity
    of the format prevents overflow. */

    case 'f':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
    if (precision < 0) precision = 6;
    if (p >= last - precision - 8) { yield = FALSE; goto END_FORMAT; }
    strncpy(newformat, item_start, fp - item_start);
    newformat[fp-item_start] = 0;
    if (length == L_LONGDOUBLE)
      sprintf(CS p, newformat, va_arg(ap, long double));
    else
      sprintf(CS p, newformat, va_arg(ap, double));
    while (*p) p++;
    break;

    /* String types */

    case '%':
    if (p >= last) { yield = FALSE; goto END_FORMAT; }
    *p++ = '%';
    break;

    case 'c':
    if (p >= last) { yield = FALSE; goto END_FORMAT; }
    *p++ = va_arg(ap, int);
    break;

    case 'D':                   /* Insert datestamp for log file names */
    s = CS tod_stamp(tod_log_datestamp);
    string_datestamp_offset = p - buffer;   /* Passed back via global */
    goto INSERT_STRING;

    case 's':
    case 'S':                   /* Forces *lower* case */
    s = va_arg(ap, char *);

    INSERT_STRING:              /* Come to from %D above */
    if (s == NULL) s = null;
    slen = Ustrlen(s);

    /* If the width is specified, check that there is a precision
    set; if not, set it to the width to prevent overruns of long
    strings. */

    if (width >= 0)
      {
      if (precision < 0) precision = width;
      }

    /* If a width is not specified and the precision is specified, set
    the width to the precision, or the string length if shorted. */

    else if (precision >= 0)
      {
      width = (precision < slen)? precision : slen;
      }

    /* If neither are specified, set them both to the string length. */

    else width = precision = slen;

    /* Check string space, and add the string to the buffer if ok. If
    not OK, add part of the string (debugging uses this to show as
    much as possible). */

    if (p >= last - width)
      {
      yield = FALSE;
      width = precision = last - p - 1;
      }
    sprintf(CS p, "%*.*s", width, precision, s);
    if (fp[-1] == 'S')
      while (*p) { *p = tolower(*p); p++; }
    else
      while (*p) p++;
    if (!yield) goto END_FORMAT;
    break;

    /* Some things are never used in Exim; also catches junk. */

    default:
    strncpy(newformat, item_start, fp - item_start);
    newformat[fp-item_start] = 0;
    log_write(0, LOG_MAIN|LOG_PANIC_DIE, "string_format: unsupported type "
      "in \"%s\" in \"%s\"", newformat, format);
    break;
    }
  }

/* Ensure string is complete; return TRUE if got to the end of the format */

END_FORMAT:

*p = 0;
return yield;
}