static int set_default_flags(int *flags)
{
  if (((*flags) & IDN2_TRANSITIONAL) && ((*flags) & IDN2_NONTRANSITIONAL))
    return IDN2_INVALID_FLAGS;

  if (((*flags) & (IDN2_TRANSITIONAL|IDN2_NONTRANSITIONAL)) && ((*flags) & IDN2_NO_TR46))
    return IDN2_INVALID_FLAGS;

  if (!((*flags) & (IDN2_NO_TR46|IDN2_TRANSITIONAL)))
    *flags |= IDN2_NONTRANSITIONAL;

  return IDN2_OK;
}