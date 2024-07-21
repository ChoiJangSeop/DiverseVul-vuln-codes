enlist (char **cpp, char *new, size_t len)
{
  int i, j;

  if (cpp == NULL)
    return NULL;
  if ((new = icpyalloc(new)) == NULL)
    {
      freelist(cpp);
      return NULL;
    }
  new[len] = '\0';
  /* Is there already something in the list that's new (or longer)? */
  for (i = 0; cpp[i] != NULL; ++i)
    if (istrstr(cpp[i], new) != NULL)
      {
        free(new);
        return cpp;
      }
  /* Eliminate any obsoleted strings. */
  j = 0;
  while (cpp[j] != NULL)
    if (istrstr(new, cpp[j]) == NULL)
      ++j;
    else
      {
        free(cpp[j]);
        if (--i == j)
          break;
        cpp[j] = cpp[i];
        cpp[i] = NULL;
      }
  /* Add the new string. */
  REALLOC(cpp, i + 2);
  cpp[i] = new;
  cpp[i + 1] = NULL;
  return cpp;
}