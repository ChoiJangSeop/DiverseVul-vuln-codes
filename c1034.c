charclass_index (charclass const s)
{
  int i;

  for (i = 0; i < dfa->cindex; ++i)
    if (equal(s, dfa->charclasses[i]))
      return i;
  REALLOC_IF_NECESSARY(dfa->charclasses, dfa->calloc, dfa->cindex + 1);
  ++dfa->cindex;
  copyset(s, dfa->charclasses[i]);
  return i;
}