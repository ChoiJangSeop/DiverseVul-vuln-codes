symbol_location_as_lhs_set (symbol *sym, location loc)
{
  if (!sym->location_of_lhs)
    sym->location = loc;
}