get_quote_count(const char *line)
{
  int quote_count;
  const char *ptr= line;

  for(quote_count= 0; ptr ++ && *ptr; ptr= strpbrk(ptr, "\"\'`"))
    quote_count ++;

  return quote_count;
}