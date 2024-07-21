dfastate (int s, struct dfa *d, int trans[])
{
  leaf_set *grps;		/* As many as will ever be needed. */
  charclass *labels;		/* Labels corresponding to the groups. */
  int ngrps = 0;		/* Number of groups actually used. */
  position pos;			/* Current position being considered. */
  charclass matches;		/* Set of matching characters. */
  int matchesf;			/* True if matches is nonempty. */
  charclass intersect;		/* Intersection with some label set. */
  int intersectf;		/* True if intersect is nonempty. */
  charclass leftovers;		/* Stuff in the label that didn't match. */
  int leftoversf;		/* True if leftovers is nonempty. */
  position_set follows;		/* Union of the follows of some group. */
  position_set tmp;		/* Temporary space for merging sets. */
  int possible_contexts;	/* Contexts that this group can match. */
  int separate_contexts;	/* Context that new state wants to know. */
  int state;			/* New state. */
  int state_newline;		/* New state on a newline transition. */
  int state_letter;		/* New state on a letter transition. */
  int next_isnt_1st_byte = 0;	/* Flag if we can't add state0.  */
  int i, j, k;

  MALLOC (grps, NOTCHAR);
  MALLOC (labels, NOTCHAR);

  zeroset(matches);

  for (i = 0; i < d->states[s].elems.nelem; ++i)
    {
      pos = d->states[s].elems.elems[i];
      if (d->tokens[pos.index] >= 0 && d->tokens[pos.index] < NOTCHAR)
        setbit(d->tokens[pos.index], matches);
      else if (d->tokens[pos.index] >= CSET)
        copyset(d->charclasses[d->tokens[pos.index] - CSET], matches);
      else if (MBS_SUPPORT
               && (d->tokens[pos.index] == ANYCHAR
                   || d->tokens[pos.index] == MBCSET))
        /* MB_CUR_MAX > 1  */
        {
          /* ANYCHAR and MBCSET must match with a single character, so we
             must put it to d->states[s].mbps, which contains the positions
             which can match with a single character not a byte.  */
          if (d->states[s].mbps.nelem == 0)
            alloc_position_set(&d->states[s].mbps, 1);
          insert(pos, &(d->states[s].mbps));
          continue;
        }
      else
        continue;

      /* Some characters may need to be eliminated from matches because
         they fail in the current context. */
      if (pos.constraint != NO_CONSTRAINT)
        {
          if (! SUCCEEDS_IN_CONTEXT(pos.constraint,
                                    d->states[s].context, CTX_NEWLINE))
            for (j = 0; j < CHARCLASS_INTS; ++j)
              matches[j] &= ~newline[j];
          if (! SUCCEEDS_IN_CONTEXT(pos.constraint,
                                    d->states[s].context, CTX_LETTER))
            for (j = 0; j < CHARCLASS_INTS; ++j)
              matches[j] &= ~letters[j];
          if (! SUCCEEDS_IN_CONTEXT(pos.constraint,
                                    d->states[s].context, CTX_NONE))
            for (j = 0; j < CHARCLASS_INTS; ++j)
              matches[j] &= letters[j] | newline[j];

          /* If there are no characters left, there's no point in going on. */
          for (j = 0; j < CHARCLASS_INTS && !matches[j]; ++j)
            continue;
          if (j == CHARCLASS_INTS)
            continue;
        }

      for (j = 0; j < ngrps; ++j)
        {
          /* If matches contains a single character only, and the current
             group's label doesn't contain that character, go on to the
             next group. */
          if (d->tokens[pos.index] >= 0 && d->tokens[pos.index] < NOTCHAR
              && !tstbit(d->tokens[pos.index], labels[j]))
            continue;

          /* Check if this group's label has a nonempty intersection with
             matches. */
          intersectf = 0;
          for (k = 0; k < CHARCLASS_INTS; ++k)
            (intersect[k] = matches[k] & labels[j][k]) ? (intersectf = 1) : 0;
          if (! intersectf)
            continue;

          /* It does; now find the set differences both ways. */
          leftoversf = matchesf = 0;
          for (k = 0; k < CHARCLASS_INTS; ++k)
            {
              /* Even an optimizing compiler can't know this for sure. */
              int match = matches[k], label = labels[j][k];

              (leftovers[k] = ~match & label) ? (leftoversf = 1) : 0;
              (matches[k] = match & ~label) ? (matchesf = 1) : 0;
            }

          /* If there were leftovers, create a new group labeled with them. */
          if (leftoversf)
            {
              copyset(leftovers, labels[ngrps]);
              copyset(intersect, labels[j]);
              MALLOC(grps[ngrps].elems, d->nleaves);
              memcpy(grps[ngrps].elems, grps[j].elems,
                     sizeof (grps[j].elems[0]) * grps[j].nelem);
              grps[ngrps].nelem = grps[j].nelem;
              ++ngrps;
            }

          /* Put the position in the current group.  The constraint is
             irrelevant here.  */
          grps[j].elems[grps[j].nelem++] = pos.index;

          /* If every character matching the current position has been
             accounted for, we're done. */
          if (! matchesf)
            break;
        }

      /* If we've passed the last group, and there are still characters
         unaccounted for, then we'll have to create a new group. */
      if (j == ngrps)
        {
          copyset(matches, labels[ngrps]);
          zeroset(matches);
          MALLOC(grps[ngrps].elems, d->nleaves);
          grps[ngrps].nelem = 1;
          grps[ngrps].elems[0] = pos.index;
          ++ngrps;
        }
    }

  alloc_position_set(&follows, d->nleaves);
  alloc_position_set(&tmp, d->nleaves);

  /* If we are a searching matcher, the default transition is to a state
     containing the positions of state 0, otherwise the default transition
     is to fail miserably. */
  if (d->searchflag)
    {
      /* Find the state(s) corresponding to the positions of state 0. */
      copy(&d->states[0].elems, &follows);
      separate_contexts = state_separate_contexts (&follows);
      state = state_index (d, &follows, separate_contexts ^ CTX_ANY);
      if (separate_contexts & CTX_NEWLINE)
        state_newline = state_index(d, &follows, CTX_NEWLINE);
      else
        state_newline = state;
      if (separate_contexts & CTX_LETTER)
        state_letter = state_index(d, &follows, CTX_LETTER);
      else
        state_letter = state;

      for (i = 0; i < NOTCHAR; ++i)
        trans[i] = (IS_WORD_CONSTITUENT(i)) ? state_letter : state;
      trans[eolbyte] = state_newline;
    }
  else
    for (i = 0; i < NOTCHAR; ++i)
      trans[i] = -1;

  for (i = 0; i < ngrps; ++i)
    {
      follows.nelem = 0;

      /* Find the union of the follows of the positions of the group.
         This is a hideously inefficient loop.  Fix it someday. */
      for (j = 0; j < grps[i].nelem; ++j)
        for (k = 0; k < d->follows[grps[i].elems[j]].nelem; ++k)
          insert(d->follows[grps[i].elems[j]].elems[k], &follows);

      if (d->mb_cur_max > 1)
        {
          /* If a token in follows.elems is not 1st byte of a multibyte
             character, or the states of follows must accept the bytes
             which are not 1st byte of the multibyte character.
             Then, if a state of follows encounter a byte, it must not be
             a 1st byte of a multibyte character nor single byte character.
             We cansel to add state[0].follows to next state, because
             state[0] must accept 1st-byte

             For example, we assume <sb a> is a certain single byte
             character, <mb A> is a certain multibyte character, and the
             codepoint of <sb a> equals the 2nd byte of the codepoint of
             <mb A>.
             When state[0] accepts <sb a>, state[i] transit to state[i+1]
             by accepting accepts 1st byte of <mb A>, and state[i+1]
             accepts 2nd byte of <mb A>, if state[i+1] encounter the
             codepoint of <sb a>, it must not be <sb a> but 2nd byte of
             <mb A>, so we cannot add state[0].  */

          next_isnt_1st_byte = 0;
          for (j = 0; j < follows.nelem; ++j)
            {
              if (!(d->multibyte_prop[follows.elems[j].index] & 1))
                {
                  next_isnt_1st_byte = 1;
                  break;
                }
            }
        }

      /* If we are building a searching matcher, throw in the positions
         of state 0 as well. */
      if (d->searchflag
          && (! MBS_SUPPORT
              || (d->mb_cur_max == 1 || !next_isnt_1st_byte)))
        for (j = 0; j < d->states[0].elems.nelem; ++j)
          insert(d->states[0].elems.elems[j], &follows);

      /* Find out if the new state will want any context information. */
      possible_contexts = charclass_context(labels[i]);
      separate_contexts = state_separate_contexts (&follows);

      /* Find the state(s) corresponding to the union of the follows. */
      if ((separate_contexts & possible_contexts) != possible_contexts)
        state = state_index (d, &follows, separate_contexts ^ CTX_ANY);
      else
        state = -1;
      if (separate_contexts & possible_contexts & CTX_NEWLINE)
        state_newline = state_index (d, &follows, CTX_NEWLINE);
      else
        state_newline = state;
      if (separate_contexts & possible_contexts & CTX_LETTER)
        state_letter = state_index(d, &follows, CTX_LETTER);
      else
        state_letter = state;

      /* Set the transitions for each character in the current label. */
      for (j = 0; j < CHARCLASS_INTS; ++j)
        for (k = 0; k < INTBITS; ++k)
          if (labels[i][j] & 1 << k)
            {
              int c = j * INTBITS + k;

              if (c == eolbyte)
                trans[c] = state_newline;
              else if (IS_WORD_CONSTITUENT(c))
                trans[c] = state_letter;
              else if (c < NOTCHAR)
                trans[c] = state;
            }
    }

  for (i = 0; i < ngrps; ++i)
    free(grps[i].elems);
  free(follows.elems);
  free(tmp.elems);
  free(grps);
  free(labels);
}