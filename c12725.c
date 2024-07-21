completion_glob_pattern (string)
     char *string;
{
  register int c;
  char *send;
  int open;

  DECLARE_MBSTATE;

  open = 0;
  send = string + strlen (string);

  while (c = *string++)
    {
      switch (c)
	{
	case '?':
	case '*':
	  return (1);

	case '[':
	  open++;
	  continue;

	case ']':
	  if (open)
	    return (1);
	  continue;

	case '+':
	case '@':
	case '!':
	  if (*string == '(')	/*)*/
	    return (1);
	  continue;

	case '\\':
	  if (*string++ == 0)
	    return (0);
	}

      /* Advance one fewer byte than an entire multibyte character to
	 account for the auto-increment in the loop above. */
#ifdef HANDLE_MULTIBYTE
      string--;
      ADVANCE_CHAR_P (string, send - string);
      string++;
#else
      ADVANCE_CHAR_P (string, send - string);
#endif
    }
  return (0);
}