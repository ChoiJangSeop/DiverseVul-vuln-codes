ctcompare(const char *a,		/* I - First string */
          const char *b)		/* I - Second string */
{
  int	result = 0;			/* Result */


  while (*a && *b)
  {
    result |= *a ^ *b;
    a ++;
    b ++;
  }

  return (result);
}