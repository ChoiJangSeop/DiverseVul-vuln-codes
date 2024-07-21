closure (void)
{
  int tindex, ntokens, i;

  atom();
  while (tok == QMARK || tok == STAR || tok == PLUS || tok == REPMN)
    if (tok == REPMN && (minrep || maxrep))
      {
        ntokens = nsubtoks(dfa->tindex);
        tindex = dfa->tindex - ntokens;
        if (maxrep < 0)
          addtok(PLUS);
        if (minrep == 0)
          addtok(QMARK);
        for (i = 1; i < minrep; ++i)
          {
            copytoks(tindex, ntokens);
            addtok(CAT);
          }
        for (; i < maxrep; ++i)
          {
            copytoks(tindex, ntokens);
            addtok(QMARK);
            addtok(CAT);
          }
        tok = lex();
      }
    else if (tok == REPMN)
      {
        dfa->tindex -= nsubtoks(dfa->tindex);
        tok = lex();
        closure();
      }
    else
      {
        addtok(tok);
        tok = lex();
      }
}