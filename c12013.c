symbol_code_set (symbol *sym, int code, location loc)
{
  int *codep = &sym->content->code;
  if (sym->content->class != token_sym)
    complain (&loc, complaint,
              _("nonterminals cannot be given a token code"));
  else if (*codep != CODE_UNDEFINED
           && *codep != code)
    complain (&loc, complaint, _("redefining code of token %s"),
              sym->tag);
  else if (code == INT_MAX)
    complain (&loc, complaint, _("code of token %s too large"),
              sym->tag);
  else
    {
      *codep = code;
      /* User defined $end token? */
      if (code == 0 && !eoftoken)
        {
          eoftoken = sym->content->symbol;
          /* It is always mapped to 0, so it was already counted in
             NTOKENS.  */
          if (eoftoken->content->number != NUMBER_UNDEFINED)
            --ntokens;
          eoftoken->content->number = 0;
        }
    }
}