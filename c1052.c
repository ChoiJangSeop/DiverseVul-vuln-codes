state_separate_contexts (position_set const *s)
{
  int separate_contexts = 0;
  unsigned int j;

  for (j = 0; j < s->nelem; ++j)
    {
      if (PREV_NEWLINE_DEPENDENT (s->elems[j].constraint))
        separate_contexts |= CTX_NEWLINE;
      if (PREV_LETTER_DEPENDENT (s->elems[j].constraint))
        separate_contexts |= CTX_LETTER;
    }

  return separate_contexts;
}