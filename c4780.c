directory_exists (dirname)
     const char *dirname;
{
  char *new_dirname;
  int dirlen, r;
  struct stat sb;

  /* First, dequote the directory name */
  new_dirname = bash_dequote_filename ((char *)dirname, rl_completion_quote_character);
  dirlen = STRLEN (new_dirname);
  if (new_dirname[dirlen - 1] == '/')
    new_dirname[dirlen - 1] = '\0';
#if defined (HAVE_LSTAT)
  r = lstat (new_dirname, &sb) == 0;
#else
  r = stat (new_dirname, &sb) == 0;
#endif
  free (new_dirname);
  return (r);
}