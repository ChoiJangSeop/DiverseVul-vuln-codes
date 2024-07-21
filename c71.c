m4_maketemp (struct obstack *obs, int argc, token_data **argv)
{
  if (bad_argc (argv[0], argc, 2, 2))
    return;
  if (no_gnu_extensions)
    {
      /* POSIX states "any trailing 'X' characters [are] replaced with
	 the current process ID as a string", without referencing the
	 file system.  Horribly insecure, but we have to do it when we
	 are in traditional mode.

	 For reference, Solaris m4 does:
	   maketemp() -> `'
	   maketemp(X) -> `X'
	   maketemp(XX) -> `Xn', where n is last digit of pid
	   maketemp(XXXXXXXX) -> `X00nnnnn', where nnnnn is 16-bit pid
      */
      const char *str = ARG (1);
      int len = strlen (str);
      int i;
      int len2;

      M4ERROR ((warning_status, 0, "recommend using mkstemp instead"));
      for (i = len; i > 1; i--)
	if (str[i - 1] != 'X')
	  break;
      obstack_grow (obs, str, i);
      str = ntoa ((int32_t) getpid (), 10);
      len2 = strlen (str);
      if (len2 > len - i)
	obstack_grow0 (obs, str + len2 - (len - i), len - i);
      else
	{
	  while (i++ < len - len2)
	    obstack_1grow (obs, '0');
	  obstack_grow0 (obs, str, len2);
	}
    }
  else
    mkstemp_helper (obs, ARG (1));
}