string_extract_double_quoted (string, sindex, stripdq)
     char *string;
     int *sindex, stripdq;
{
  size_t slen;
  char *send;
  int j, i, t;
  unsigned char c;
  char *temp, *ret;		/* The new string we return. */
  int pass_next, backquote, si;	/* State variables for the machine. */
  int dquote;
  DECLARE_MBSTATE;

  slen = strlen (string + *sindex) + *sindex;
  send = string + slen;

  pass_next = backquote = dquote = 0;
  temp = (char *)xmalloc (1 + slen - *sindex);

  j = 0;
  i = *sindex;
  while (c = string[i])
    {
      /* Process a character that was quoted by a backslash. */
      if (pass_next)
	{
	  /* XXX - take another look at this in light of Interp 221 */
	  /* Posix.2 sez:

	     ``The backslash shall retain its special meaning as an escape
	     character only when followed by one of the characters:
		$	`	"	\	<newline>''.

	     If STRIPDQ is zero, we handle the double quotes here and let
	     expand_word_internal handle the rest.  If STRIPDQ is non-zero,
	     we have already been through one round of backslash stripping,
	     and want to strip these backslashes only if DQUOTE is non-zero,
	     indicating that we are inside an embedded double-quoted string. */

	     /* If we are in an embedded quoted string, then don't strip
		backslashes before characters for which the backslash
		retains its special meaning, but remove backslashes in
		front of other characters.  If we are not in an
		embedded quoted string, don't strip backslashes at all.
		This mess is necessary because the string was already
		surrounded by double quotes (and sh has some really weird
		quoting rules).
		The returned string will be run through expansion as if
		it were double-quoted. */
	  if ((stripdq == 0 && c != '"') ||
	      (stripdq && ((dquote && (sh_syntaxtab[c] & CBSDQUOTE)) || dquote == 0)))
	    temp[j++] = '\\';
	  pass_next = 0;

add_one_character:
	  COPY_CHAR_I (temp, j, string, send, i);
	  continue;
	}

      /* A backslash protects the next character.  The code just above
	 handles preserving the backslash in front of any character but
	 a double quote. */
      if (c == '\\')
	{
	  pass_next++;
	  i++;
	  continue;
	}

      /* Inside backquotes, ``the portion of the quoted string from the
	 initial backquote and the characters up to the next backquote
	 that is not preceded by a backslash, having escape characters
	 removed, defines that command''. */
      if (backquote)
	{
	  if (c == '`')
	    backquote = 0;
	  temp[j++] = c;
	  i++;
	  continue;
	}

      if (c == '`')
	{
	  temp[j++] = c;
	  backquote++;
	  i++;
	  continue;
	}

      /* Pass everything between `$(' and the matching `)' or a quoted
	 ${ ... } pair through according to the Posix.2 specification. */
      if (c == '$' && ((string[i + 1] == LPAREN) || (string[i + 1] == LBRACE)))
	{
	  int free_ret = 1;

	  si = i + 2;
	  if (string[i + 1] == LPAREN)
	    ret = extract_command_subst (string, &si, 0);
	  else
	    ret = extract_dollar_brace_string (string, &si, Q_DOUBLE_QUOTES, 0);

	  temp[j++] = '$';
	  temp[j++] = string[i + 1];

	  /* Just paranoia; ret will not be 0 unless no_longjmp_on_fatal_error
	     is set. */
	  if (ret == 0 && no_longjmp_on_fatal_error)
	    {
	      free_ret = 0;
	      ret = string + i + 2;
	    }

	  for (t = 0; ret[t]; t++, j++)
	    temp[j] = ret[t];
	  temp[j] = string[si];

	  if (string[si])
	    {
	      j++;
	      i = si + 1;
	    }
	  else
	    i = si;

	  if (free_ret)
	    free (ret);
	  continue;
	}

      /* Add any character but a double quote to the quoted string we're
	 accumulating. */
      if (c != '"')
	goto add_one_character;

      /* c == '"' */
      if (stripdq)
	{
	  dquote ^= 1;
	  i++;
	  continue;
	}

      break;
    }
  temp[j] = '\0';

  /* Point to after the closing quote. */
  if (c)
    i++;
  *sindex = i;

  return (temp);
}