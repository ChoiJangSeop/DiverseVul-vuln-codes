symbol_new (uniqstr tag, location loc)
{
  symbol *res = xmalloc (sizeof *res);
  uniqstr_assert (tag);

  /* If the tag is not a string (starts with a double quote), check
     that it is valid for Yacc. */
  if (tag[0] != '\"' && tag[0] != '\'' && strchr (tag, '-'))
    complain (&loc, Wyacc,
              _("POSIX Yacc forbids dashes in symbol names: %s"), tag);

  res->tag = tag;
  res->location = loc;
  res->translatable = false;
  res->location_of_lhs = false;
  res->alias = NULL;
  res->content = sym_content_new (res);
  res->is_alias = false;

  if (nsyms == SYMBOL_NUMBER_MAXIMUM)
    complain (NULL, fatal, _("too many symbols in input grammar (limit is %d)"),
              SYMBOL_NUMBER_MAXIMUM);
  nsyms++;
  return res;
}