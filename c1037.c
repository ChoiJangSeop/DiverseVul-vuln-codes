dfaanalyze (struct dfa *d, int searchflag)
{
  int *nullable;		/* Nullable stack. */
  int *nfirstpos;		/* Element count stack for firstpos sets. */
  position *firstpos;		/* Array where firstpos elements are stored. */
  int *nlastpos;		/* Element count stack for lastpos sets. */
  position *lastpos;		/* Array where lastpos elements are stored. */
  position_set tmp;		/* Temporary set for merging sets. */
  position_set merged;		/* Result of merging sets. */
  int separate_contexts;	/* Context wanted by some position. */
  int *o_nullable;
  int *o_nfirst, *o_nlast;
  position *o_firstpos, *o_lastpos;
  int i, j;
  position *pos;

#ifdef DEBUG
  fprintf(stderr, "dfaanalyze:\n");
  for (i = 0; i < d->tindex; ++i)
    {
      fprintf(stderr, " %d:", i);
      prtok(d->tokens[i]);
    }
  putc('\n', stderr);
#endif

  d->searchflag = searchflag;

  MALLOC(nullable, d->depth);
  o_nullable = nullable;
  MALLOC(nfirstpos, d->depth);
  o_nfirst = nfirstpos;
  MALLOC(firstpos, d->nleaves);
  o_firstpos = firstpos, firstpos += d->nleaves;
  MALLOC(nlastpos, d->depth);
  o_nlast = nlastpos;
  MALLOC(lastpos, d->nleaves);
  o_lastpos = lastpos, lastpos += d->nleaves;
  alloc_position_set(&merged, d->nleaves);

  CALLOC(d->follows, d->tindex);

  for (i = 0; i < d->tindex; ++i)
    {
    switch (d->tokens[i])
      {
      case EMPTY:
        /* The empty set is nullable. */
        *nullable++ = 1;

        /* The firstpos and lastpos of the empty leaf are both empty. */
        *nfirstpos++ = *nlastpos++ = 0;
        break;

      case STAR:
      case PLUS:
        /* Every element in the firstpos of the argument is in the follow
           of every element in the lastpos. */
        tmp.nelem = nfirstpos[-1];
        tmp.elems = firstpos;
        pos = lastpos;
        for (j = 0; j < nlastpos[-1]; ++j)
          {
            merge(&tmp, &d->follows[pos[j].index], &merged);
            copy(&merged, &d->follows[pos[j].index]);
          }

      case QMARK:
        /* A QMARK or STAR node is automatically nullable. */
        if (d->tokens[i] != PLUS)
          nullable[-1] = 1;
        break;

      case CAT:
        /* Every element in the firstpos of the second argument is in the
           follow of every element in the lastpos of the first argument. */
        tmp.nelem = nfirstpos[-1];
        tmp.elems = firstpos;
        pos = lastpos + nlastpos[-1];
        for (j = 0; j < nlastpos[-2]; ++j)
          {
            merge(&tmp, &d->follows[pos[j].index], &merged);
            copy(&merged, &d->follows[pos[j].index]);
          }

        /* The firstpos of a CAT node is the firstpos of the first argument,
           union that of the second argument if the first is nullable. */
        if (nullable[-2])
          nfirstpos[-2] += nfirstpos[-1];
        else
          firstpos += nfirstpos[-1];
        --nfirstpos;

        /* The lastpos of a CAT node is the lastpos of the second argument,
           union that of the first argument if the second is nullable. */
        if (nullable[-1])
          nlastpos[-2] += nlastpos[-1];
        else
          {
            pos = lastpos + nlastpos[-2];
            for (j = nlastpos[-1] - 1; j >= 0; --j)
              pos[j] = lastpos[j];
            lastpos += nlastpos[-2];
            nlastpos[-2] = nlastpos[-1];
          }
        --nlastpos;

        /* A CAT node is nullable if both arguments are nullable. */
        nullable[-2] = nullable[-1] && nullable[-2];
        --nullable;
        break;

      case OR:
        /* The firstpos is the union of the firstpos of each argument. */
        nfirstpos[-2] += nfirstpos[-1];
        --nfirstpos;

        /* The lastpos is the union of the lastpos of each argument. */
        nlastpos[-2] += nlastpos[-1];
        --nlastpos;

        /* An OR node is nullable if either argument is nullable. */
        nullable[-2] = nullable[-1] || nullable[-2];
        --nullable;
        break;

      default:
        /* Anything else is a nonempty position.  (Note that special
           constructs like \< are treated as nonempty strings here;
           an "epsilon closure" effectively makes them nullable later.
           Backreferences have to get a real position so we can detect
           transitions on them later.  But they are nullable. */
        *nullable++ = d->tokens[i] == BACKREF;

        /* This position is in its own firstpos and lastpos. */
        *nfirstpos++ = *nlastpos++ = 1;
        --firstpos, --lastpos;
        firstpos->index = lastpos->index = i;
        firstpos->constraint = lastpos->constraint = NO_CONSTRAINT;

        /* Allocate the follow set for this position. */
        alloc_position_set(&d->follows[i], 1);
        break;
      }
#ifdef DEBUG
    /* ... balance the above nonsyntactic #ifdef goo... */
      fprintf(stderr, "node %d:", i);
      prtok(d->tokens[i]);
      putc('\n', stderr);
      fprintf(stderr, nullable[-1] ? " nullable: yes\n" : " nullable: no\n");
      fprintf(stderr, " firstpos:");
      for (j = nfirstpos[-1] - 1; j >= 0; --j)
        {
          fprintf(stderr, " %d:", firstpos[j].index);
          prtok(d->tokens[firstpos[j].index]);
        }
      fprintf(stderr, "\n lastpos:");
      for (j = nlastpos[-1] - 1; j >= 0; --j)
        {
          fprintf(stderr, " %d:", lastpos[j].index);
          prtok(d->tokens[lastpos[j].index]);
        }
      putc('\n', stderr);
#endif
    }

  /* For each follow set that is the follow set of a real position, replace
     it with its epsilon closure. */
  for (i = 0; i < d->tindex; ++i)
    if (d->tokens[i] < NOTCHAR || d->tokens[i] == BACKREF
#if MBS_SUPPORT
        || d->tokens[i] == ANYCHAR
        || d->tokens[i] == MBCSET
#endif
        || d->tokens[i] >= CSET)
      {
#ifdef DEBUG
        fprintf(stderr, "follows(%d:", i);
        prtok(d->tokens[i]);
        fprintf(stderr, "):");
        for (j = d->follows[i].nelem - 1; j >= 0; --j)
          {
            fprintf(stderr, " %d:", d->follows[i].elems[j].index);
            prtok(d->tokens[d->follows[i].elems[j].index]);
          }
        putc('\n', stderr);
#endif
        copy(&d->follows[i], &merged);
        epsclosure(&merged, d);
        copy(&merged, &d->follows[i]);
      }

  /* Get the epsilon closure of the firstpos of the regexp.  The result will
     be the set of positions of state 0. */
  merged.nelem = 0;
  for (i = 0; i < nfirstpos[-1]; ++i)
    insert(firstpos[i], &merged);
  epsclosure(&merged, d);

  /* Build the initial state. */
  d->salloc = 1;
  d->sindex = 0;
  MALLOC(d->states, d->salloc);

  separate_contexts = state_separate_contexts (&merged);
  state_index(d, &merged,
              (separate_contexts & CTX_NEWLINE
               ? CTX_NEWLINE
               : separate_contexts ^ CTX_ANY));

  free(o_nullable);
  free(o_nfirst);
  free(o_firstpos);
  free(o_nlast);
  free(o_lastpos);
  free(merged.elems);
}