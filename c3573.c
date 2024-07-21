cleanup_environment()
{
if (!keep_environment || *keep_environment == '\0')
  clearenv();
else if (Ustrcmp(keep_environment, "*") != 0)
  {
  uschar **p;
  if (environ) for (p = USS environ; *p; /* see below */)
    {
    uschar *name = string_copyn(*p, US Ustrchr(*p, '=') - *p);

    if (OK != match_isinlist(name, CUSS &keep_environment,
        0, NULL, NULL, MCL_NOEXPAND, FALSE, NULL))
      if (unsetenv(CS name) < 0) return FALSE;
      else /* nothing */;
    else
      p++;

    store_reset(name);
    }
  }
if (add_environment)
  {
    uschar *p;
    int sep = 0;
    const uschar* envlist = add_environment;
    while ((p = string_nextinlist(&envlist, &sep, NULL, 0)))
        putenv(CS p);
  }

  return TRUE;
}