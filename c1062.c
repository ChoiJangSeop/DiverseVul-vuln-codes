prepend_args (char const *options, char *buf, char **argv)
{
  char const *o = options;
  char *b = buf;
  int n = 0;

  for (;;)
    {
      while (c_isspace ((unsigned char) *o))
        o++;
      if (!*o)
        return n;
      if (argv)
        argv[n] = b;
      n++;

      do
        if ((*b++ = *o++) == '\\' && *o)
          b[-1] = *o++;
      while (*o && ! c_isspace ((unsigned char) *o));

      *b++ = '\0';
    }
}