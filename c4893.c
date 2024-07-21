cmp_addr_policies(smartlist_t *a, smartlist_t *b)
{
  int r, i;
  int len_a = a ? smartlist_len(a) : 0;
  int len_b = b ? smartlist_len(b) : 0;

  for (i = 0; i < len_a && i < len_b; ++i) {
    if ((r = cmp_single_addr_policy(smartlist_get(a, i), smartlist_get(b, i))))
      return r;
  }
  if (i == len_a && i == len_b)
    return 0;
  if (i < len_a)
    return -1;
  else
    return 1;
}