transit_state_singlebyte (struct dfa *d, int s, unsigned char const *p,
                                  int *next_state)
{
  int *t;
  int works = s;

  status_transit_state rval = TRANSIT_STATE_IN_PROGRESS;

  while (rval == TRANSIT_STATE_IN_PROGRESS)
    {
      if ((t = d->trans[works]) != NULL)
        {
          works = t[*p];
          rval = TRANSIT_STATE_DONE;
          if (works < 0)
            works = 0;
        }
      else if (works < 0)
        {
          if (p == buf_end)
            {
              /* At the moment, it must not happen.  */
              abort ();
            }
          works = 0;
        }
      else if (d->fails[works])
        {
          works = d->fails[works][*p];
          rval = TRANSIT_STATE_DONE;
        }
      else
        {
          build_state(works, d);
        }
    }
  *next_state = works;
  return rval;
}