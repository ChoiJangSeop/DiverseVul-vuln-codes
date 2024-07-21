epsclosure (position_set *s, struct dfa const *d)
{
  int i, j;
  char *visited;	/* array of booleans, enough to use char, not int */
  position p, old;

  CALLOC(visited, d->tindex);

  for (i = 0; i < s->nelem; ++i)
    if (d->tokens[s->elems[i].index] >= NOTCHAR
        && d->tokens[s->elems[i].index] != BACKREF
#if MBS_SUPPORT
        && d->tokens[s->elems[i].index] != ANYCHAR
        && d->tokens[s->elems[i].index] != MBCSET
#endif
        && d->tokens[s->elems[i].index] < CSET)
      {
        old = s->elems[i];
        p.constraint = old.constraint;
        delete(s->elems[i], s);
        if (visited[old.index])
          {
            --i;
            continue;
          }
        visited[old.index] = 1;
        switch (d->tokens[old.index])
          {
          case BEGLINE:
            p.constraint &= BEGLINE_CONSTRAINT;
            break;
          case ENDLINE:
            p.constraint &= ENDLINE_CONSTRAINT;
            break;
          case BEGWORD:
            p.constraint &= BEGWORD_CONSTRAINT;
            break;
          case ENDWORD:
            p.constraint &= ENDWORD_CONSTRAINT;
            break;
          case LIMWORD:
            p.constraint &= LIMWORD_CONSTRAINT;
            break;
          case NOTLIMWORD:
            p.constraint &= NOTLIMWORD_CONSTRAINT;
            break;
          default:
            break;
          }
        for (j = 0; j < d->follows[old.index].nelem; ++j)
          {
            p.index = d->follows[old.index].elems[j].index;
            insert(p, s);
          }
        /* Force rescan to start at the beginning. */
        i = -1;
      }

  free(visited);
}