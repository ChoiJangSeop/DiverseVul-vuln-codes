LZWReadByte (FILE *fd,
	     int   flag,
	     int   input_code_size)
{
  static int fresh = FALSE;
  int code, incode;
  static int code_size, set_code_size;
  static int max_code, max_code_size;
  static int firstcode, oldcode;
  static int clear_code, end_code;
  static int table[2][(1 << MAX_LZW_BITS)];
  static int stack[(1 << (MAX_LZW_BITS)) * 2], *sp;
  register int i;

  if (flag)
    {
      set_code_size = input_code_size;
      code_size = set_code_size + 1;
      clear_code = 1 << set_code_size;
      end_code = clear_code + 1;
      max_code_size = 2 * clear_code;
      max_code = clear_code + 2;

      GetCode (fd, 0, TRUE);

      fresh = TRUE;

      for (i = 0; i < clear_code; ++i)
	{
	  table[0][i] = 0;
	  table[1][i] = i;
	}
      for (; i < (1 << MAX_LZW_BITS); ++i)
	table[0][i] = table[1][0] = 0;

      sp = stack;

      return 0;
    }
  else if (fresh)
    {
      fresh = FALSE;
      do
	{
	  firstcode = oldcode =
	    GetCode (fd, code_size, FALSE);
	}
      while (firstcode == clear_code);
      return firstcode;
    }

  if (sp > stack)
    return *--sp;

  while ((code = GetCode (fd, code_size, FALSE)) >= 0)
    {
      if (code == clear_code)
	{
	  for (i = 0; i < clear_code; ++i)
	    {
	      table[0][i] = 0;
	      table[1][i] = i;
	    }
	  for (; i < (1 << MAX_LZW_BITS); ++i)
	    table[0][i] = table[1][i] = 0;
	  code_size = set_code_size + 1;
	  max_code_size = 2 * clear_code;
	  max_code = clear_code + 2;
	  sp = stack;
	  firstcode = oldcode =
	    GetCode (fd, code_size, FALSE);
	  return firstcode;
	}
      else if (code == end_code)
	{
	  int count;
	  unsigned char buf[260];

	  if (ZeroDataBlock)
	    return -2;

	  while ((count = GetDataBlock (fd, buf)) > 0)
	    ;

	  if (count != 0)
	    g_print ("GIF: missing EOD in data stream (common occurence)");
	  return -2;
	}

      incode = code;

      if (code >= max_code)
	{
	  *sp++ = firstcode;
	  code = oldcode;
	}

      while (code >= clear_code)
	{
	  *sp++ = table[1][code];
	  if (code == table[0][code])
	    {
	      g_message ("GIF: circular table entry BIG ERROR\n");
	      gimp_quit ();
	    }
	  code = table[0][code];
	}

      *sp++ = firstcode = table[1][code];

      if ((code = max_code) < (1 << MAX_LZW_BITS))
	{
	  table[0][code] = oldcode;
	  table[1][code] = firstcode;
	  ++max_code;
	  if ((max_code >= max_code_size) &&
	      (max_code_size < (1 << MAX_LZW_BITS)))
	    {
	      max_code_size *= 2;
	      ++code_size;
	    }
	}

      oldcode = incode;

      if (sp > stack)
	return *--sp;
    }
  return code;
}