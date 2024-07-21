inboth (char **left, char **right)
{
  char **both;
  char **temp;
  int lnum, rnum;

  if (left == NULL || right == NULL)
    return NULL;
  both = malloc(sizeof *both);
  if (both == NULL)
    return NULL;
  both[0] = NULL;
  for (lnum = 0; left[lnum] != NULL; ++lnum)
    {
      for (rnum = 0; right[rnum] != NULL; ++rnum)
        {
          temp = comsubs(left[lnum], right[rnum]);
          if (temp == NULL)
            {
              freelist(both);
              return NULL;
            }
          both = addlists(both, temp);
          freelist(temp);
          free(temp);
          if (both == NULL)
            return NULL;
        }
    }
  return both;
}