nsubtoks (int tindex)
{
  int ntoks1;

  switch (dfa->tokens[tindex - 1])
    {
    default:
      return 1;
    case QMARK:
    case STAR:
    case PLUS:
      return 1 + nsubtoks(tindex - 1);
    case CAT:
    case OR:
      ntoks1 = nsubtoks(tindex - 1);
      return 1 + ntoks1 + nsubtoks(tindex - 1 - ntoks1);
    }
}