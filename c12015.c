symbol_class_set (symbol *sym, symbol_class class, location loc, bool declaring)
{
  aver (class != unknown_sym);
  sym_content *s = sym->content;
  if (class == pct_type_sym)
    {
      if (s->class == token_sym)
        complain_pct_type_on_token (&loc);
      else if (s->class == unknown_sym)
        s->class = class;
    }
  else if (s->class != unknown_sym && s->class != pct_type_sym
           && s->class != class)
    complain_class_redeclared (sym, class, loc);
  else
    {
      if (class == token_sym && s->class == pct_type_sym)
        complain_pct_type_on_token (&sym->location);

      if (class == nterm_sym && s->class != nterm_sym)
        s->number = nnterms++;
      else if (class == token_sym && s->number == NUMBER_UNDEFINED)
        s->number = ntokens++;
      s->class = class;

      if (declaring)
        {
          if (s->status == declared)
            {
              complain (&loc, Wother,
                        _("symbol %s redeclared"), sym->tag);
              subcomplain (&sym->location, Wother,
                           _("previous declaration"));
            }
          else
            {
              sym->location = loc;
              s->status = declared;
            }
        }
    }
}