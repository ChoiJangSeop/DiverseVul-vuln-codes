context_length_arg (char const *str, int *out)
{
  uintmax_t value;
  if (! (xstrtoumax (str, 0, 10, &value, "") == LONGINT_OK
         && 0 <= (*out = value)
         && *out == value))
    {
      error (EXIT_TROUBLE, 0, "%s: %s", str,
             _("invalid context length argument"));
    }
}