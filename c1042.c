copytoks (int tindex, int ntokens)
{
  int i;

  for (i = 0; i < ntokens; ++i)
    {
      addtok(dfa->tokens[tindex + i]);
      /* Update index into multibyte csets.  */
      if (MB_CUR_MAX > 1 && dfa->tokens[tindex + i] == MBCSET)
        dfa->multibyte_prop[dfa->tindex - 1] = dfa->multibyte_prop[tindex + i];
    }
}