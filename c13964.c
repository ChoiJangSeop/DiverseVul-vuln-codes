static bool safecmp(char *a, char *b)
{
  if(a && b)
    return !strcmp(a, b);
  else if(!a && !b)
    return TRUE; /* match */
  return FALSE; /* no match */
}