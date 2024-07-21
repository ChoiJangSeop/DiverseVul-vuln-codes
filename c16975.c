sh_backslash_quote (string, table)
     char *string;
     char *table;
{
  int c;
  char *result, *r, *s, *backslash_table;

  result = (char *)xmalloc (2 * strlen (string) + 1);

  backslash_table = table ? table : bstab;
  for (r = result, s = string; s && (c = *s); s++)
    {
      
      if (backslash_table[c] == 1)
	*r++ = '\\';
      else if (c == '#' && s == string)			/* comment char */
	*r++ = '\\';
      *r++ = c;
    }

  *r = '\0';
  return (result);
}