dfamust (struct dfa *d)
{
  must *musts;
  must *mp;
  char *result;
  int ri;
  int i;
  int exact;
  token t;
  static must must0;
  struct dfamust *dm;
  static char empty_string[] = "";

  result = empty_string;
  exact = 0;
  MALLOC (musts, d->tindex + 1);
  mp = musts;
  for (i = 0; i <= d->tindex; ++i)
    mp[i] = must0;
  for (i = 0; i <= d->tindex; ++i)
    {
      mp[i].in = xmalloc(sizeof *mp[i].in);
      mp[i].left = xmalloc(2);
      mp[i].right = xmalloc(2);
      mp[i].is = xmalloc(2);
      mp[i].left[0] = mp[i].right[0] = mp[i].is[0] = '\0';
      mp[i].in[0] = NULL;
    }
#ifdef DEBUG
  fprintf(stderr, "dfamust:\n");
  for (i = 0; i < d->tindex; ++i)
    {
      fprintf(stderr, " %d:", i);
      prtok(d->tokens[i]);
    }
  putc('\n', stderr);
#endif
  for (ri = 0; ri < d->tindex; ++ri)
    {
      switch (t = d->tokens[ri])
        {
        case LPAREN:
        case RPAREN:
          assert (!"neither LPAREN nor RPAREN may appear here");
        case EMPTY:
        case BEGLINE:
        case ENDLINE:
        case BEGWORD:
        case ENDWORD:
        case LIMWORD:
        case NOTLIMWORD:
        case BACKREF:
          resetmust(mp);
          break;
        case STAR:
        case QMARK:
          assert (musts < mp);
          --mp;
          resetmust(mp);
          break;
        case OR:
          assert (&musts[2] <= mp);
          {
            char **new;
            must *lmp;
            must *rmp;
            int j, ln, rn, n;

            rmp = --mp;
            lmp = --mp;
            /* Guaranteed to be.  Unlikely, but. . . */
            if (!STREQ (lmp->is, rmp->is))
              lmp->is[0] = '\0';
            /* Left side--easy */
            i = 0;
            while (lmp->left[i] != '\0' && lmp->left[i] == rmp->left[i])
              ++i;
            lmp->left[i] = '\0';
            /* Right side */
            ln = strlen(lmp->right);
            rn = strlen(rmp->right);
            n = ln;
            if (n > rn)
              n = rn;
            for (i = 0; i < n; ++i)
              if (lmp->right[ln - i - 1] != rmp->right[rn - i - 1])
                break;
            for (j = 0; j < i; ++j)
              lmp->right[j] = lmp->right[(ln - i) + j];
            lmp->right[j] = '\0';
            new = inboth(lmp->in, rmp->in);
            if (new == NULL)
              goto done;
            freelist(lmp->in);
            free(lmp->in);
            lmp->in = new;
          }
          break;
        case PLUS:
          assert (musts < mp);
          --mp;
          mp->is[0] = '\0';
          break;
        case END:
          assert (mp == &musts[1]);
          for (i = 0; musts[0].in[i] != NULL; ++i)
            if (strlen(musts[0].in[i]) > strlen(result))
              result = musts[0].in[i];
          if (STREQ (result, musts[0].is))
            exact = 1;
          goto done;
        case CAT:
          assert (&musts[2] <= mp);
          {
            must *lmp;
            must *rmp;

            rmp = --mp;
            lmp = --mp;
            /* In.  Everything in left, plus everything in
               right, plus catenation of
               left's right and right's left. */
            lmp->in = addlists(lmp->in, rmp->in);
            if (lmp->in == NULL)
              goto done;
            if (lmp->right[0] != '\0' &&
                rmp->left[0] != '\0')
              {
                char *tp;

                tp = icpyalloc(lmp->right);
                tp = icatalloc(tp, rmp->left);
                lmp->in = enlist(lmp->in, tp, strlen(tp));
                free(tp);
                if (lmp->in == NULL)
                  goto done;
              }
            /* Left-hand */
            if (lmp->is[0] != '\0')
              {
                lmp->left = icatalloc(lmp->left,
                                      rmp->left);
                if (lmp->left == NULL)
                  goto done;
              }
            /* Right-hand */
            if (rmp->is[0] == '\0')
              lmp->right[0] = '\0';
            lmp->right = icatalloc(lmp->right, rmp->right);
            if (lmp->right == NULL)
              goto done;
            /* Guaranteed to be */
            if (lmp->is[0] != '\0' && rmp->is[0] != '\0')
              {
                lmp->is = icatalloc(lmp->is, rmp->is);
                if (lmp->is == NULL)
                  goto done;
              }
            else
              lmp->is[0] = '\0';
          }
          break;
        default:
          if (t < END)
            {
              assert (!"oops! t >= END");
            }
          else if (t == '\0')
            {
              /* not on *my* shift */
              goto done;
            }
          else if (t >= CSET
                   || !MBS_SUPPORT
                   || t == ANYCHAR
                   || t == MBCSET
                   )
            {
              /* easy enough */
              resetmust(mp);
            }
          else
            {
              /* plain character */
              resetmust(mp);
              mp->is[0] = mp->left[0] = mp->right[0] = t;
              mp->is[1] = mp->left[1] = mp->right[1] = '\0';
              mp->in = enlist(mp->in, mp->is, (size_t)1);
              if (mp->in == NULL)
                goto done;
            }
          break;
        }
#ifdef DEBUG
      fprintf(stderr, " node: %d:", ri);
      prtok(d->tokens[ri]);
      fprintf(stderr, "\n  in:");
      for (i = 0; mp->in[i]; ++i)
        fprintf(stderr, " \"%s\"", mp->in[i]);
      fprintf(stderr, "\n  is: \"%s\"\n", mp->is);
      fprintf(stderr, "  left: \"%s\"\n", mp->left);
      fprintf(stderr, "  right: \"%s\"\n", mp->right);
#endif
      ++mp;
    }
 done:
  if (strlen(result))
    {
      MALLOC(dm, 1);
      dm->exact = exact;
      MALLOC(dm->must, strlen(result) + 1);
      strcpy(dm->must, result);
      dm->next = d->musts;
      d->musts = dm;
    }
  mp = musts;
  for (i = 0; i <= d->tindex; ++i)
    {
      freelist(mp[i].in);
      free(mp[i].in);
      free(mp[i].left);
      free(mp[i].right);
      free(mp[i].is);
    }
  free(mp);
}