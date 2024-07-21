symbol_check_defined (symbol *sym)
{
  sym_content *s = sym->content;
  if (s->class == unknown_sym || s->class == pct_type_sym)
    {
      complain_symbol_undeclared (sym);
      s->class = nterm_sym;
      s->number = nnterms++;
    }

  if (s->class == token_sym
      && sym->tag[0] == '"'
      && !sym->is_alias)
    complain (&sym->location, Wdangling_alias,
              _("string literal %s not attached to a symbol"),
              sym->tag);

  for (int i = 0; i < 2; ++i)
    symbol_code_props_get (sym, i)->is_used = true;

  /* Set the semantic type status associated to the current symbol to
     'declared' so that we could check semantic types unnecessary uses. */
  if (s->type_name)
    {
      semantic_type *sem_type = semantic_type_get (s->type_name, NULL);
      if (sem_type)
        sem_type->status = declared;
    }
}